#include "hammer/vm/context.hpp"

#include "hammer/compiler/opcodes.hpp"
#include "hammer/compiler/output.hpp"
#include "hammer/compiler/string_table.hpp"
#include "hammer/core/byte_order.hpp"
#include "hammer/core/defs.hpp"
#include "hammer/core/overloaded.hpp"
#include "hammer/vm/handles.hpp"
#include "hammer/vm/objects/array.hpp"
#include "hammer/vm/objects/coroutine.hpp"
#include "hammer/vm/objects/function.hpp"
#include "hammer/vm/objects/hash_table.hpp"
#include "hammer/vm/objects/object.hpp"
#include "hammer/vm/objects/string.hpp"

#include "hammer/vm/context.ipp"

#include <cmath>

namespace hammer::vm {

static constexpr u32 default_stack_size = 10 * 1024;
static constexpr u32 max_stack_size = 4 << 20;

template<typename T>
static T read_big_endian(const byte*& ptr) {
    T value;
    std::memcpy(&value, ptr, sizeof(T));
    value = be_to_host(value);
    ptr += sizeof(T);
    return value;
}

struct add_op {
    i64 operator()(i64 a, i64 b) {
        i64 result;
        if (HAMMER_UNLIKELY(!checked_add(a, b, result))) // TODO exception
            HAMMER_ERROR("Integer overflow in addition.");
        return result;
    }

    f64 operator()(f64 a, f64 b) { return a + b; }
};

struct sub_op {
    i64 operator()(i64 a, i64 b) {
        i64 result;
        if (HAMMER_UNLIKELY(!checked_sub(a, b, result))) // TODO exception
            HAMMER_ERROR("Integer overflow in subtraction.");
        return result;
    }

    f64 operator()(f64 a, f64 b) { return a - b; }
};

struct mul_op {
    i64 operator()(i64 a, i64 b) {
        i64 result;
        if (HAMMER_UNLIKELY(!checked_mul(a, b, result)))
            HAMMER_ERROR("Integer overflow in multiplication.");
        return result;
    }

    f64 operator()(f64 a, f64 b) { return a * b; }
};

struct div_op {
    i64 operator()(i64 a, i64 b) {
        if (HAMMER_UNLIKELY(b == 0))
            HAMMER_ERROR("Integer division by zero.");
        if (HAMMER_UNLIKELY(a == std::numeric_limits<i64>::min()) && (b == -1))
            HAMMER_ERROR("Integer overflow in division.");
        return a / b;
    }

    f64 operator()(f64 a, f64 b) { return a / b; }
};

struct mod_op {
    i64 operator()(i64 a, i64 b) {
        if (HAMMER_UNLIKELY(b == 0))
            HAMMER_ERROR("Integer modulus by zero.");
        if (HAMMER_UNLIKELY(a == std::numeric_limits<i64>::min()) && (b == -1))
            HAMMER_ERROR("Integer overflow in modulus.");
        return a % b;
    }

