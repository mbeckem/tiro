#include "hammer/vm/context.hpp"

#include "hammer/compiler/opcodes.hpp"
#include "hammer/compiler/output.hpp"
#include "hammer/compiler/string_table.hpp"
#include "hammer/core/byte_order.hpp"
#include "hammer/core/defs.hpp"
#include "hammer/core/overloaded.hpp"
#include "hammer/vm/handles.hpp"
#include "hammer/vm/object.hpp"

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

    double operator()(double a, double b) { return a + b; }
};

struct sub_op {
    i64 operator()(i64 a, i64 b) {
        i64 result;
        if (HAMMER_UNLIKELY(!checked_sub(a, b, result))) // TODO exception
            HAMMER_ERROR("Integer overflow in subtraction.");
        return result;
    }

    double operator()(double a, double b) { return a - b; }
};

struct mul_op {
    i64 operator()(i64 a, i64 b) {
        i64 result;
        if (HAMMER_UNLIKELY(!checked_mul(a, b, result)))
            HAMMER_ERROR("Integer overflow in multiplication.");
        return result;
    }

    double operator()(double a, double b) { return a * b; }
};

struct div_op {
    i64 operator()(i64 a, i64 b) {
        if (HAMMER_UNLIKELY(b == 0))
            HAMMER_ERROR("Integer division by zero.");
        if (HAMMER_UNLIKELY(a == std::numeric_limits<i64>::min()) && (b == -1))
            HAMMER_ERROR("Integer overflow in division.");
        return a / b;
    }

    double operator()(double a, double b) { return a / b; }
};

struct mod_op {
    i64 operator()(i64 a, i64 b) {
        if (HAMMER_UNLIKELY(b == 0))
            HAMMER_ERROR("Integer modulus by zero.");
        if (HAMMER_UNLIKELY(a == std::numeric_limits<i64>::min()) && (b == -1))
            HAMMER_ERROR("Integer overflow in modulus.");
        return a % b;
    }

