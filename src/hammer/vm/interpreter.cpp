#include "hammer/vm/interpreter.hpp"

#include "hammer/compiler/opcodes.hpp"
#include "hammer/core/byte_order.hpp"
#include "hammer/vm/context.hpp"
#include "hammer/vm/math.hpp"
#include "hammer/vm/objects/array.hpp"
#include "hammer/vm/objects/function.hpp"
#include "hammer/vm/objects/hash_table.hpp"
#include "hammer/vm/objects/string.hpp"

#include "hammer/vm/context.ipp"

namespace hammer::vm {

static constexpr u32 default_stack_size = 8 * 1024;
static constexpr u32 max_stack_size = 16 << 20;

template<typename T>
static T read_big_endian(const byte*& ptr) {
    T value;
    std::memcpy(&value, ptr, sizeof(T));
    value = be_to_host(value);
    ptr += sizeof(T);
    return value;
}

static bool truthy(Context& ctx, Handle<Value> v) {
    return !(v->is_null() || v->same(ctx.get_false()));
}

static Value bitwise_not(Context& ctx, Handle<Value> v) {
    return ctx.get_integer(~convert_integer(v));
}

static int compare(Handle<Value> a, Handle<Value> b) {
    if (a->is_null()) {
        if (b->is_null())
            return 0;
        return -1;
    }
    if (b->is_null())
        return 1;

    return compare_numbers(a, b);
}

static Value get_module_member(CoroutineStack::Frame* frame, u32 index) {
    Module mod = frame->tmpl.module();
    Tuple members = mod.members();
    HAMMER_CHECK(index < members.size(), "Module member index out of bounds.");
    return members.get(index);
}

static void
set_module_member(CoroutineStack::Frame* frame, u32 index, Value value) {
    Module mod = frame->tmpl.module();
    Tuple members = mod.members();
    HAMMER_CHECK(index < members.size(), "Module member index out of bounds.");
    members.set(index, value);
}

void Interpreter::init(Context& ctx) {
    ctx_ = &ctx;
}

Coroutine Interpreter::create_coroutine(Handle<Function> fn) {
    HAMMER_ASSERT(!fn->is_null(), "Invalid function.");
    HAMMER_ASSERT(current_.is_null(), "Already executing a coroutine.");

    HAMMER_CHECK(fn->tmpl().params() == 0,
        "Can only invoke nullary functions right now.");

    Root stack(ctx(), CoroutineStack::make(ctx(), default_stack_size));
    Root name(ctx(), String::make(ctx(), "Coro-1")); // TODO name
    Root coro(ctx(), Coroutine::make(ctx(), name, stack));

    bool ok = true;
    ok &= stack->push_value(fn);
    ok &= stack->push_frame(fn->tmpl(), fn->closure());
    HAMMER_CHECK(ok, "Failed to create initial function frame.");
    return coro;
}

Value Interpreter::run(Handle<Coroutine> coro) {
    HAMMER_ASSERT(current_.is_null(), "Must not be running a coroutine.");

    HAMMER_ASSERT(!coro->is_null(), "Invalid coroutine.");
    HAMMER_ASSERT(coro->state() == CoroutineState::Ready,
        "Coroutine must be in ready state.");
    HAMMER_ASSERT(coro->stack().top_frame(), "No execution frame.");

    current_ = coro.get();
    stack_ = coro->stack();
    frame_ = stack_.top_frame();
    tmpl_ = frame_->tmpl;
    code_ = tmpl_.code().view();

    ScopeExit reset_coroutine = [&] {
        current_ = Coroutine();
        stack_ = CoroutineStack();
        frame_ = nullptr;
        tmpl_ = FunctionTemplate();
        code_ = {};
    };

    // TODO eventually coroutines should be able to yield
    run_instructions();

    HAMMER_ASSERT(stack_.top_value_count() == 1,
        "Must have left one value on the stack.");
    current_.result(Handle<Value>::from_slot(stack_.top_value()));
    current_.state(CoroutineState::Done);
    current_.stack({});
    return current_.result();
}

void Interpreter::run_instructions() {
    [[maybe_unused]] auto readable = [&]() { return code_.end() - frame_->pc; };

    auto read_op = [&] {
        // TODO static verify
        HAMMER_ASSERT(readable() >= 1, "Not enough available bytes.");

        u8 opcode = *frame_->pc++;
        HAMMER_ASSERT(valid_opcode(opcode), "Invalid opcode.");
        return static_cast<Opcode>(opcode);
    };

    auto read_i64 = [&] {
        // TODO static verify
        HAMMER_ASSERT(readable() >= 8, "Not enough available bytes.");
        return static_cast<i64>(read_big_endian<u64>(frame_->pc));
    };

    auto read_f64 = [&] {
        // TODO static verify
        HAMMER_ASSERT(readable() >= 8, "Not enough available bytes.");
        // FIXME float serialization in some helper function, see also compiler/binary.hpp
        static_assert(sizeof(f64) == sizeof(u64));
        u64 as_u64 = read_big_endian<u64>(frame_->pc);
        f64 d;
        std::memcpy(&d, &as_u64, sizeof(f64));
        return d;
    };

    auto read_u32 = [&] {
        // TODO static verify
        HAMMER_ASSERT(readable() >= 4, "Not enough available bytes.");
        return read_big_endian<u32>(frame_->pc);
    };

    while (1) {
        // TODO static verify
        if (HAMMER_UNLIKELY(frame_->pc == code_.end())) {
            HAMMER_ERROR(
                "Invalid program counter: end of code reached "
                "without return from function.");
        }

        Opcode op = read_op();
        //fmt::print("Running op {}\n", to_string(op));

        ScopeExit reset_registers = [&] { registers_used_ = 0; };
        switch (op) {
        case Opcode::Invalid:
            HAMMER_ERROR("Logic error.");
            break;
        case Opcode::LoadNull:
            push_value(Value::null());
            break;
        case Opcode::LoadFalse:
            push_value(ctx().get_boolean(false));
            break;
        case Opcode::LoadTrue:
            push_value(ctx().get_boolean(true));
            break;
        case Opcode::LoadInt: {
            const i64 value = read_i64();
            push_value(ctx().get_integer(value));
            break;
        }
        case Opcode::LoadFloat: {
            const f64 value = read_f64();
            push_value(Float::make(ctx(), value));
            break;
        }
        case Opcode::LoadParam: {
            const u32 index = read_u32();
            HAMMER_ASSERT(
                index < frame_->args, "Parameter index out of bounds.");
            push_value(stack_.args()[index]);
            break;
        }
        case Opcode::StoreParam: {
            const u32 index = read_u32();
            HAMMER_ASSERT(
                index < frame_->args, "Parameter index out of bounds.");

            // TODO static verify possible?
            stack_.args()[index] = *stack_.top_value();
            stack_.pop_value();
            break;
        }
        case Opcode::LoadLocal: {
            const u32 index = read_u32();
            HAMMER_ASSERT(index < frame_->locals, "Local index out of bounds.");

            Value local = stack_.locals()[index];
            if (HAMMER_UNLIKELY(ctx().get_undefined().same(local))) {
                HAMMER_ERROR("Local value is undefined.");
            }

            push_value(local);
            break;
        }
        case Opcode::StoreLocal: {
            const u32 index = read_u32();
            HAMMER_ASSERT(index < frame_->locals, "Local index out of bounds.");
            stack_.locals()[index] = *stack_.top_value();
            stack_.pop_value();
            break;
        }
        case Opcode::LoadClosure: {
            HAMMER_CHECK(!frame_->closure.is_null(),
                "Function does not have a closure.");

            push_value(frame_->closure);
            break;
        }
        case Opcode::LoadContext: {
            const u32 level = read_u32();
            const u32 index = read_u32();

            Value* top = stack_.top_value();
            Value context_value = *top;
            HAMMER_CHECK(context_value.is<ClosureContext>(),
                "The value is not a closure context.");

            ClosureContext context = context_value.as<ClosureContext>();
            if (index != 0)
                context = context.parent(level);

            Value v = context.get(index);
            if (HAMMER_UNLIKELY(ctx().get_undefined().same(v))) {
                HAMMER_ERROR("Closure variable is undefined.");
            }

            *top = v;
            break;
        }
        case Opcode::StoreContext: {
            const u32 level = read_u32();
            const u32 index = read_u32();

            Value context_value = *stack_.top_value(1);
            HAMMER_CHECK(context_value.is<ClosureContext>(),
                "The value is not a closure context.");

            Value value = *stack_.top_value(0);

            ClosureContext context = context_value.as<ClosureContext>();
            if (index != 0)
                context = context.parent(level);
            context.set(index, value);
            stack_.pop_values(2);
            break;
        }
        case Opcode::LoadMember: {
            const u32 member_index = read_u32();
            Value symbol = get_module_member(frame_, member_index);
            HAMMER_CHECK(symbol.is<Symbol>(),
                "The module member at index {} must be a symbol.",
                member_index);

            Value* obj = stack_.top_value();
            HAMMER_CHECK(obj->is<Module>(),
                "LoadMember opcode is only implemented for modules."); // TODO

            HashTable exported = obj->as<Module>().exported();
            std::optional<Value> found;
            if (exported) {
                found = exported.get(symbol);
            }

            HAMMER_CHECK(found, "Failed to find {} in module.",
                symbol.as<Symbol>().name().view()); // TODO nicer
            *obj = *found;
            break;
        }
        case Opcode::LoadIndex: {
            // TODO indexable protocol
            MutableHandle<Value> obj = MutableHandle<Value>::from_slot(
                stack_.top_value(1));
            switch (obj->type()) {
            case ValueType::Array: {
                Handle<Array> array = obj.cast<Array>();
                Handle<Value> index = Handle<Value>::from_slot(
                    stack_.top_value(0));

                i64 raw_index;
                if (auto opt = try_extract_integer(index)) {
                    raw_index = *opt;
                } else {
                    HAMMER_ERROR("Array index must be an integer.");
                }

                HAMMER_CHECK(raw_index >= 0 && u64(raw_index) < array->size(),
                    "Invalid index {} into array of size {}.", raw_index,
                    array->size());
                obj.set(array->get(static_cast<size_t>(raw_index)));
                stack_.pop_value();
                break;
            }
            case ValueType::Tuple: {
                Handle<Tuple> tuple = obj.cast<Tuple>();
                Handle<Value> index = Handle<Value>::from_slot(
                    stack_.top_value(0));

                i64 raw_index;
                if (auto opt = try_extract_integer(index)) {
                    raw_index = *opt;
                } else {
                    HAMMER_ERROR("Tuple index must be an integer.");
                }

                HAMMER_CHECK(raw_index >= 0 && u64(raw_index) < tuple->size(),
                    "Invalid index {} into tuple of size {}.", raw_index,
                    tuple->size());
                obj.set(tuple->get(static_cast<size_t>(raw_index)));
                stack_.pop_value();
                break;
            }
            case ValueType::HashTable: {
                Handle<HashTable> table = obj.cast<HashTable>();
                Handle<Value> key = Handle<Value>::from_slot(
                    stack_.top_value(0));
                if (auto found = table->get(key.get())) {
                    obj.set(*found);
                } else {
                    obj.set(Value::null());
                }
                stack_.pop_value();
                break;
            }
            default:
                HAMMER_ERROR(
                    "Loading an index is not supported for objects of type {}.",
                    to_string(obj->type()));
            }
            break;
        }
        case Opcode::StoreIndex: {
            // TODO indexable protocol
            MutableHandle<Value> obj = MutableHandle<Value>::from_slot(
                stack_.top_value(2));
            switch (obj->type()) {
            case ValueType::Array: {
                Handle<Array> array = obj.cast<Array>();
                Handle<Value> index = Handle<Value>::from_slot(
                    stack_.top_value(1));
                Handle<Value> value = Handle<Value>::from_slot(
                    stack_.top_value(0));

                i64 raw_index;
                if (auto opt = try_extract_integer(index)) {
                    raw_index = *opt;
                } else {
                    HAMMER_ERROR("Array index must be an integer.");
                }

                HAMMER_CHECK(raw_index >= 0 && u64(raw_index) < array->size(),
                    "Invalid index {} into array of size {}.", raw_index,
                    array->size());
                array->set(static_cast<size_t>(raw_index), value);
                stack_.pop_values(3);
                break;
            }
            case ValueType::Tuple: {
                Handle<Tuple> tuple = obj.cast<Tuple>();
                Handle<Value> index = Handle<Value>::from_slot(
                    stack_.top_value(1));
                Handle<Value> value = Handle<Value>::from_slot(
                    stack_.top_value(0));

                i64 raw_index;
                if (auto opt = try_extract_integer(index)) {
                    raw_index = *opt;
                } else {
                    HAMMER_ERROR("Tuple index must be an integer.");
                }

                HAMMER_CHECK(raw_index >= 0 && u64(raw_index) < tuple->size(),
                    "Invalid index {} into tuple of size {}.", raw_index,
                    tuple->size());
                tuple->set(static_cast<size_t>(raw_index), value.get());
                stack_.pop_values(3);
                break;
            }
            case ValueType::HashTable: {
                Handle<HashTable> table = obj.cast<HashTable>();
                Handle<Value> key = Handle<Value>::from_slot(
                    stack_.top_value(1));
                Handle<Value> value = Handle<Value>::from_slot(
                    stack_.top_value(0));
                table->set(ctx(), key, value);
                stack_.pop_values(3);
                break;
            }
            default:
                HAMMER_ERROR(
                    "Loading an index is not supported for objects of type {}.",
                    to_string(obj->type()));
            }
            break;
        }
        case Opcode::LoadModule: {
            const u32 index = read_u32();
            push_value(get_module_member(frame_, index));
            break;
        }
        case Opcode::StoreModule: {
            const u32 index = read_u32();
            set_module_member(frame_, index, *stack_.top_value());
            stack_.pop_value();
            break;
        }
        case Opcode::Dup:
            push_value(*stack_.top_value());
            break;
        case Opcode::Pop:
            stack_.pop_value();
            break;
        case Opcode::Rot2: {
            Value tmp = *stack_.top_value(0);
            *stack_.top_value(0) = *stack_.top_value(1);
            *stack_.top_value(1) = tmp;
            break;
        }
        case Opcode::Rot3: {
            Value tmp = *stack_.top_value(0);
            *stack_.top_value(0) = *stack_.top_value(1);
            *stack_.top_value(1) = *stack_.top_value(2);
            *stack_.top_value(2) = tmp;
            break;
        }
        case Opcode::Rot4: {
            Value tmp = *stack_.top_value(0);
            *stack_.top_value(0) = *stack_.top_value(1);
            *stack_.top_value(1) = *stack_.top_value(2);
            *stack_.top_value(2) = *stack_.top_value(3);
            *stack_.top_value(3) = tmp;
            break;
        }
        case Opcode::Add: {
            auto a = MutableHandle<Value>::from_slot(stack_.top_value(1));
            auto b = Handle<Value>::from_slot(stack_.top_value(0));
            a.set(add(ctx(), a, b));
            stack_.pop_value();
            break;
        }
        case Opcode::Sub: {
            auto a = MutableHandle<Value>::from_slot(stack_.top_value(1));
            auto b = Handle<Value>::from_slot(stack_.top_value(0));
            a.set(sub(ctx(), a, b));
            stack_.pop_value();
            break;
        }
        case Opcode::Mul: {
            auto a = MutableHandle<Value>::from_slot(stack_.top_value(1));
            auto b = Handle<Value>::from_slot(stack_.top_value(0));
            a.set(mul(ctx(), a, b));
            stack_.pop_value();
            break;
        }
        case Opcode::Div: {
            auto a = MutableHandle<Value>::from_slot(stack_.top_value(1));
            auto b = Handle<Value>::from_slot(stack_.top_value(0));
            a.set(div(ctx(), a, b));
            stack_.pop_value();
            break;
        }
        case Opcode::Mod: {
            auto a = MutableHandle<Value>::from_slot(stack_.top_value(1));
            auto b = Handle<Value>::from_slot(stack_.top_value(0));
            a.set(mod(ctx(), a, b));
            stack_.pop_value();
            break;
        }
        case Opcode::Pow: {
            auto a = MutableHandle<Value>::from_slot(stack_.top_value(1));
            auto b = Handle<Value>::from_slot(stack_.top_value(0));
            a.set(pow(ctx(), a, b));
            stack_.pop_value();
            break;
        }
        case Opcode::LNot: {
            auto a = MutableHandle<Value>::from_slot(stack_.top_value());
            a.set(ctx().get_boolean(truthy(ctx(), a)));
            break;
        }
        case Opcode::BNot: {
            auto a = MutableHandle<Value>::from_slot(stack_.top_value());
            a.set(bitwise_not(ctx(), a));
            break;
        }
        case Opcode::UPos: {
            auto a = MutableHandle<Value>::from_slot(stack_.top_value());
            a.set(unary_plus(ctx(), a));
            break;
        }
        case Opcode::UNeg: {
            auto a = MutableHandle<Value>::from_slot(stack_.top_value());
            a.set(unary_minus(ctx(), a));
            break;
        }
        case Opcode::Gt: {
            auto a = MutableHandle<Value>::from_slot(stack_.top_value(1));
            auto b = Handle<Value>::from_slot(stack_.top_value(0));
            a.set(ctx().get_boolean(compare(a, b) > 0));
            stack_.pop_value();
            break;
        }
        case Opcode::Gte: {
            auto a = MutableHandle<Value>::from_slot(stack_.top_value(1));
            auto b = Handle<Value>::from_slot(stack_.top_value(0));
            a.set(ctx().get_boolean(compare(a, b) >= 0));
            stack_.pop_value();
            break;
        }
        case Opcode::Lt: {
            auto a = MutableHandle<Value>::from_slot(stack_.top_value(1));
            auto b = Handle<Value>::from_slot(stack_.top_value(0));
            a.set(ctx().get_boolean(compare(a, b) < 0));
            stack_.pop_value();
            break;
        }
        case Opcode::Lte: {
            auto a = MutableHandle<Value>::from_slot(stack_.top_value(1));
            auto b = Handle<Value>::from_slot(stack_.top_value(0));
            a.set(ctx().get_boolean(compare(a, b) <= 0));
            stack_.pop_value();
            break;
        }
        case Opcode::Eq: {
            auto a = MutableHandle<Value>::from_slot(stack_.top_value(1));
            auto b = Handle<Value>::from_slot(stack_.top_value(0));
            a.set(ctx().get_boolean(equal(a, b)));
            stack_.pop_value();
            break;
        }
        case Opcode::NEq: {
            auto a = MutableHandle<Value>::from_slot(stack_.top_value(1));
            auto b = Handle<Value>::from_slot(stack_.top_value(0));
            a.set(ctx().get_boolean(!equal(a, b)));
            stack_.pop_value();
            break;
        }
        case Opcode::MkArray: {
            const u32 size = read_u32();
            const Span<const Value> values = stack_.top_values(size);

            auto array = reg<Array>(Array::make(ctx(), values));
            stack_.pop_values(size);
            push_value(array.get());
            break;
        }
        case Opcode::MkTuple: {
            const u32 size = read_u32();
            const Span<const Value> values = stack_.top_values(size);

            auto tuple = reg<>(Tuple::make(ctx(), values));
            stack_.pop_values(size);
            push_value(tuple.get());
            break;
        }
        case Opcode::MkMap: {
            // FIXME overflow protection
            const u32 pairs = read_u32();
            const u32 kv_count = pairs * 2;
            const Span<Value> kvs = stack_.top_values(kv_count);

            auto map = reg<HashTable>(HashTable::make(ctx(), pairs));
            for (u32 i = 0; i < kv_count; i += 2) {
                auto key = Handle<Value>::from_slot(kvs.data() + i);
                auto value = Handle<Value>::from_slot(kvs.data() + i + 1);
                map->set(ctx(), key, value);
            }

            stack_.pop_values(kv_count);
            stack_.push_value(map.get());
            break;
        }
        case Opcode::MkContext: {
            const u32 size = read_u32();

            auto context_value = MutableHandle<Value>::from_slot(
                stack_.top_value());
            HAMMER_CHECK(
                context_value->is_null() || context_value->is<ClosureContext>(),
                "Parent of closure context must be null or a another closure "
                "context.");
            context_value.set(ClosureContext::make(
                ctx(), size, context_value.cast<ClosureContext>()));
            break;
        }
        case Opcode::MkClosure: {
            auto tmpl_value = MutableHandle<Value>::from_slot(
                stack_.top_value(1));
            HAMMER_CHECK(tmpl_value->is<FunctionTemplate>(),
                "First argument to MkClosure must be a function template.");

            auto closure_value = Handle<Value>::from_slot(stack_.top_value(0));
            HAMMER_CHECK(
                closure_value->is_null() || closure_value->is<ClosureContext>(),
                "Second argument to MkClosure must be null or a closure "
                "context.");

            tmpl_value.set(Function::make(ctx(),
                tmpl_value.strict_cast<FunctionTemplate>(),
                closure_value.cast<ClosureContext>()));
            stack_.pop_value();
            break;
        }
        case Opcode::Jmp: {
            const u32 offset = read_u32();
            // TODO static verify
            HAMMER_ASSERT(offset < code_.size(), "Invalid jump destination.");
            frame_->pc = code_.data() + offset;
            break;
        }
        case Opcode::JmpTrue: {
            const u32 offset = read_u32();
            // TODO static verify
            HAMMER_ASSERT(offset < code_.size(), "Invalid jump destination.");
            if (truthy(ctx(), Handle<Value>::from_slot(stack_.top_value()))) {
                frame_->pc = code_.data() + offset;
            }
            break;
        }
        case Opcode::JmpTruePop: {
            const u32 offset = read_u32();
            // TODO static verify
            HAMMER_ASSERT(offset < code_.size(), "Invalid jump destination.");
            if (truthy(ctx(), Handle<Value>::from_slot(stack_.top_value()))) {
                frame_->pc = code_.data() + offset;
            }
            stack_.pop_value();
            break;
        }
        case Opcode::JmpFalse: {
            const u32 offset = read_u32();
            // TODO static verify
            HAMMER_ASSERT(offset < code_.size(), "Invalid jump destination.");
            if (!truthy(ctx(), Handle<Value>::from_slot(stack_.top_value()))) {
                frame_->pc = code_.data() + offset;
            }
            break;
        }
        case Opcode::JmpFalsePop: {
            const u32 offset = read_u32();
            // TODO static verify
            HAMMER_ASSERT(offset < code_.size(), "Invalid jump destination.");
            if (!truthy(ctx(), Handle<Value>::from_slot(stack_.top_value()))) {
                frame_->pc = code_.data() + offset;
            }
            stack_.pop_value();
            break;
        }
        case Opcode::Call: {
            const u32 args = read_u32();
            Value* funcval = stack_.top_value(args);

            if (funcval->is<Function>()) {
                auto tmpl = reg(funcval->as<Function>().tmpl());
                auto closure = reg(funcval->as<Function>().closure());
                if (tmpl->params() != args) {
                    HAMMER_ERROR(
                        "Invalid number of function arguments (need {}, got "
                        "{}).",
                        tmpl->params(), args);
                }

                push_frame(tmpl, closure);
                frame_ = stack_.top_frame();
                tmpl_ = frame_->tmpl;
                code_ = tmpl_.code().view();
                break;
            } else if (funcval->is<NativeFunction>()) {
                auto native = reg(funcval->as<NativeFunction>());
                if (args < native->min_params()) {
                    HAMMER_ERROR(
                        "Invalid number of function arguments (need {}, got "
                        "{}).",
                        native->min_params(), args);
                }

                *funcval = Value::null(); // Default for return value
                NativeFunction::Frame native_frame(ctx(),
                    stack_.top_values(args),
                    MutableHandle<Value>::from_slot(funcval));
                native->function()(native_frame);
                stack_.pop_values(args);
            } else {
                HAMMER_ERROR("Cannot call object of type {} as a function.",
                    to_string(funcval->type()));
            }
            break;
        }
        case Opcode::CallMember: {
            const u32 symbol_index = read_u32();
            const u32 args = read_u32();

            Value* object = stack_.top_value(args);
            auto symbol = reg(get_module_member(frame_, symbol_index));
            HAMMER_CHECK(symbol->is<Symbol>(),
                "Referenced module member must be a symbol.");

            // TODO: Call actual user defined member functions
            auto member = ctx().types().member_function_invokable(
                ctx(), Handle<Value>::from_slot(object), symbol.cast<Symbol>());
            HAMMER_CHECK(member,
                "Failed to find member function {} on object of type {}.",
                symbol->as<Symbol>().name().view(), to_string(object->type()));

            // TODO: Must have flags that specifiy whether a function needs "this" or not.
            // For now, we always pass "this" here.
            if (member->type() == ValueType::NativeFunction) {
                auto native = reg(member->as<NativeFunction>());

                u32 native_args = args;
                if (native->method()) {
                    // This includes the original object in the argument list;
                    // it is directly after the arguments in the stack_.
                    if (HAMMER_UNLIKELY(!checked_add(native_args, u32(1))))
                        HAMMER_ERROR("Too many function call arguments.");
                }

                if (native_args < native->min_params()) {
                    HAMMER_ERROR(
                        "Invalid number of function arguments (need {}, got "
                        "{}).",
                        native->min_params(), native_args);
                }

                auto result = reg(Value::null());
                NativeFunction::Frame native_frame(
                    ctx(), stack_.top_values(native_args), result);
                native->function()(native_frame);
                *object = result.get();
                stack_.pop_values(args);
            } else {
                HAMMER_ERROR(
                    "Not implemented: used defined member functions."); // FIXME
            }
            break;
        }
        case Opcode::Ret: {
            const u32 args = frame_->args;
            Value result = *stack_.top_value();
            stack_.pop_frame();
            stack_.pop_values(args);      // Function arguments
            *stack_.top_value() = result; // This was the function or the object

            // TODO move frame bookkeeping
            if (auto* frame = stack_.top_frame()) {
                frame_ = frame;
                tmpl_ = frame->tmpl;
                code_ = tmpl_.code().view();
            } else {
                return; // Coroutine done.
            }
            break;
        }
        case Opcode::AssertFail: {
            Value* expr = stack_.top_value(1);
            Value* message = stack_.top_value(0);

            HAMMER_CHECK(expr->is<String>(),
                "Assertion expression message must be a string value.");
            HAMMER_CHECK(message->is_null() || message->is<String>(),
                "Assertion error message must be a string or null.");

            if (message->is_null()) {
                HAMMER_ERROR(
                    "Assertion `{}` failed.", expr->as<String>().view());
            } else {
                HAMMER_ERROR("Assertion `{}` failed: {}",
                    expr->as<String>().view(), message->as<String>().view());
            }
            break;
        }

        case Opcode::LSh:
        case Opcode::RSh:
        case Opcode::BAnd:
        case Opcode::BOr:
        case Opcode::BXor:
        case Opcode::MkSet:
        case Opcode::StoreMember:
        case Opcode::LoadGlobal:
            HAMMER_ERROR("Instruction not implemented: {}.", to_string(op));
            break;
        }
    }
}

// v is saved in the (rare) slow path inside a register (this is done for performance).
// It may not be necessary.
void Interpreter::push_value(Value v) {
    if (HAMMER_LIKELY(stack_.push_value(v)))
        return;

    auto saved = reg(v);
    grow_stack();
    [[maybe_unused]] bool ok = stack_.push_value(saved);
    HAMMER_ASSERT(ok, "Failed to push value after stack growth.");
}

void Interpreter::push_frame(
    Handle<FunctionTemplate> tmpl, Handle<ClosureContext> closure) {
    if (HAMMER_LIKELY(stack_.push_frame(tmpl.get(), closure.get())))
        return;

    grow_stack();
    [[maybe_unused]] bool ok = stack_.push_frame(tmpl.get(), closure.get());
    HAMMER_ASSERT(ok, "Failed to push frame after stack growth.");
}

void Interpreter::grow_stack() {
    u32 next_size;
    if (HAMMER_UNLIKELY(!checked_mul<u32>(stack_.stack_size(), 2, next_size)))
        HAMMER_ERROR("Overflow in stack size computation.");

    if (HAMMER_UNLIKELY(next_size > max_stack_size))
        HAMMER_ERROR("Stack overflow.");

    Root old_stack(ctx(), stack_);
    Root new_stack(ctx(), CoroutineStack::grow(ctx(), old_stack, next_size));

    current_.stack(new_stack);
    stack_ = new_stack;
    frame_ = stack_.top_frame();
};

Value* Interpreter::allocate_register_slot() {
    // This error would be a programming error, the maximum number of
    // internal registers has a static upper limit.
    HAMMER_CHECK(registers_used_ < registers_.size(),
        "No more registers: all are already allocated.");
    return &registers_[registers_used_++];
}

} // namespace hammer::vm
