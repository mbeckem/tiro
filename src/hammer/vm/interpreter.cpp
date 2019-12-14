#include "hammer/vm/interpreter.hpp"

#include "hammer/core/byte_order.hpp"
#include "hammer/vm/context.hpp"
#include "hammer/vm/math.hpp"
#include "hammer/vm/objects/array.hpp"
#include "hammer/vm/objects/classes.hpp"
#include "hammer/vm/objects/function.hpp"
#include "hammer/vm/objects/hash_table.hpp"
#include "hammer/vm/objects/string.hpp"

#include "hammer/vm/context.ipp"
#include "hammer/vm/objects/coroutine.ipp"

#include <cstring>

namespace hammer::vm {

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

static Value get_module_member(UserFrame* frame, u32 index) {
    Module mod = frame->tmpl.module();
    Tuple members = mod.members();
    HAMMER_CHECK(index < members.size(), "Module member index out of bounds.");
    return members.get(index);
}

static void set_module_member(UserFrame* frame, u32 index, Value value) {
    Module mod = frame->tmpl.module();
    Tuple members = mod.members();
    HAMMER_CHECK(index < members.size(), "Module member index out of bounds.");
    members.set(index, value);
}

static Opcode read_op(UserFrame* frame);
static i64 read_i64(UserFrame* frame);
static f64 read_f64(UserFrame* frame);
static u32 read_u32(UserFrame* frame);
static size_t readable(UserFrame* frame);
static bool offset_in_bounds(UserFrame* frame, u32 offset);

void Interpreter::init(Context& ctx) {
    ctx_ = &ctx;
}

Coroutine Interpreter::create_coroutine(Handle<Value> function) {
    HAMMER_ASSERT(current_.is_null(), "Already executing a coroutine.");
    HAMMER_CHECK(!function->is_null(), "Invalid function object.");

    Root stack(
        ctx(), CoroutineStack::make(ctx(), CoroutineStack::initial_size));
    Root name(ctx(), String::make(ctx(), "Coro-1")); // TODO name
    return Coroutine::make(ctx(), name, function, stack);
}

void Interpreter::run(Handle<Coroutine> coro) {
    HAMMER_ASSERT(current_.is_null(), "Must not be running a coroutine.");
    HAMMER_ASSERT(!coro->is_null(), "Invalid coroutine.");

    current_ = coro.get();
    stack_ = coro->stack();
    frame_ = stack_.top_frame();
    ScopeExit reset_coroutine = [&] {
        current_ = Coroutine();
        stack_ = CoroutineStack();
        frame_ = nullptr;
    };

    run_until_block();

    if (current_.state() == CoroutineState::Done) {
        HAMMER_ASSERT(stack_.top_value_count() == 1,
            "Must have left one value on the stack.");
        current_.result(Handle<Value>::from_slot(stack_.top_value()));
        current_.stack({});
    } else {
        HAMMER_ASSERT(current_.state() == CoroutineState::Waiting,
            "Invalid coroutine state after running, must be either Done or "
            "Waiting.");
    }
}

void Interpreter::run_until_block() {
    HAMMER_ASSERT(is_runnable(current_.state()),
        "Coroutine must be in a runnable state.");

    // This is the first time the coroutine runs. Start interpreting the job function.
    if (current_.state() == CoroutineState::New) {
        current_.state(CoroutineState::Running);

        push_value(current_.function());
        switch (call_function(0)) {
        case CallResult::Continue:
            current_.state(CoroutineState::Running);
            break;
        case CallResult::Evaluated:
            current_.state(CoroutineState::Done);
            break;
        case CallResult::Yield:
            current_.state(CoroutineState::Waiting);
            break;
        }
    } else {
        current_.state(CoroutineState::Running);
    }

    // Interpret call frames until yield or done
    CoroutineState state = current_.state();
    while (state == CoroutineState::Running) {
        HAMMER_ASSERT(frame_, "Invalid frame.");

        switch (frame_->type) {
        case FrameType::User:
            state = run_frame();
            break;
        case FrameType::Async:
            state = run_async_frame();
            break;
        }

        HAMMER_ASSERT(state == CoroutineState::Running
                          || state == CoroutineState::Waiting
                          || state == CoroutineState::Done,
            "Unexpected coroutine state.");
    }

    current_.state(state);
}

CoroutineState Interpreter::run_frame() {
    auto frame = [&] {
        HAMMER_ASSERT(frame_, "Invalid frame.");
        HAMMER_ASSERT(frame_->type == FrameType::User,
            "Current frame is not a user frame.");
        return static_cast<UserFrame*>(frame_);
    };

    while (1) {
        // TODO static verify
        if (HAMMER_UNLIKELY(frame()->pc == frame()->tmpl.code().view().end())) {
            HAMMER_ERROR(
                "Invalid program counter: end of code reached "
                "without return from function.");
        }

        Opcode op = read_op(frame());
        // fmt::print("Running op {}\n", to_string(op));

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
            const i64 value = read_i64(frame());
            push_value(ctx().get_integer(value));
            break;
        }
        case Opcode::LoadFloat: {
            const f64 value = read_f64(frame());
            push_value(Float::make(ctx(), value));
            break;
        }
        case Opcode::LoadParam: {
            // TODO static verify param index
            const u32 index = read_u32(frame());
            push_value(*stack_.arg(index));
            break;
        }
        case Opcode::StoreParam: {
            // TODO static verify param index
            const u32 index = read_u32(frame());
            HAMMER_ASSERT(
                index < frame()->args, "Parameter index out of bounds.");

            *stack_.arg(index) = *stack_.top_value();
            stack_.pop_value();
            break;
        }
        case Opcode::LoadLocal: {
            // TODO static verify local index
            const u32 index = read_u32(frame());

            Value local = *stack_.local(index);
            if (HAMMER_UNLIKELY(ctx().get_undefined().same(local))) {
                HAMMER_ERROR("Local value is undefined.");
            }

            push_value(local);
            break;
        }
        case Opcode::StoreLocal: {
            // TODO static verify local index
            const u32 index = read_u32(frame());
            *stack_.local(index) = *stack_.top_value();
            stack_.pop_value();
            break;
        }
        case Opcode::LoadClosure: {
            HAMMER_CHECK(!frame()->closure.is_null(),
                "Function does not have a closure.");

            push_value(frame()->closure);
            break;
        }
        case Opcode::LoadContext: {
            const u32 level = read_u32(frame());
            const u32 index = read_u32(frame());

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
            const u32 level = read_u32(frame());
            const u32 index = read_u32(frame());

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
            const u32 member_index = read_u32(frame());
            auto symbol = reg(get_module_member(frame(), member_index));
            HAMMER_CHECK(symbol->is<Symbol>(),
                "The module member at index {} must be a symbol.",
                member_index);

            auto object = Handle<Value>::from_slot(stack_.top_value());

            auto found = ctx().types().load_member(
                ctx(), object, symbol.cast<Symbol>());
            HAMMER_CHECK(found,
                "Failed to load property {} in value of type {}.",
                symbol->as<Symbol>().name().view(),
                to_string(object->type())); // TODO nicer

            *stack_.top_value() = *found;
            break;
        }
        case Opcode::StoreMember: {
            const u32 member_index = read_u32(frame());
            auto symbol = reg(get_module_member(frame(), member_index));
            HAMMER_CHECK(symbol->is<Symbol>(),
                "The module member at index {} must be a symbol.",
                member_index);

            auto object = Handle<Value>::from_slot(stack_.top_value(1));
            auto value = Handle<Value>::from_slot(stack_.top_value(0));

            bool ok = ctx().types().store_member(
                ctx(), object, symbol.cast<Symbol>(), value);
            HAMMER_CHECK(ok, "Failed to store property {} in value of type {}.",
                symbol->as<Symbol>().name().view(),
                to_string(object->type())); // TODO nicer

            stack_.pop_values(2);
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
            const u32 index = read_u32(frame());
            push_value(get_module_member(frame(), index));
            break;
        }
        case Opcode::StoreModule: {
            const u32 index = read_u32(frame());
            set_module_member(frame(), index, *stack_.top_value());
            stack_.pop_value();
            break;
        }
        case Opcode::Dup:
            push_value(*stack_.top_value());
            break;
        case Opcode::Pop:
            HAMMER_CHECK(
                stack_.top_value_count() > 0, "Cannot pop any more values.");
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
            const u32 size = read_u32(frame());
            const Span<const Value> values = stack_.top_values(size);

            auto array = reg<Array>(Array::make(ctx(), values));
            stack_.pop_values(size);
            push_value(array.get());
            break;
        }
        case Opcode::MkTuple: {
            const u32 size = read_u32(frame());
            const Span<const Value> values = stack_.top_values(size);

            auto tuple = reg<>(Tuple::make(ctx(), values));
            stack_.pop_values(size);
            push_value(tuple.get());
            break;
        }
        case Opcode::MkMap: {
            // FIXME overflow protection
            const u32 pairs = read_u32(frame());
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
            const u32 size = read_u32(frame());

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
            const u32 offset = read_u32(frame());
            // TODO static verify
            jump(frame(), offset);
            break;
        }
        case Opcode::JmpTrue: {
            const u32 offset = read_u32(frame());
            // TODO static verify
            if (truthy(ctx(), Handle<Value>::from_slot(stack_.top_value()))) {
                jump(frame(), offset);
            }
            break;
        }
        case Opcode::JmpTruePop: {
            const u32 offset = read_u32(frame());
            // TODO static verify
            if (truthy(ctx(), Handle<Value>::from_slot(stack_.top_value()))) {
                jump(frame(), offset);
            }
            stack_.pop_value();
            break;
        }
        case Opcode::JmpFalse: {
            const u32 offset = read_u32(frame());
            // TODO static verify
            if (!truthy(ctx(), Handle<Value>::from_slot(stack_.top_value()))) {
                jump(frame(), offset);
            }
            break;
        }
        case Opcode::JmpFalsePop: {
            const u32 offset = read_u32(frame());
            // TODO static verify
            if (!truthy(ctx(), Handle<Value>::from_slot(stack_.top_value()))) {
                jump(frame(), offset);
            }
            stack_.pop_value();
            break;
        }
        case Opcode::Call: {
            const u32 argc = read_u32(frame());
            switch (call_function(argc)) {
            case CallResult::Continue:
            case CallResult::Evaluated:
                return CoroutineState::Running;
            case CallResult::Yield:
                return CoroutineState::Waiting;
            }
            break;
        }
        case Opcode::LoadMethod: {
            const u32 symbol_index = read_u32(frame());

            auto object = reg(*stack_.top_value());
            auto symbol = reg(get_module_member(frame(), symbol_index));
            HAMMER_CHECK(symbol->is<Symbol>(),
                "Referenced module member must be a symbol.");

            auto func = reg(Value::null());
            if (auto opt = ctx().types().load_method(
                    ctx(), object, symbol.cast<Symbol>());
                HAMMER_LIKELY(opt)) {
                func.set(*opt);
            } else {
                HAMMER_ERROR(
                    "Failed to find attribute {} on object of type {}.",
                    symbol->as<Symbol>().name().view(),
                    to_string(object->type()));
            }

            if (func->is<Method>()) {
                *stack_.top_value() = func.cast<Method>()->function();
                push_value(object);
            } else {
                *stack_.top_value() = func;
                push_value(Value::null());
            }
            break;
        }
        case Opcode::CallMethod: {
            const u32 argc = read_u32(frame());
            switch (call_method(argc)) {
            case CallResult::Continue:
            case CallResult::Evaluated:
                return CoroutineState::Running;
            case CallResult::Yield:
                return CoroutineState::Waiting;
            }
            break;
        }
        case Opcode::Ret: {
            return exit_function(*stack_.top_value());
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
        case Opcode::LoadGlobal:
            HAMMER_ERROR("Instruction not implemented: {}.", to_string(op));
            break;
        }
    }
}

