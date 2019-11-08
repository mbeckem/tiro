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

#include "hammer/vm/context.ipp"

#include <cmath>

namespace hammer::vm {

static constexpr u32 default_stack_size = 10 * 1024;
static constexpr u32 max_stack_size = 4 << 20;
static constexpr u32 max_module_size = 1 << 20; // # of members

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
}

Context::~Context() {}

Module Context::load(
    const CompiledModule& compiled_module, const StringTable& strings) {

    HAMMER_CHECK(compiled_module.name.valid(),
        "Module definition without a valid module name.");
    HAMMER_CHECK(compiled_module.members.size() < max_module_size,
        "Module definition is too large.");

    Root module_name(
        *this, String::make(*this, strings.value(compiled_module.name)));
    Root module_members(
        *this, Tuple::make(*this, compiled_module.members.size()));
    Root module(*this, Module::make(*this, module_name, module_members));

    u32 index = 0;
    for (const ModuleItem& member : compiled_module.members) {
        Overloaded visitor{//
            [&](const ModuleItem::Function& item) -> Value {
                FunctionDescriptor& f = *item.value;

                // FIXME name interned
                Root function_name(
                    *this, String::make(*this,
                               f.name ? strings.value(f.name) : "<UNNAMED>"));

                Root tmpl(*this, FunctionTemplate::make(*this, function_name,
                                     module, f.params, f.locals, f.code));
                if (f.type == FunctionDescriptor::TEMPLATE)
                    return tmpl;

                Root func(*this,
                    Function::make(*this, tmpl, Handle<ClosureContext>()));
                return func;
            },
            [&](const ModuleItem::Integer& item) -> Value {
                return Integer::make(*this, item.value);
            },
            [&](const ModuleItem::Float& item) -> Value {
                return Integer::make(*this, item.value);
            },
            [&](const ModuleItem::String& item) -> Value {
                // FIXME intern strings
                HAMMER_CHECK(
                    item.value.valid(), "Invalid string in module definition.");
                return String::make(*this, strings.value(item.value));
            },
            [&](auto &&) -> Value {
                HAMMER_ERROR("Unsupported module member of type {}.",
                    to_string(member.which()));
            }};

        Root value(*this, visit(member, visitor));
        HAMMER_WRITE_INDEX(*this, *module_members, index, value);
        ++index;
    }

    std::string std_name;
    {
        auto sv = strings.value(compiled_module.name);
        std_name = std::string(sv.begin(), sv.end());
    }

    if (auto pos = modules_.find(std_name); pos != modules_.end()) {
        // TODO not an internal error!
        HAMMER_ERROR("A module with that name has already been defined.");
    }

    modules_.emplace(std_name, module);
    return module;
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

    auto push_frame = [&](FunctionTemplate tmpl, ClosureContext closure) {
        if (HAMMER_LIKELY(stack.push_frame(tmpl, closure)))
            return;

        grow_stack();
        [[maybe_unused]] bool ok = stack.push_frame(tmpl, closure);
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
        case Opcode::LoadIndex: {
            Value array_value = *stack.top_value(1);
            HAMMER_CHECK(array_value.is<Array>(),
                "The value is not an array."); // TODO nicer

            Value index_value = *stack.top_value(0);
            HAMMER_CHECK(index_value.is<Integer>(),
                "The value is not an integer."); // TODO nicer

            Array array = array_value.as<Array>();
            i64 index = index_value.as<Integer>().value();
            HAMMER_CHECK(index >= 0 && u64(index) < array.size(),
                "Invalid index {} into array of size {}.", index, array.size());

            *stack.top_value(1) = array.get(static_cast<size_t>(index));
            stack.pop_value();
            break;
        }
        case Opcode::StoreIndex: {
            Value array_value = *stack.top_value(2);
            Value index_value = *stack.top_value(1);
            Value value = *stack.top_value(0);

            HAMMER_CHECK(array_value.is<Array>(),
                "The value is not an array."); // TODO nicer
            HAMMER_CHECK(index_value.is<Integer>(),
                "The value is not an integer."); // TODO nicer

            Array array = array_value.as<Array>();
            i64 index = index_value.as<Integer>().value();
            HAMMER_CHECK(index >= 0 && u64(index) < array.size(),
                "Invalid index {} into array of size {}.", index, array.size());

            array.set(*this, static_cast<size_t>(index), value);
            stack.pop_values(3);
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
            if (!funcval->is<Function>()) {
                HAMMER_ERROR("Cannot call object of type {} as a function.",
                    to_string(funcval->type()));
            }

            Function func = funcval->as<Function>();
            if (func.tmpl().params() != args) {
                HAMMER_ERROR(
                    "Invalid number of function arguments (need {}, got {}).",
                    args, func.tmpl().params());
            }

            push_frame(func.tmpl(), func.closure());
            return;
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
            [[maybe_unused]] Value* expr = stack.top_value(1);
            // A human readable string (or null)
            [[maybe_unused]] Value* message = stack.top_value(0);
            // TODO Format a message.
            HAMMER_ERROR("Assertion failed.");
            break;
        }

        case Opcode::LSh:
        case Opcode::RSh:
        case Opcode::BAnd:
        case Opcode::BOr:
        case Opcode::BXor:
        case Opcode::MkSet:
        case Opcode::LoadMember:
        case Opcode::StoreMember:
        case Opcode::LoadGlobal:
            HAMMER_ERROR("Instruction not implemented: {}.", to_string(op));
            break;
        }
    }
}

} // namespace hammer::vm