    f64 operator()(f64 a, f64 b) { return std::fmod(a, b); }
};

static i64 to_integer(Handle<Value> v) {
    switch (v->type()) {
    case ValueType::Integer:
        return v->as<Integer>().value();
    case ValueType::Float:
        return v->as<Float>().value();

    default:
        // TODO exception
        HAMMER_ERROR("Cannot convert value of type {} to integer.",
            to_string(v->type()));
    }
}

static f64 to_float(Handle<Value> v) {
    switch (v->type()) {
    case ValueType::Integer:
        return v->as<Integer>().value();
    case ValueType::Float:
        return v->as<Float>().value();

    default:
        // TODO exception
        HAMMER_ERROR(
            "Cannot convert value of type {} to float.", to_string(v->type()));
    }
}

template<typename Operation>
static Value binary_op(
    Context& ctx, Handle<Value> left, Handle<Value> right, Operation&& op) {
    if (left->is<Float>() || right->is<Float>()) {
        f64 a = left->is<Float>() ? left->as<Float>().value() : to_float(left);
        f64 b = right->is<Float>() ? right->as<Float>().value()
                                   : to_float(right);
        return Float::make(ctx, op(a, b));
    } else {
        i64 a = left->is<Integer>() ? left->as<Integer>().value()
                                    : to_integer(left);
        i64 b = right->is<Integer>() ? right->as<Integer>().value()
                                     : to_integer(right);
        return Integer::make(ctx, op(a, b)); // TODO small ints
    }
}

static bool truthy(Handle<Value> v) {
    switch (v->type()) {
    case ValueType::Null:
        return false;
    case ValueType::Undefined:
        HAMMER_ERROR("Undefined value used in boolean context.");
    case ValueType::Boolean:
        return v->as<Boolean>().value();
    default:
        return true;
    }
}

static Value bitwise_not(Context& ctx, Handle<Value> v) {
    if (HAMMER_UNLIKELY(!v->is<Integer>()))
        HAMMER_ERROR(
            "Invalid operand type for bitwise not: {}.", to_string(v->type()));
    return Integer::make(ctx, ~v->as<Integer>().value());
}

static void unary_plus(Handle<Value> v) {
    switch (v->type()) {
    case ValueType::Integer:
    case ValueType::Float:
        return;
    default:
        HAMMER_ERROR(
            "Invalid operand type for unary plus: {}.", to_string(v->type()));
    }
}

static Value unary_minus(Context& ctx, Handle<Value> v) {
    switch (v->type()) {
    case ValueType::Integer: {
        i64 iv = v->as<Integer>().value();
        if (HAMMER_UNLIKELY(iv == -1))
            HAMMER_ERROR("Integer overflow in unary minus.");
        return Integer::make(ctx, -iv);
    }
    case ValueType::Float:
        return Float::make(ctx, -v->as<Float>().value());
    default:
        HAMMER_ERROR(
            "Invalid operand type for unary minus: {}.", to_string(v->type()));
    }
}

static int compare(Handle<Value> a, Handle<Value> b) {
    if (a->is_null()) {
        if (b->is_null())
            return 0;
        return -1;
    }
    if (b->is_null())
        return 1;

    auto cmp = [](auto lhs, auto rhs) {
        if (lhs > rhs)
            return 1;
        else if (lhs < rhs)
            return -1;
        return 0;
    };

    // TODO comparison between integer and float correct?
    switch (a->type()) {
    case ValueType::Integer: {
        switch (b->type()) {
        case ValueType::Integer:
            return cmp(a->as<Integer>().value(), b->as<Integer>().value());
        case ValueType::Float:
            return cmp(a->as<Integer>().value(), b->as<Float>().value());
        default:
            break;
        }
        break;
    }
    case ValueType::Float: {
        switch (b->type()) {
        case ValueType::Integer:
            return cmp(a->as<Float>().value(), b->as<Integer>().value());
        case ValueType::Float:
            return cmp(a->as<Float>().value(), b->as<Float>().value());
        default:
            break;
        }
        break;
    }
    default:
        break;
    }

    HAMMER_ERROR("Comparisons are not defined for types {} and {}.",
        to_string(a->type()), to_string(b->type()));
}

Context::Context() {
    true_ = Boolean::make(*this, true);
    false_ = Boolean::make(*this, false);
    undefined_ = Undefined::make(*this);
    stop_iteration_ = SpecialValue::make(*this, "STOP_ITERATION");
    interned_strings_ = HashTable::make(*this);
    modules_ = HashTable::make(*this);
}

Context::~Context() {}

bool Context::add_module(Handle<Module> module) {
    HAMMER_CHECK(!module->is_null(), "Module must not be null.");
    HAMMER_CHECK(!module->name().is_null(), "Module must have a valid name.");

    if (modules_.contains(module->name())) {
        return false;
    }

    Root<String> name(*this, module->name());
    name.set(intern_string(name));

    modules_.set(*this, name.handle(), module);
    return true;
}

bool Context::find_module(Handle<String> name, MutableHandle<Module> module) {
    if (auto opt = modules_.get(name)) {
        module.set(opt->as_strict<Module>());
        return true;
    }
    return false;
}

String Context::intern_string(Handle<String> str) {
    HAMMER_CHECK(!str->is_null(), "String must not be null.");

    if (str->interned())
        return str;

    Root interned(*this, str.get());
    intern_impl(interned.mut_handle(), {});
    return interned;
}

String Context::get_interned_string(std::string_view view) {
    // Improvement: we can avoid constructing the temporary string by introducing
    // a find_equivalent(hash, compare, ...) function to the table. Care must be taken
    // to use the same hash function in that case.
    Root str(*this, String::make(*this, view));
    return intern_string(str);
}

Symbol Context::get_symbol(Handle<String> str) {
    Root<String> interned_str(*this, str);
    Root<Symbol> symbol(*this);

    intern_impl(interned_str.mut_handle(), symbol.mut_handle());
    return symbol;
}

void Context::intern_impl(MutableHandle<String> str,
    std::optional<MutableHandle<Symbol>> assoc_symbol) {

    {
        Root<Value> existing_string(*this);
        Root<Value> existing_value(*this);
        if (interned_strings_.find(str, existing_string.mut_handle(),
                existing_value.mut_handle())) {
            HAMMER_ASSERT(
                existing_string->is<String>(), "Key must be a string.");
            HAMMER_ASSERT(existing_string->as<String>().interned(),
                "Existing string must have been interned.");
            HAMMER_ASSERT(
                existing_value->is<Symbol>(), "Value must be a symbol.");

            if (assoc_symbol) {
                assoc_symbol->set(existing_value->as<Symbol>());
            }
            return str.set(existing_string->as<String>());
        }
    }

    // TODO: I'm being lazy here, create a symbol right away. This could be delayed only
    // for those instances where a symbol is actually needed.
    Root symbol(*this, Symbol::make(*this, str));
    interned_strings_.set(*this, str, symbol.handle());
    str->interned(true);

    if (assoc_symbol) {
        assoc_symbol->set(symbol);
    }
}

Value Context::run(Handle<Function> fn) {
    HAMMER_ASSERT(!fn->is_null(), "Invalid function.");
    HAMMER_ASSERT(current_.is_null(), "Already executing a coroutine.");

    HAMMER_CHECK(fn->tmpl().params() == 0,
        "Can only invoke nullary functions right now.");

    {
        Root stack(*this, CoroutineStack::make(*this, default_stack_size));
        Root name(*this, String::make(*this, "Coro-1"));
        Root coro(*this, Coroutine::make(*this, name, stack));

        bool ok = true;
        ok &= stack->push_value(fn);
        ok &= stack->push_frame(fn->tmpl(), fn->closure());
        HAMMER_CHECK(ok, "Failed to create initial function frame.");

        current_ = coro;
    }

    Value v = run_until_complete(Handle<Coroutine>::from_slot(&current_));
    current_ = Coroutine();
    return v;
}

Value Context::run_until_complete(Handle<Coroutine> coro) {
    HAMMER_ASSERT(!coro->is_null(), "Invalid coroutine.");
    HAMMER_ASSERT_NOT_NULL(coro->stack().top_frame());
    HAMMER_ASSERT(coro->state() == CoroutineState::Ready,
        "Cannot run coroutines with this state.");

    while (coro->stack().top_frame() != nullptr) {
        run_frame(coro);
    }

    HAMMER_ASSERT(coro->stack().top_value_count() == 1,
        "Must have left one value on the stack.");
    HAMMER_WRITE_MEMBER(*this, *coro, result,
        Handle<Value>::from_slot(coro->stack().top_value()));
    coro->state(CoroutineState::Done);
    return coro->result();
}

void Context::run_frame(Handle<Coroutine> coro) {
    HAMMER_ASSERT(!coro->is_null(), "Invalid coroutine.");

    CoroutineStack stack = coro->stack();
    CoroutineStack::Frame* frame = stack.top_frame();
    Span<const byte> code = frame->tmpl.code().view();

    auto grow_stack = [&]() {
        u32 next_size;
        if (HAMMER_UNLIKELY(
                !checked_mul<u32>(stack.stack_size(), 2, next_size)))
            HAMMER_ERROR("Overflow in stack size computation.");

        if (HAMMER_UNLIKELY(next_size > max_stack_size))
            HAMMER_ERROR("Stack overflow.");

        Root old_stack(*this, coro->stack());
        Root new_stack(
            *this, CoroutineStack::grow(*this, old_stack, next_size));

        HAMMER_WRITE_MEMBER(*this, *coro, stack, new_stack);
        stack = coro->stack();
        frame = coro->stack().top_frame();
    };

    auto push_value = [&](Value v) {
        if (HAMMER_LIKELY(stack.push_value(v)))
            return;

        grow_stack();
        [[maybe_unused]] bool ok = stack.push_value(v);
        HAMMER_ASSERT(ok, "Failed to push value after stack growth.");
    };

    auto push_frame = [&](Handle<FunctionTemplate> tmpl,
                          Handle<ClosureContext> closure) {
        if (HAMMER_LIKELY(stack.push_frame(tmpl.get(), closure.get())))
            return;

        grow_stack();
        [[maybe_unused]] bool ok = stack.push_frame(tmpl.get(), closure.get());
        HAMMER_ASSERT(ok, "Failed to push frame after stack growth.");
    };

    [[maybe_unused]] auto readable = [&]() { return code.end() - frame->pc; };

    auto read_op = [&] {
        // TODO static verify
        HAMMER_ASSERT(readable() >= 1, "Not enough available bytes.");

        u8 opcode = *frame->pc++;
        HAMMER_ASSERT(valid_opcode(opcode), "Invalid opcode.");
        return static_cast<Opcode>(opcode);
    };

    auto read_i64 = [&] {
        // TODO static verify
        HAMMER_ASSERT(readable() >= 8, "Not enough available bytes.");
        return static_cast<i64>(read_big_endian<u64>(frame->pc));
    };

    auto read_f64 = [&] {
        // TODO static verify
        HAMMER_ASSERT(readable() >= 8, "Not enough available bytes.");
        // FIXME float serialization in some helper function, see also compiler/binary.hpp
        static_assert(sizeof(f64) == sizeof(u64));
        u64 as_u64 = read_big_endian<u64>(frame->pc);
        f64 d;
        std::memcpy(&d, &as_u64, sizeof(f64));
        return d;
    };

    auto read_u32 = [&] {
        // TODO static verify
        HAMMER_ASSERT(readable() >= 4, "Not enough available bytes.");
        return read_big_endian<u32>(frame->pc);
    };

    while (1) {
        // TODO static verify
        if (HAMMER_UNLIKELY(frame->pc == code.end())) {
            HAMMER_ERROR(
                "Invalid program counter: end of code reached "
                "without return from function.");
        }

        Opcode op = read_op();
        // fmt::print("Running op {}\n", to_string(op));

        switch (op) {
        case Opcode::Invalid:
            HAMMER_ERROR("Logic error.");
            break;
        case Opcode::LoadNull:
            push_value(Value::null());
            break;
        case Opcode::LoadFalse:
            push_value(false_);
            break;
        case Opcode::LoadTrue:
            push_value(true_);
            break;
        case Opcode::LoadInt: {
            const i64 value = read_i64();
            // FIXME small integers
            push_value(Integer::make(*this, value));
            break;
        }
        case Opcode::LoadFloat: {
            const f64 value = read_f64();
            push_value(Float::make(*this, value));
            break;
        }
        case Opcode::LoadParam: {
            const u32 index = read_u32();
            HAMMER_ASSERT(
                index < frame->args, "Parameter index out of bounds.");
            push_value(stack.args()[index]);
            break;
        }
        case Opcode::StoreParam: {
            const u32 index = read_u32();
            HAMMER_ASSERT(
                index < frame->args, "Parameter index out of bounds.");

            // TODO static verify possible?
            stack.args()[index] = *stack.top_value();
            stack.pop_value();
            break;
        }
        case Opcode::LoadLocal: {
            const u32 index = read_u32();
            HAMMER_ASSERT(index < frame->locals, "Local index out of bounds.");

            Value local = stack.locals()[index];
            if (HAMMER_UNLIKELY(undefined_.same(local))) {
                HAMMER_ERROR("Local value is undefined.");
            }

            push_value(local);
            break;
        }
        case Opcode::StoreLocal: {
            const u32 index = read_u32();
            HAMMER_ASSERT(index < frame->locals, "Local index out of bounds.");
            stack.locals()[index] = *stack.top_value();
            stack.pop_value();
            break;
        }
        case Opcode::LoadClosure: {
            HAMMER_CHECK(
                !frame->closure.is_null(), "Function does not have a closure.");

            push_value(frame->closure);
            break;
        }
        case Opcode::LoadContext: {
            const u32 level = read_u32();
            const u32 index = read_u32();

            Value* top = stack.top_value();
            Value context_value = *top;
            HAMMER_CHECK(context_value.is<ClosureContext>(),
                "The value is not a closure context.");

            ClosureContext context = context_value.as<ClosureContext>();
            if (index != 0)
                context = context.parent(level);

            Value v = context.get(index);
            if (HAMMER_UNLIKELY(undefined_.same(v))) {
                HAMMER_ERROR("Closure variable is undefined.");
            }

            *top = v;
            break;
        }
        case Opcode::StoreContext: {
            const u32 level = read_u32();
            const u32 index = read_u32();

            Value context_value = *stack.top_value(1);
            HAMMER_CHECK(context_value.is<ClosureContext>(),
                "The value is not a closure context.");

            Value value = *stack.top_value(0);

            ClosureContext context = context_value.as<ClosureContext>();
            if (index != 0)
                context = context.parent(level);

            HAMMER_WRITE_INDEX(*this, context, index, value);
            stack.pop_values(2);
            break;
        }
        case Opcode::LoadMember: {
            const u32 member_index = read_u32();
            Tuple members = frame->tmpl.module().members();
            HAMMER_CHECK(members && member_index < members.size(),
                "Member index out of bounds.");

            Value symbol = members.get(member_index);
            HAMMER_CHECK(symbol.is<Symbol>(),
                "The module member at index {} must be a symbol.",
                member_index);

            Value* obj = stack.top_value();
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
                stack.top_value(1));
            switch (obj->type()) {
            case ValueType::Array: {
                Handle<Array> array = obj.cast<Array>();
                Handle<Value> index = Handle<Value>::from_slot(
                    stack.top_value(0));
                HAMMER_CHECK(
                    index->is<Integer>(), "Array index must be an integer.");
                i64 raw_index = index.cast<Integer>()->value();
                HAMMER_CHECK(raw_index >= 0 && u64(raw_index) < array->size(),
                    "Invalid index {} into array of size {}.", raw_index,
                    array->size());
                obj.set(array->get(static_cast<size_t>(raw_index)));
                stack.pop_value();
                break;
            }
            case ValueType::Tuple: {
                Handle<Tuple> tuple = obj.cast<Tuple>();
                Handle<Value> index = Handle<Value>::from_slot(
                    stack.top_value(0));
                HAMMER_CHECK(
                    index->is<Integer>(), "Tuple index must be an integer.");
                i64 raw_index = index.cast<Integer>()->value();
                HAMMER_CHECK(raw_index >= 0 && u64(raw_index) < tuple->size(),
                    "Invalid index {} into tuple of size {}.", raw_index,
                    tuple->size());
                obj.set(tuple->get(static_cast<size_t>(raw_index)));
                stack.pop_value();
                break;
            }
            case ValueType::HashTable: {
                Handle<HashTable> table = obj.cast<HashTable>();
                Handle<Value> key = Handle<Value>::from_slot(
                    stack.top_value(0));
                if (auto found = table->get(key.get())) {
                    obj.set(*found);
                } else {
                    obj.set(Value::null());
                }
                stack.pop_value();
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
                stack.top_value(2));
            switch (obj->type()) {
            case ValueType::Array: {
                Handle<Array> array = obj.cast<Array>();
                Handle<Value> index = Handle<Value>::from_slot(
                    stack.top_value(1));
                Handle<Value> value = Handle<Value>::from_slot(
                    stack.top_value(0));

                HAMMER_CHECK(
                    index->is<Integer>(), "Array index must be an integer.");
                i64 raw_index = index.cast<Integer>()->value();
                HAMMER_CHECK(raw_index >= 0 && u64(raw_index) < array->size(),
                    "Invalid index {} into array of size {}.", raw_index,
                    array->size());
                array->set(*this, static_cast<size_t>(raw_index), value);
                stack.pop_values(3);
                break;
            }
            case ValueType::Tuple: {
                Handle<Tuple> tuple = obj.cast<Tuple>();
                Handle<Value> index = Handle<Value>::from_slot(
                    stack.top_value(1));
                Handle<Value> value = Handle<Value>::from_slot(
                    stack.top_value(0));

                HAMMER_CHECK(
                    index->is<Integer>(), "Tuple index must be an integer.");
                i64 raw_index = index.cast<Integer>()->value();
                HAMMER_CHECK(raw_index >= 0 && u64(raw_index) < tuple->size(),
                    "Invalid index {} into tuple of size {}.", raw_index,
                    tuple->size());
                tuple->set(write_barrier(), static_cast<size_t>(raw_index),
                    value.get());
                stack.pop_values(3);
                break;
            }
            case ValueType::HashTable: {
                Handle<HashTable> table = obj.cast<HashTable>();
                Handle<Value> key = Handle<Value>::from_slot(
                    stack.top_value(1));
                Handle<Value> value = Handle<Value>::from_slot(
                    stack.top_value(0));
                table->set(*this, key, value);
                stack.pop_values(3);
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
            Tuple members = frame->tmpl.module().members();
            // TODO static verify
            HAMMER_ASSERT(members && index < members.size(),
                "Module member index out of bounds.");
            push_value(members.get(index));
            break;
        }
        case Opcode::StoreModule: {
            const u32 index = read_u32();
            Tuple members = frame->tmpl.module().members();
            // TODO static verify
            HAMMER_ASSERT(members && index < members.size(),
                "Module member index out of bounds.");
            HAMMER_WRITE_INDEX(*this, members, index, *stack.top_value());
            break;
        }
        case Opcode::Dup:
            push_value(*stack.top_value());
            break;
        case Opcode::Pop:
            stack.pop_value();
            break;
        case Opcode::Rot2: {
            Value tmp = *stack.top_value(0);
            *stack.top_value(0) = *stack.top_value(1);
            *stack.top_value(1) = tmp;
            break;
        }
        case Opcode::Rot3: {
            Value tmp = *stack.top_value(0);
            *stack.top_value(0) = *stack.top_value(1);
            *stack.top_value(1) = *stack.top_value(2);
            *stack.top_value(2) = tmp;
            break;
        }
        case Opcode::Rot4: {
            Value tmp = *stack.top_value(0);
            *stack.top_value(0) = *stack.top_value(1);
            *stack.top_value(1) = *stack.top_value(2);
            *stack.top_value(2) = *stack.top_value(3);
            *stack.top_value(3) = tmp;
            break;
        }
        case Opcode::Add: {
            auto a = MutableHandle<Value>::from_slot(stack.top_value(1));
            auto b = Handle<Value>::from_slot(stack.top_value(0));
            a.set(binary_op(*this, a, b, add_op()));
            stack.pop_value();
            break;
        }
        case Opcode::Sub: {
            auto a = MutableHandle<Value>::from_slot(stack.top_value(1));
            auto b = Handle<Value>::from_slot(stack.top_value(0));
            a.set(binary_op(*this, a, b, sub_op()));
            stack.pop_value();
            break;
        }
        case Opcode::Mul: {
            auto a = MutableHandle<Value>::from_slot(stack.top_value(1));
            auto b = Handle<Value>::from_slot(stack.top_value(0));
            a.set(binary_op(*this, a, b, mul_op()));
            stack.pop_value();
            break;
        }
        case Opcode::Div: {
            auto a = MutableHandle<Value>::from_slot(stack.top_value(1));
            auto b = Handle<Value>::from_slot(stack.top_value(0));
            a.set(binary_op(*this, a, b, div_op()));
            stack.pop_value();
            break;
        }
        case Opcode::Mod: {
            auto a = MutableHandle<Value>::from_slot(stack.top_value(1));
            auto b = Handle<Value>::from_slot(stack.top_value(0));
            a.set(binary_op(*this, a, b, mod_op()));
            stack.pop_value();
            break;
        }
        case Opcode::Pow: {
            HAMMER_ERROR("Power not implemented yet."); // FIXME
        }
        case Opcode::LNot: {
            auto a = MutableHandle<Value>::from_slot(stack.top_value());
            a.set(truthy(a) ? false_ : true_);
            break;
        }
        case Opcode::BNot: {
            auto a = MutableHandle<Value>::from_slot(stack.top_value());
            a.set(bitwise_not(*this, a));
            break;
        }
        case Opcode::UPos: {
            // Just check its type, unary plus is a noop otherwise.
            unary_plus(Handle<Value>::from_slot(stack.top_value()));
            break;
        }
        case Opcode::UNeg: {
            auto a = MutableHandle<Value>::from_slot(stack.top_value());
            a.set(unary_minus(*this, a));
            break;
        }
        case Opcode::Gt: {
            auto a = MutableHandle<Value>::from_slot(stack.top_value(1));
            auto b = Handle<Value>::from_slot(stack.top_value(0));
            a.set(compare(a, b) > 0 ? true_ : false_);
            stack.pop_value();
            break;
        }
        case Opcode::Gte: {
            auto a = MutableHandle<Value>::from_slot(stack.top_value(1));
            auto b = Handle<Value>::from_slot(stack.top_value(0));
            a.set(compare(a, b) >= 0 ? true_ : false_);
            stack.pop_value();
            break;
        }
        case Opcode::Lt: {
            auto a = MutableHandle<Value>::from_slot(stack.top_value(1));
            auto b = Handle<Value>::from_slot(stack.top_value(0));
            a.set(compare(a, b) < 0 ? true_ : false_);
            stack.pop_value();
            break;
        }
        case Opcode::Lte: {
            auto a = MutableHandle<Value>::from_slot(stack.top_value(1));
            auto b = Handle<Value>::from_slot(stack.top_value(0));
            a.set(compare(a, b) <= 0 ? true_ : false_);
            stack.pop_value();
            break;
        }
        case Opcode::Eq: {
            auto a = MutableHandle<Value>::from_slot(stack.top_value(1));
            auto b = Handle<Value>::from_slot(stack.top_value(0));
            a.set(equal(a, b) ? true_ : false_);
            stack.pop_value();
            break;
        }
        case Opcode::NEq: {
            auto a = MutableHandle<Value>::from_slot(stack.top_value(1));
            auto b = Handle<Value>::from_slot(stack.top_value(0));
            a.set(equal(a, b) ? false_ : true_);
            stack.pop_value();
            break;
        }
        case Opcode::MkArray: {
            const u32 size = read_u32();
            const Span<const Value> values = stack.top_values(size);

            auto array = reg<0, Array>();
            array.set(Array::make(*this, values));
            stack.pop_values(size);
            push_value(array.get());
            break;
        }
        case Opcode::MkTuple: {
            const u32 size = read_u32();
            const Span<const Value> values = stack.top_values(size);

            auto tuple = reg<0, Tuple>();
            tuple.set(Tuple::make(*this, values));
            stack.pop_values(size);
            push_value(tuple.get());
            break;
        }
        case Opcode::MkMap: {
            // FIXME overflow protection
            const u32 pairs = read_u32();
            const u32 kv_count = pairs * 2;
            const Span<Value> kvs = stack.top_values(kv_count);

            auto map = reg<0, HashTable>();
            map.set(HashTable::make(*this, pairs));
            for (u32 i = 0; i < kv_count; i += 2) {
                auto key = Handle<Value>::from_slot(kvs.data() + i);
                auto value = Handle<Value>::from_slot(kvs.data() + i + 1);
                map->set(*this, key, value);
            }

            stack.pop_values(kv_count);
            stack.push_value(map.get());
            break;
        }
        case Opcode::MkContext: {
            const u32 size = read_u32();

            auto context_value = MutableHandle<Value>::from_slot(
                stack.top_value());
            HAMMER_CHECK(
                context_value->is_null() || context_value->is<ClosureContext>(),
                "Parent of closure context must be null or a another closure "
                "context.");
            context_value.set(ClosureContext::make(
                *this, size, context_value.cast<ClosureContext>()));
            break;
        }
        case Opcode::MkClosure: {
            auto tmpl_value = MutableHandle<Value>::from_slot(
                stack.top_value(1));
            HAMMER_CHECK(tmpl_value->is<FunctionTemplate>(),
                "First argument to MkClosure must be a function template.");

            auto closure_value = Handle<Value>::from_slot(stack.top_value(0));
            HAMMER_CHECK(
                closure_value->is_null() || closure_value->is<ClosureContext>(),
                "Second argument to MkClosure must be null or a closure "
                "context.");

            tmpl_value.set(Function::make(*this,
                tmpl_value.strict_cast<FunctionTemplate>(),
                closure_value.cast<ClosureContext>()));
            stack.pop_value();
            break;
        }
        case Opcode::Jmp: {
            const u32 offset = read_u32();
            // TODO static verify
            HAMMER_ASSERT(offset < code.size(), "Invalid jump destination.");
            frame->pc = code.data() + offset;
            break;
        }
        case Opcode::JmpTrue: {
            const u32 offset = read_u32();
            // TODO static verify
            HAMMER_ASSERT(offset < code.size(), "Invalid jump destination.");
            if (truthy(Handle<Value>::from_slot(stack.top_value()))) {
                frame->pc = code.data() + offset;
            }
            break;
        }
        case Opcode::JmpTruePop: {
            const u32 offset = read_u32();
            // TODO static verify
            HAMMER_ASSERT(offset < code.size(), "Invalid jump destination.");
            if (truthy(Handle<Value>::from_slot(stack.top_value()))) {
                frame->pc = code.data() + offset;
            }
            stack.pop_value();
            break;
        }
        case Opcode::JmpFalse: {
            const u32 offset = read_u32();
            // TODO static verify
            HAMMER_ASSERT(offset < code.size(), "Invalid jump destination.");
            if (!truthy(Handle<Value>::from_slot(stack.top_value()))) {
                frame->pc = code.data() + offset;
            }
            break;
        }
        case Opcode::JmpFalsePop: {
            const u32 offset = read_u32();
            // TODO static verify
            HAMMER_ASSERT(offset < code.size(), "Invalid jump destination.");
            if (!truthy(Handle<Value>::from_slot(stack.top_value()))) {
                frame->pc = code.data() + offset;
            }
            stack.pop_value();
            break;
        }
        case Opcode::Call: {
            const u32 args = read_u32();
            Value* funcval = stack.top_value(args);

            if (funcval->is<Function>()) {
                auto tmpl = reg<0>(funcval->as<Function>().tmpl());
                auto closure = reg<1>(funcval->as<Function>().closure());
                if (tmpl->params() != args) {
                    HAMMER_ERROR(
                        "Invalid number of function arguments (need {}, got "
                        "{}).",
                        args, tmpl->params());
                }

                push_frame(tmpl, closure);
                return;
            } else if (funcval->is<NativeFunction>()) {
                auto native = reg<0>(funcval->as<NativeFunction>());
                if (args < native->min_params()) {
                    HAMMER_ERROR(
                        "Invalid number of function arguments (need {}, got "
                        "{}).",
                        args, native->min_params());
                }

                *funcval = Value::null(); // Default for return value
                NativeFunction::Frame native_frame(*this,
                    stack.top_values(args),
                    MutableHandle<Value>::from_slot(funcval));
                native->function()(native_frame);
                stack.pop_values(args);
            } else {
                HAMMER_ERROR("Cannot call object of type {} as a function.",
                    to_string(funcval->type()));
            }
            break;
        }
        case Opcode::Ret: {
            const u32 args = frame->args;
            auto value = reg<0>(*stack.top_value());
            stack.pop_frame();
            stack.pop_values(args);     // Function arguments
            *stack.top_value() = value; // This was the function object
            return;
        }
        case Opcode::AssertFail: {
            // Expression that failed, as a string
            Value* expr = stack.top_value(1);
            // A human readable string (or null)
            Value* message = stack.top_value(0);

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

} // namespace hammer::vm