CoroutineState Interpreter::run_async_frame() {
    // We are entering a async function frame. That means that the initial async function
    // (which has suspended the coroutine) has resumed it. The result is ready (within the frame)
    // and we must simply return it to the caller.
    HAMMER_ASSERT(frame_->type == FrameType::Async, "Expected an async frame.");
    AsyncFrame* af = static_cast<AsyncFrame*>(frame_);
    return exit_function(af->return_value);
}

Interpreter::CallResult Interpreter::call_function(u32 argc) {
    HAMMER_ASSERT(stack_.top_value_count() >= argc + 1,
        "The value stack must contain the function object and all arguments.");
    return enter_function(argc, argc, false);
}

Interpreter::CallResult Interpreter::call_method(u32 argc) {
    HAMMER_ASSERT(stack_.top_value_count() >= argc + 2,
        "The value stack must contain the function object and all arguments.");

    // TODO overflow
    if (!stack_.top_value(argc)->is_null()) {
        // LOAD_METHOD determined that the function is a method - include the non-null
        // object in the argument count.
        return enter_function(argc + 1, argc + 1, false);
    } else {
        return enter_function(argc + 1, argc, true);
    }
}

Interpreter::CallResult Interpreter::enter_function(
    u32 function_location, u32 argc, bool pop_one_more) {

    // Returns a handle to the function. The handle becomes invalid if the stack moves.
    auto func_handle = [&] {
        return MutableHandle<Value>::from_slot(
            stack_.top_value(function_location));
    };

    auto frame_flags = [&] {
        u8 flags = 0;
        flags |= pop_one_more ? FRAME_POP_ONE_MORE : 0;
        return flags;
    };

    const ValueType function_type = func_handle()->type();
    switch (function_type) {

    // Invokes a user defined function call. A new stack frame is pushed onto the stack, then we return.
    // The interpeter will continue the evaluation in the new function frame.
    // The final return instruction in the callee will restore the stack.
    // If `pop_one_more` is true, an additional value will be popped after returning from the callee.
    // This can happen if a normal function is called via the LOAD_METHOD / CALL_METHOD instruction pair,
    // in that case the unused `this` argument is on the stack but remains unused (it must still be popped, though).
    case ValueType::Function: {
        auto func = func_handle().cast<Function>();
        auto tmpl = reg(func->tmpl());
        auto closure = reg(func->closure());
        if (tmpl->params() != argc) {
            HAMMER_ERROR(
                "Invalid number of function arguments (need {}, but have {}).",
                tmpl->params(), argc);
        }

        push_user_frame(tmpl, closure, frame_flags());
        return CallResult::Continue;
    }

    // Invokes a member function with a bound "this" parameter.
    // TODO Write a test for this and implement the bound-method creation in LOAD_MEMBER
    case ValueType::BoundMethod: {
        reserve_values(1);

        must_push_value(Value::null());
        ++function_location;

        auto bound = func_handle().cast<BoundMethod>();

        // Shift all existing arguments by one slot and
        // add the "this" parameter at the front.
        auto args = stack_.top_values(argc + 1);
        std::memmove(args.data() + 1, args.data(), argc);
        args[0] = bound->object();

        // Replace the callee.
        func_handle().set(bound->function());

        // Invoke the new callee.
        return enter_function(function_location, argc + 1, pop_one_more);
    }

    // Invokes a simple native function in a synchronous fashion.
    // This will evaluate the function and return to the caller with the function's result.
    case ValueType::NativeFunction: {
        auto native_func = func_handle().cast<NativeFunction>();
        if (argc < native_func->min_params()) {
            HAMMER_ERROR(
                "Invalid number of function arguments (need {}, but have {}).",
                native_func->min_params(), argc);
        }

        auto result = reg(Value::null()); // Default return value
        NativeFunction::Frame native_frame(
            ctx(), native_func, stack_.top_values(argc), result);
        native_func->function()(native_frame);
        stack_.pop_values(argc + (pop_one_more ? 1 : 0));
        *stack_.top_value() = result;
        return CallResult::Evaluated;
    }

    // Invokes a native async function. The function call below should
    // start and asynchronous action and suspend the coroutine. Once
    // the coroutine is resumed again, the interpreter will see an AsyncFrame
    // and return with the result found there.
    case ValueType::NativeAsyncFunction: {
        auto async_function = func_handle().cast<NativeAsyncFunction>();
        if (argc < async_function->min_params()) {
            HAMMER_ERROR(
                "Invalid number of function arguments (need {}, but have {}).",
                async_function->min_params(), argc);
        }

        push_async_frame(async_function, argc, frame_flags());

        AsyncFrame* af = static_cast<AsyncFrame*>(frame_);
        NativeAsyncFunction::Frame native_frame(ctx(),
            Handle<Coroutine>::from_slot(&current_),
            Handle<NativeAsyncFunction>::from_slot(&af->func),
            stack_.top_values(argc),
            MutableHandle<Value>::from_slot(&af->return_value));

        auto native_func = async_function->function();
        native_func(std::move(native_frame));

        HAMMER_ASSERT(current_.state() == CoroutineState::Running,
            "The async native function must not alter the coroutine state in "
            "its initiating call.");
        return CallResult::Yield;
    }

    default:
        HAMMER_ERROR("Cannot call object of type {} as a function.",
            to_string(function_type));
    }
}