    double operator()(double a, double b) { return std::fmod(a, b); }
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

static double to_float(Handle<Value> v) {
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
        double a = left->is<Float>() ? left->as<Float>().value()
                                     : to_float(left);
        double b = right->is<Float>() ? right->as<Float>().value()
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

// TODO think about float / integer equality.
static bool equal(Handle<Value> a, Handle<Value> b) {
    switch (a->type()) {
    case ValueType::Boolean: {
        switch (b->type()) {
        case ValueType::Boolean:
            return a->as<Boolean>().value() == b->as<Boolean>().value();
        default:
            return false;
        }
    }
    case ValueType::Integer: {
        switch (b->type()) {
        case ValueType::Integer:
            return a->as<Integer>().value() == b->as<Integer>().value();
        case ValueType::Float:
            return a->as<Integer>().value()
                   == b->as<Float>().value(); // TODO correct?
        default:
            return false;
        }
    }
    case ValueType::Float: {
        switch (b->type()) {
        case ValueType::Integer:
            return a->as<Float>().value()
                   == b->as<Integer>().value(); // TODO correct?
        case ValueType::Float:
            return a->as<Float>().value() == b->as<Float>().value();
        default:
            return false;
        }
    }
    case ValueType::String:
        return a->as<String>().view() == b->as<String>().view();

    default:
        // Reference semantics
        switch (b->type()) {
        case ValueType::Boolean:
        case ValueType::Integer:
        case ValueType::Float:
        case ValueType::String:
            return false;
        default:
            return a->heap_ptr() == b->heap_ptr();
        }
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
}

Context::~Context() {}

Module Context::load(
    const CompiledModule& compiled_module, const StringTable& strings) {
    Root module_name(
        *this, String::make(*this, strings.value(compiled_module.name)));
    Root module_members(
        *this, Array::make(*this, compiled_module.members.size()));
    Root module(*this, Module::make(*this, module_name, module_members));

    size_t index = 0;
    for (const auto& member_ptr : compiled_module.members) {
        Overloaded visitor{[&]([[maybe_unused]] const CompiledImport& i) {
                               // FIXME imports
                               HAMMER_ERROR("Imports not implemented yet.");
                           },
            [&](const CompiledFunction& f) {
                Root function_name(
                    *this, String::make(*this, strings.value(f.name)));
                Root literals(*this, Array::make(*this, f.literals.size()));

                Root tmpl(
                    *this, FunctionTemplate::make(*this, function_name, module,
                               literals, f.params, f.locals, f.code));
                Root func(*this, Function::make(*this, tmpl, Handle<Value>()));
                HAMMER_WRITE_INDEX(*this, *module_members, index, func);
            },
            [&](const CompiledOutput&) {
                HAMMER_ERROR("Invalid compiled value at module level.");
            }};
        visit_output(*member_ptr, visitor);

        ++index;
    }

    std::string std_name;
    {
        auto sv = strings.value(compiled_module.name);
        std_name = std::string(sv.begin(), sv.end());
    }

    if (auto pos = modules_.find(std_name); pos != modules_.end()) {
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

    auto push_frame = [&](FunctionTemplate tmpl, Value closure) {
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
        HAMMER_ASSERT(*frame->pc != 0
                          && *frame->pc <= static_cast<u8>(Opcode::LastOpcode),
            "Invalid opcode.");
        return static_cast<Opcode>(*frame->pc++);
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
        static_assert(sizeof(double) == sizeof(u64));
        u64 as_u64 = read_big_endian<u64>(frame->pc);
        double d;
        std::memcpy(&d, &as_u64, sizeof(double));
        return d;
    };

    auto read_u32 = [&] {
        // TODO static verify
        HAMMER_ASSERT(readable() >= 4, "Not enough available bytes.");
        return read_big_endian<u32>(frame->pc);
    };

    while (1) {
        // TODO static verify
        if (HAMMER_UNLIKELY(frame->pc == code.end()))
            HAMMER_ERROR(
                "Invalid program counter: end of code reached without return "
                "from "
                "function.");

        // TODO static verify

        Opcode op = read_op();
        // fmt::print("Running op {}\n", to_string(op));

        switch (op) {
        case Opcode::Invalid:
            HAMMER_ERROR("Logic error.");

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
            i64 value = read_i64();
            // FIXME small integers
            push_value(Integer::make(*this, value));
            break;
        }
        case Opcode::LoadFloat: {
            double value = read_f64();
            push_value(Float::make(*this, value));
            break;
        }
        case Opcode::LoadConst: {
            u32 index = read_u32();
            Array literals = frame->tmpl.literals();
            // TODO static verify
            HAMMER_ASSERT(
                literals && index < literals.size(), "Invalid constant index.");
            push_value(literals.get(index));
            break;
        }
        case Opcode::LoadParam: {
            u32 index = read_u32();
            HAMMER_ASSERT(
                index < frame->args, "Parameter index out of bounds.");
            push_value(stack.args()[index]);
            break;
        }
        case Opcode::StoreParam: {
            u32 index = read_u32();
            HAMMER_ASSERT(
                index < frame->args, "Parameter index out of bounds.");

            // TODO static verify possible?
            stack.args()[index] = *stack.top_value();
            stack.pop_value();
            break;
        }
        case Opcode::LoadLocal: {
            u32 index = read_u32();
            HAMMER_ASSERT(index < frame->locals, "Local index out of bounds.");
            push_value(stack.locals()[index]);
            break;
        }
        case Opcode::StoreLocal: {
            u32 index = read_u32();
            HAMMER_ASSERT(index < frame->locals, "Local index out of bounds.");
            stack.locals()[index] = *stack.top_value();
            stack.pop_value();
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
                "Invalid index {} into array of size{}.", index, array.size());

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
                "Invalid index {} into array of size{}.", index, array.size());

            HAMMER_WRITE_INDEX(*this, array, static_cast<size_t>(index), value);
            stack.pop_values(3);
            break;
        }
        case Opcode::LoadModule: {
            u32 index = read_u32();
            Array members = frame->tmpl.module().members();
            // TODO static verify
            HAMMER_ASSERT(members && index < members.size(),
                "Module member index out of bounds.");
            stack.push_value(members.get(index));
            break;
        }
        case Opcode::StoreModule: {
            u32 index = read_u32();
            Array members = frame->tmpl.module().members();
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
        case Opcode::LSh:
        case Opcode::RSh:
        case Opcode::BAnd:
        case Opcode::BOr:
        case Opcode::BXor:
            HAMMER_NOT_IMPLEMENTED(); // FIXME

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
        case Opcode::MkArray:
        case Opcode::MkMap:
        case Opcode::MkSet:
            HAMMER_NOT_IMPLEMENTED(); // FIXME

        case Opcode::Jmp: {
            u32 offset = read_u32();
            // TODO static verify
            HAMMER_ASSERT(offset < code.size(), "Invalid jump destination.");
            frame->pc = code.data() + offset;
            break;
        }
        case Opcode::JmpTrue: {
            u32 offset = read_u32();
            // TODO static verify
            HAMMER_ASSERT(offset < code.size(), "Invalid jump destination.");
            if (truthy(Handle<Value>::from_slot(stack.top_value()))) {
                frame->pc = code.data() + offset;
            }
            break;
        }
        case Opcode::JmpTruePop: {
            u32 offset = read_u32();
            // TODO static verify
            HAMMER_ASSERT(offset < code.size(), "Invalid jump destination.");
            if (truthy(Handle<Value>::from_slot(stack.top_value()))) {
                frame->pc = code.data() + offset;
            }
            stack.pop_value();
            break;
        }
        case Opcode::JmpFalse: {
            u32 offset = read_u32();
            // TODO static verify
            HAMMER_ASSERT(offset < code.size(), "Invalid jump destination.");
            if (!truthy(Handle<Value>::from_slot(stack.top_value()))) {
                frame->pc = code.data() + offset;
            }
            break;
        }
        case Opcode::JmpFalsePop: {
            u32 offset = read_u32();
            // TODO static verify
            HAMMER_ASSERT(offset < code.size(), "Invalid jump destination.");
            if (!truthy(Handle<Value>::from_slot(stack.top_value()))) {
                frame->pc = code.data() + offset;
            }
            stack.pop_value();
            break;
        }
        case Opcode::Call: {
            u32 args = read_u32();
            Value* funcval = stack.top_value(args);
            if (!funcval->is<Function>()) {
                HAMMER_ERROR("Cannot call object of type {} as a function.",
                    to_string(funcval->type()));
            }
            Function func = funcval->as<Function>();
            push_frame(func.tmpl(), func.closure());
            return;
        }
        case Opcode::Ret: {
            u32 args = frame->args;
            auto value = reg<0>(*stack.top_value());
            stack.pop_frame();
            stack.pop_values(args);     // Function arguments
            *stack.top_value() = value; // This was the function object
            return;
        }
        case Opcode::LoadEnv:
        case Opcode::StoreEnv:
        case Opcode::LoadMember:
        case Opcode::StoreMember:
        case Opcode::LoadGlobal:
            HAMMER_ERROR("Instruction not implemented: {}.", to_string(op));
            break;
        }
    }
}

} // namespace hammer::vm