CoroutineState Interpreter::exit_function(Value return_value) {
    HAMMER_ASSERT(frame_, "Invalid frame.");

    u32 pop_args = frame_->args;
    if (frame_->flags & FRAME_POP_ONE_MORE) {
        // Normal function invoked via CALL_METHOD, pop the additional value,
        // the comment for call_method.
        ++pop_args;
    }

    pop_frame();
    stack_.pop_values(pop_args);        // Function arguments
    *stack_.top_value() = return_value; // This was the function object
    return stack_.top_frame() ? CoroutineState::Running : CoroutineState::Done;
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

void Interpreter::must_push_value(Value v) {
    if (HAMMER_LIKELY(stack_.push_value(v)))
        return;

    // Programming error; use reserve_values() correctly.
    HAMMER_ERROR(
        "The stack is full "
        "(failed to reserve enough capacity beforehand).");
}

void Interpreter::push_user_frame(
    Handle<FunctionTemplate> tmpl, Handle<ClosureContext> closure, u8 flags) {
    if (HAMMER_UNLIKELY(
            !stack_.push_user_frame(tmpl.get(), closure.get(), flags))) {
        grow_stack();
        [[maybe_unused]] bool ok = stack_.push_user_frame(
            tmpl.get(), closure.get(), flags);
        HAMMER_ASSERT(ok, "Failed to push frame after stack growth.");
    }
    update_frame();
}

void Interpreter::push_async_frame(
    Handle<NativeAsyncFunction> func, u32 argc, u8 flags) {
    if (HAMMER_UNLIKELY(!stack_.push_async_frame(func.get(), argc, flags))) {
        grow_stack();
        [[maybe_unused]] bool ok = stack_.push_async_frame(
            func.get(), argc, flags);
        HAMMER_ASSERT(ok, "Failed to push frame after stack growth.");
    }
    update_frame();
}

void Interpreter::pop_frame() {
    HAMMER_ASSERT(
        stack_.top_frame(), "Cannot pop a frame from an empty call stack.");
    HAMMER_ASSERT(stack_.top_frame() == frame_, "Unexpected current frame.");

    stack_.pop_frame();
    update_frame();
}

void Interpreter::update_frame() {
    HAMMER_ASSERT(stack_, "Null stack.");
    frame_ = stack_.top_frame();
}

// TODO: All functions/opcodes in this file should use reserve ahead of time.
void Interpreter::reserve_values(u32 value_count) {
    while (stack_.value_capacity_remaining() < value_count) {
        grow_stack();
    }
}

void Interpreter::grow_stack() {
    u32 next_size;
    if (HAMMER_UNLIKELY(!checked_mul<u32>(stack_.object_size(), 2, next_size)))
        HAMMER_ERROR("Overflow in stack size computation.");

    if (HAMMER_UNLIKELY(next_size > CoroutineStack::max_size))
        HAMMER_ERROR("Stack overflow.");

    Root old_stack(ctx(), stack_);
    Root new_stack(ctx(), CoroutineStack::grow(ctx(), old_stack, next_size));

    current_.stack(new_stack);
    stack_ = new_stack;

    HAMMER_ASSERT(frame_->type == FrameType::User,
        "Other frame types not implemented yet."); // TODO
    frame_ = static_cast<UserFrame*>(stack_.top_frame());
};

void Interpreter::jump(UserFrame* frame, u32 offset) {
    HAMMER_ASSERT(
        offset_in_bounds(frame, offset), "Jump destination is out of bounds.");
    frame->pc = frame->tmpl.code().data() + offset;
}

Value* Interpreter::allocate_register_slot() {
    // This error would be a programming error, the maximum number of
    // internal registers has a static upper limit.
    HAMMER_CHECK(registers_used_ < registers_.size(),
        "No more registers: all are already allocated.");
    return &registers_[registers_used_++];
}

Opcode read_op(UserFrame* frame) {
    // TODO static verify
    HAMMER_ASSERT(readable(frame) >= 1, "Not enough available bytes.");

    u8 opcode = *frame->pc++;
    HAMMER_ASSERT(valid_opcode(opcode), "Invalid opcode.");
    return static_cast<Opcode>(opcode);
};

i64 read_i64(UserFrame* frame) {
    // TODO static verify
    HAMMER_ASSERT(readable(frame) >= 8, "Not enough available bytes.");
    return static_cast<i64>(read_big_endian<u64>(frame->pc));
};

f64 read_f64(UserFrame* frame) {
    // TODO static verify
    HAMMER_ASSERT(readable(frame) >= 8, "Not enough available bytes.");
    // FIXME float serialization in some helper function, see also compiler/binary.hpp
    static_assert(sizeof(f64) == sizeof(u64));
    u64 as_u64 = read_big_endian<u64>(frame->pc);
    f64 d;
    std::memcpy(&d, &as_u64, sizeof(f64));
    return d;
};

u32 read_u32(UserFrame* frame) {
    // TODO static verify
    HAMMER_ASSERT(readable(frame) >= 4, "Not enough available bytes.");
    return read_big_endian<u32>(frame->pc);
};

size_t readable(UserFrame* frame) {
    return static_cast<size_t>(frame->tmpl.code().view().end() - frame->pc);
}

bool offset_in_bounds(UserFrame* frame, u32 offset) {
    return offset < frame->tmpl.code().size();
}

} // namespace hammer::vm
