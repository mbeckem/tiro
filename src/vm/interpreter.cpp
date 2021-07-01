#include "vm/interpreter.hpp"

#include "bytecode/op.hpp"
#include "common/adt/function_ref.hpp"
#include "common/memory/byte_order.hpp"
#include "vm/context.hpp"
#include "vm/context.ipp"
#include "vm/math.hpp"
#include "vm/objects/all.hpp"

#include <cstring>

// #define TIRO_TRACE_CALLS

namespace tiro::vm {

// Returns the current stack of the given coroutine. Asserts that the coroutine has a valid stack (if not,
// it would have been already completed).
static CoroutineStack current_stack(Coroutine coro) {
    TIRO_DEBUG_ASSERT(coro.stack().has_value(), "Coroutine does not have a valid stack.");
    return coro.stack().value();
}

static CoroutineStack current_stack(Handle<Coroutine> coro) {
    return current_stack(*coro);
}

// Grows the stack to ensure that that the condition is true.
// If the coroutine's stack changed as a result, the new stack will be applied
// to the coroutine and will also be returned in the optional. The optional will be empty
// if the stack was not modified.
//
// The stack will grow for as long `cond(current_stack)` returns false.
//
// \pre coroutines must have a valid inital stack (otherwise they would already have been completed).
static std::optional<CoroutineStack>
grow_stack_impl(Context& ctx, Handle<Coroutine> coro, FunctionRef<bool(CoroutineStack)> cond) {
    // Fast path: condition is already true.
    if (TIRO_LIKELY(cond(current_stack(coro))))
        return {};

    // Slow path: must allocate a new stack and transfer everything.
    Scope sc(ctx);
    Local old_stack = sc.local(current_stack(coro));
    Local new_stack = sc.local<CoroutineStack>(defer_init);

    TIRO_DEBUG_ASSERT(
        object_size(*old_stack) <= CoroutineStack::max_size, "Existing stack is too large.");

    // This should only do a single iteration in almost all circumstances, since the required memory
    // is typically very small (e.g. enough space for a function frame, or for a few values on the stack).
    // We don't have a precise measurement to predict the required size, though.
    do {
        u32 next_size = static_cast<u32>(object_size(*old_stack));
        if (TIRO_UNLIKELY(!checked_mul<u32>(next_size, 2)))
            TIRO_ERROR("Overflow in stack size computation.");

        if (TIRO_UNLIKELY(next_size > CoroutineStack::max_size))
            TIRO_ERROR("Stack overflow.");

        new_stack = CoroutineStack::grow(ctx, old_stack, next_size);
    } while (!cond(*new_stack));

    coro->stack(*new_stack);
    return *new_stack;
}

// Grows the stack to ensure that there are at least `required_value_capacity` free value
// slots available.
static std::optional<CoroutineStack>
reserve_values(Context& ctx, Handle<Coroutine> coro, u32 required_value_capacity) {
    auto cond = [&](CoroutineStack current) {
        return current.value_capacity_remaining() >= required_value_capacity;
    };

    auto result = grow_stack_impl(ctx, coro, cond);
    TIRO_DEBUG_ASSERT(!grow_stack_impl(ctx, coro, cond),
        "A repeated invocation must be a noop since enough space has been allocated.");
    return result;
}

// Pushes a value on the coroutine's stack. The stack must be large enough to hold
// the additional value.
static void must_push_value(CoroutineStack stack, Value v) {
    if (TIRO_LIKELY(stack.push_value(v)))
        return;

    // Programming error; use reserve_values() correctly.
    TIRO_ERROR(
        "The stack is full "
        "(failed to reserve enough capacity beforehand).");
}

static void must_push_value(Handle<Coroutine> coro, Value v) {
    must_push_value(current_stack(*coro), v);
}

// Pushes a user function call frame on the coroutine stack. Resizes the stack as necessary.
static void push_user_frame(Context& ctx, Handle<Coroutine> coro, Handle<CodeFunctionTemplate> tmpl,
    Handle<Nullable<Environment>> closure, u8 flags) {
    grow_stack_impl(ctx, coro,
        [&](CoroutineStack current) { return current.push_user_frame(*tmpl, *closure, flags); });
}

static void push_sync_frame(
    Context& ctx, Handle<Coroutine> coro, Handle<NativeFunction> func, u32 argc, u8 flags) {
    grow_stack_impl(ctx, coro,
        [&](CoroutineStack current) { return current.push_sync_frame(*func, argc, flags); });
}

// Pushes an async native function call frame on the coroutine stack. Resizes the stack as necessary.
static void push_async_frame(
    Context& ctx, Handle<Coroutine> coro, Handle<NativeFunction> func, u32 argc, u8 flags) {
    grow_stack_impl(ctx, coro,
        [&](CoroutineStack current) { return current.push_async_frame(*func, argc, flags); });
}

static void push_catch_frame(Context& ctx, Handle<Coroutine> coro, u32 argc, u8 flags) {
    grow_stack_impl(
        ctx, coro, [&](CoroutineStack current) { return current.push_catch_frame(argc, flags); });
}

template<typename T>
static T read_big_endian(const byte*& ptr) {
    T value;
    std::memcpy(&value, ptr, sizeof(T));
    value = be_to_host(value);
    ptr += sizeof(T);
    return value;
}

[[maybe_unused]] static void
trace_call(Context& ctx, Handle<Coroutine> coro, Handle<Value> function, u32 argc) {
    Scope sc(ctx);
    Local stack = sc.local(current_stack(coro));
    Local builder = sc.local(StringBuilder::make(ctx));

    TIRO_DEBUG_ASSERT(stack->top_value_count() >= argc,
        "Not enough arguments on the stack for this function call.");
    HandleSpan args = HandleSpan<Value>(stack->top_values(argc));

    to_string(ctx, builder, function);

    builder->append(ctx, "(");
    for (u32 i = 0; i < argc; ++i) {
        if (i > 0)
            builder->append(ctx, ", ");

        to_string(ctx, builder, args[i]);
    }
    builder->append(ctx, ")");

    fmt::print("CALL: {}\n", builder->view());
}

Value* Registers::alloc_slot() {
    // This error would be a programming error, the maximum number of
    // internal registers has a static upper limit.
    TIRO_CHECK(slots_used_ < slots_.size(), "No more available registers.");
    return &slots_[slots_used_++];
}

BytecodeInterpreter::BytecodeInterpreter(
    Context& ctx, Interpreter& parent, Registers& regs, Coroutine coro, CodeFrame* frame)
    : ctx_(ctx)
    , parent_(parent)
    , regs_(regs)
    , coro_(coro)
    , stack_(coro.stack().value())
    , frame_(frame) {
    TIRO_DEBUG_ASSERT(frame == stack_.top_frame(), "Frame must be on top of the stack.");
    TIRO_DEBUG_ASSERT(frame->type == FrameType::Code, "Unexpected frame type.");
}

#define TIRO_BINOP(op)                         \
    do {                                       \
        auto lhs = read_local();               \
        auto rhs = read_local();               \
        auto target = read_local();            \
        auto result = op(ctx_, lhs, rhs);      \
        if (result.has_exception())            \
            return unwind(result.exception()); \
                                               \
        target.set(result.value());            \
    } while (0)

#define TIRO_UNOP(op)                          \
    do {                                       \
        auto value = read_local();             \
        auto target = read_local();            \
        auto result = op(ctx_, value);         \
        if (result.has_exception())            \
            return unwind(result.exception()); \
                                               \
        target.set(result.value());            \
    } while (0)

#define TIRO_CMP(expr)                         \
    do {                                       \
        auto lhs = read_local();               \
        auto rhs = read_local();               \
        auto target = read_local();            \
        auto result = compare(ctx_, lhs, rhs); \
        if (result.has_exception())            \
            return unwind(result.exception()); \
                                               \
        auto cmp = result.value();             \
        target.set(ctx_.get_boolean((expr)));  \
    } while (0)

void BytecodeInterpreter::run() {
    while (1) {
        // TODO static verify
        if (TIRO_UNLIKELY(readable_bytes() == 0)) {
            TIRO_ERROR(
                "Invalid program counter: end of code reached "
                "without return from function.");
        }

        const BytecodeOp op = read_op();
        // fmt::print("Running op {}\n", to_string(op));

        ScopeExit reset_registers = [&] { regs_.reset(); };
        switch (op) {
        case BytecodeOp::LoadNull: {
            auto target = read_local();
            target.set(Value::null());
            break;
        }
        case BytecodeOp::LoadFalse: {
            auto target = read_local();
            target.set(ctx_.get_boolean(false));
            break;
        }
        case BytecodeOp::LoadTrue: {
            auto target = read_local();
            target.set(ctx_.get_boolean(true));
            break;
        }
        case BytecodeOp::LoadInt: {
            const i64 value = read_i64();
            auto target = read_local();
            target.set(ctx_.get_integer(value));
            break;
        }
        case BytecodeOp::LoadFloat: {
            const f64 value = read_f64();
            auto target = read_local();
            target.set(Float::make(ctx_, value));
            break;
        }
        case BytecodeOp::LoadParam: {
            // TODO static verify param index
            const u32 source = read_u32();
            auto target = read_local();
            TIRO_DEBUG_ASSERT(source < frame_->args, "Parameter index out of bounds.");

            target.set(*CoroutineStack::arg(frame_, source));
            break;
        }
        case BytecodeOp::StoreParam: {
            // TODO static verify param index
            auto source = read_local();
            const u32 target = read_u32();
            TIRO_DEBUG_ASSERT(target < frame_->args, "Parameter index out of bounds.");

            *CoroutineStack::arg(frame_, target) = *source;
            break;
        }
        case BytecodeOp::LoadModule: {
            const u32 source = read_u32();
            auto target = read_local();
            target.set(get_member(source));
            break;
        }
        case BytecodeOp::StoreModule: {
            auto source = read_local();
            const u32 index = read_u32();
            set_member(index, *source);
            break;
        }
        case BytecodeOp::LoadMember: {
            auto object = read_local();
            const u32 name = read_u32();
            auto target = read_local();

            // TODO: Static verify
            auto name_arg = reg(get_member(name)).try_cast<Symbol>();
            TIRO_CHECK(name_arg, "The module member at index {} must be a symbol.", name);

            auto name_symbol = name_arg.handle();
            auto res = ctx_.types().load_member(ctx_, object, name_symbol);
            if (TIRO_UNLIKELY(res.has_exception()))
                return unwind(res.exception());

            target.set(res.value());
            break;
        }
        case BytecodeOp::StoreMember: {
            auto source = read_local();
            auto object = read_local();
            const u32 name = read_u32();

            // TODO: Static verify
            auto name_arg = reg(get_member(name)).try_cast<Symbol>();
            TIRO_CHECK(name_arg, "The module member at index {} must be a symbol.", name);

            auto name_symbol = name_arg.handle();
            auto res = ctx_.types().store_member(ctx_, object, name_symbol, source);
            if (TIRO_UNLIKELY(res.has_exception()))
                return unwind(res.exception());

            break;
        }
        case BytecodeOp::LoadTupleMember: {
            auto object = read_local();
            const u32 index = read_u32();
            auto target = read_local();

            auto maybe_tuple = object.try_cast<Tuple>();
            if (TIRO_UNLIKELY(!maybe_tuple)) {
                return unwind(TIRO_FORMAT_EXCEPTION(
                    ctx_, "expected object of type tuple, but got '{}'", object->type()));
            }

            auto tuple = maybe_tuple.handle();
            if (TIRO_UNLIKELY(index >= tuple->size())) {
                return unwind(TIRO_FORMAT_EXCEPTION(
                    ctx_, "invalid index {} into tuple of size {}", index, tuple->size()));
            }
            target.set(tuple->get(index));
            break;
        }
        case BytecodeOp::StoreTupleMember: {
            auto source = read_local();
            auto object = read_local();
            const u32 index = read_u32();

            auto maybe_tuple = object.try_cast<Tuple>();
            if (TIRO_UNLIKELY(!maybe_tuple)) {
                return unwind(TIRO_FORMAT_EXCEPTION(
                    ctx_, "expected object of type tuple but got '{}'.", object->type()));
            }

            auto tuple = maybe_tuple.handle();
            if (TIRO_UNLIKELY(index >= tuple->size())) {
                return unwind(TIRO_FORMAT_EXCEPTION(
                    ctx_, "invalid index {} into tuple of size {}", index, tuple->size()));
            }
            tuple->set(index, *source);
            break;
        }
        case BytecodeOp::LoadIndex: {
            auto array = read_local();
            auto index = read_local();
            auto target = read_local();

            auto res = ctx_.types().load_index(ctx_, array, index);
            if (TIRO_UNLIKELY(res.has_exception()))
                return unwind(res.exception());

            target.set(res.value());
            break;
        }
        case BytecodeOp::StoreIndex: {
            auto source = read_local();
            auto array = read_local();
            auto index = read_local();

            auto res = ctx_.types().store_index(ctx_, array, index, source);
            if (TIRO_UNLIKELY(res.has_exception()))
                return unwind(res.exception());

            break;
        }
        case BytecodeOp::LoadClosure: {
            auto target = read_local();

            // TODO: Static verify
            TIRO_CHECK(!frame_->closure.is_null(), "Function does not have a closure.");
            target.set(frame_->closure);
            break;
        }
        case BytecodeOp::LoadEnv: {
            auto env_arg = read_local();
            const u32 level = read_u32();
            const u32 index = read_u32();
            auto target = read_local();

            auto maybe_env = env_arg.try_cast<Environment>();
            if (TIRO_UNLIKELY(!maybe_env)) {
                return unwind(TIRO_FORMAT_EXCEPTION(
                    ctx_, "expected object of type environment, but got '{}'", env_arg->type()));
            }

            auto current_env = reg<Nullable<Environment>>(*maybe_env.handle());
            if (level != 0)
                current_env.set(current_env->value().parent(level));

            if (TIRO_UNLIKELY(current_env->is_null())) { // Codegen error
                return unwind(TIRO_FORMAT_EXCEPTION(
                    ctx_, "too many levels requested from closure environment: {}", level));
            }

            if (TIRO_UNLIKELY(index >= current_env->value().size())) { // Codegen error
                return unwind(TIRO_FORMAT_EXCEPTION(ctx_,
                    "environment index {} is too large for environment of size {}", index,
                    current_env->value().size()));
            }

            auto value = current_env->value().get(index);
            if (TIRO_UNLIKELY(ctx_.get_undefined().same(value))) { // Codegen error
                return unwind(TIRO_FORMAT_EXCEPTION(
                    ctx_, "closure environment variable at index {} is undefined", index));
            }

            target.set(value);
            break;
        }
        case BytecodeOp::StoreEnv: {
            auto source = read_local();
            auto env_arg = read_local();
            const u32 level = read_u32();
            const u32 index = read_u32();

            auto maybe_env = env_arg.try_cast<Environment>();
            if (TIRO_UNLIKELY(!maybe_env)) {
                return unwind(TIRO_FORMAT_EXCEPTION(
                    ctx_, "expected object of type environment, but got '{}'", env_arg->type()));
            }

            auto current_env = reg<Nullable<Environment>>(*maybe_env.handle());
            if (level != 0)
                current_env.set(current_env->value().parent(level));

            if (TIRO_UNLIKELY(index >= current_env->value().size())) { // Codegen error
                return unwind(TIRO_FORMAT_EXCEPTION(ctx_,
                    "environment index {} is too large for environment of size {}", index,
                    current_env->value().size()));
            }

            if (TIRO_UNLIKELY(current_env->is_null())) { // Codegen error
                return unwind(TIRO_FORMAT_EXCEPTION(
                    ctx_, "too many levels requested from closure environment: {}", level));
            }

            current_env->value().set(index, *source);
            break;
        }
        case BytecodeOp::Add:
            TIRO_BINOP(add);
            break;
        case BytecodeOp::Sub:
            TIRO_BINOP(sub);
            break;
        case BytecodeOp::Mul:
            TIRO_BINOP(mul);
            break;
        case BytecodeOp::Div:
            TIRO_BINOP(div);
            break;
        case BytecodeOp::Mod:
            TIRO_BINOP(mod);
            break;
        case BytecodeOp::Pow:
            TIRO_BINOP(pow);
            break;
        case BytecodeOp::UAdd:
            TIRO_UNOP(unary_plus);
            break;
        case BytecodeOp::UNeg:
            TIRO_UNOP(unary_minus);
            break;

        // TODO
        case BytecodeOp::LSh:
        case BytecodeOp::RSh:
        case BytecodeOp::BAnd:
        case BytecodeOp::BOr:
        case BytecodeOp::BXor:
            TIRO_ERROR("Instruction not implemented yet: {}.", op);

        case BytecodeOp::BNot:
            TIRO_UNOP(bitwise_not);
            break;
        case BytecodeOp::Gt:
            TIRO_CMP((cmp > 0));
            break;
        case BytecodeOp::Gte:
            TIRO_CMP((cmp >= 0));
            break;
        case BytecodeOp::Lt:
            TIRO_CMP((cmp < 0));
            break;
        case BytecodeOp::Lte:
            TIRO_CMP((cmp <= 0));
            break;
        case BytecodeOp::Eq: {
            auto lhs = read_local();
            auto rhs = read_local();
            auto target = read_local();
            target.set(ctx_.get_boolean(equal(*lhs, *rhs)));
            break;
        }
        case BytecodeOp::NEq: {
            auto lhs = read_local();
            auto rhs = read_local();
            auto target = read_local();
            target.set(ctx_.get_boolean(!equal(*lhs, *rhs)));
            break;
        }
        case BytecodeOp::LNot: {
            auto value = read_local();
            auto target = read_local();
            target.set(ctx_.get_boolean(!ctx_.is_truthy(value)));
            break;
        }
        case BytecodeOp::Array: {
            const u32 count = read_u32();
            auto target = read_local();

            target.set(Array::make(ctx_, HandleSpan<Value>(stack_.top_values(count))));
            stack_.pop_values(count);
            break;
        }
        case BytecodeOp::Tuple: {
            const u32 count = read_u32();
            auto target = read_local();

            target.set(Tuple::make(ctx_, HandleSpan<Value>(stack_.top_values(count))));
            stack_.pop_values(count);
            break;
        }
        case BytecodeOp::Set: {
            const u32 count = read_u32();
            auto target = read_local();

            target.set(Set::make(ctx_, HandleSpan<Value>(stack_.top_values(count))));
            stack_.pop_values(count);
            break;
        }
        case BytecodeOp::Map: {
            // FIXME overflow protection
            const u32 count = read_u32();
            auto target = read_local();

            // TODO: Static verify
            TIRO_CHECK(count % 2 == 0,
                "Map instruction requires an even number of arguments (keys "
                "and values).");
            const Span<Value> pairs = stack_.top_values(count);

            auto map = reg(HashTable::make(ctx_, count));
            for (u32 i = 0; i < count; i += 2) {
                auto key = Handle<Value>(pairs.data() + i);
                auto value = Handle<Value>(pairs.data() + i + 1);
                map->set(ctx_, key, value);
            }

            target.set(map);
            stack_.pop_values(count);
            break;
        }
        case BytecodeOp::Env: {
            auto parent = read_local().try_cast<Nullable<Environment>>();
            const u32 size = read_u32();
            auto target = read_local();

            if (TIRO_UNLIKELY(!parent)) {
                return unwind(
                    TIRO_FORMAT_EXCEPTION(ctx_, "parent must be null or another environment"));
            }

            target.set(Environment::make(ctx_, size, maybe_null(parent.handle())));
            break;
        }
        case BytecodeOp::Closure: {
            auto tmpl_arg = read_local();
            auto env_arg = read_local();
            auto target = read_local();

            auto maybe_tmpl = tmpl_arg.try_cast<CodeFunctionTemplate>();
            if (TIRO_UNLIKELY(!maybe_tmpl)) {
                return unwind(TIRO_FORMAT_EXCEPTION(
                    ctx_, "expected a function template, but got '{}'", tmpl_arg->type()));
            }

            auto maybe_env = env_arg.try_cast<Nullable<Environment>>();
            if (TIRO_UNLIKELY(!maybe_env)) {
                return unwind(TIRO_FORMAT_EXCEPTION(
                    ctx_, "expected an environment or null, but got '{}'", env_arg->type()));
            }

            target.set(
                CodeFunction::make(ctx_, maybe_tmpl.handle(), maybe_null(maybe_env.handle())));
            break;
        }
        case BytecodeOp::Record: {
            const u32 tmpl_arg = read_u32();
            auto target = read_local();

            // TODO: Static verify
            auto maybe_tmpl = reg(get_member(tmpl_arg)).try_cast<RecordTemplate>();
            TIRO_CHECK(
                maybe_tmpl, "the module member at index {} must be a record template", tmpl_arg);

            target.set(Record::make(ctx_, maybe_tmpl.handle()));
            break;
        }
        case BytecodeOp::Iterator: {
            auto container = read_local();
            auto target = read_local();
            auto res = ctx_.types().iterator(ctx_, container);
            if (TIRO_UNLIKELY(res.has_exception()))
                return unwind(res.exception());

            target.set(res.value());
            break;
        }
        case BytecodeOp::IteratorNext: {
            auto iterator = read_local();
            auto valid = read_local();
            auto value = read_local();

            auto res = ctx_.types().iterator_next(ctx_, iterator);
            if (TIRO_UNLIKELY(res.has_exception()))
                return unwind(res.exception());

            auto& next = res.value();
            valid.set(ctx_.get_boolean(next.has_value()));
            value.set(next ? *next : Value::null());
            break;
        }
        case BytecodeOp::Formatter: {
            // Initial capacity would improve performance!
            auto target = read_local();
            target.set(StringBuilder::make(ctx_));
            break;
        }
        case BytecodeOp::AppendFormat: {
            auto value = read_local();
            auto formatter_arg = read_local();

            auto maybe_formatter = formatter_arg.try_cast<StringBuilder>();
            if (TIRO_UNLIKELY(!maybe_formatter)) {
                return unwind(TIRO_FORMAT_EXCEPTION(
                    ctx_, "expected a string builder, but got '{}'", formatter_arg->type()));
            }
            to_string(ctx_, maybe_formatter.handle(), value);
            break;
        }
        case BytecodeOp::FormatResult: {
            auto formatter_arg = read_local();
            auto target = read_local();

            auto maybe_formatter = formatter_arg.try_cast<StringBuilder>();
            if (TIRO_UNLIKELY(!maybe_formatter)) {
                return unwind(TIRO_FORMAT_EXCEPTION(
                    ctx_, "expected a string builder, but got '{}'", formatter_arg->type()));
            }
            target.set(maybe_formatter.handle()->to_string(ctx_));
            break;
        }
        case BytecodeOp::Copy: {
            auto source = read_local();
            auto target = read_local();
            target.set(source);
            break;
        }
        case BytecodeOp::Swap: {
            auto a = read_local();
            auto b = read_local();
            auto t = a.get();
            a.set(b);
            b.set(t);
            break;
        };
        case BytecodeOp::Push: {
            reserve_stack(1);

            auto value = read_local();
            push_stack(*value);
            break;
        }
        case BytecodeOp::Pop: {
            if (TIRO_UNLIKELY(stack_.top_value_count() == 0)) {
                return unwind(
                    TIRO_FORMAT_EXCEPTION(ctx_, "cannot pop any more values from the stack"));
            }

            stack_.pop_value();
            break;
        }
        case BytecodeOp::PopTo: {
            if (TIRO_UNLIKELY(stack_.top_value_count() == 0)) {
                return unwind(
                    TIRO_FORMAT_EXCEPTION(ctx_, "cannot pop any more values from the stack"));
            }

            auto target = read_local();
            target.set(*stack_.top_value());
            stack_.pop_value();
            break;
        }
        case BytecodeOp::Jmp: {
            // TODO static verify
            const u32 target = read_u32();
            set_pc(target);
            break;
        }
        case BytecodeOp::JmpTrue: {
            // TODO static verify
            auto value = read_local();
            const u32 target = read_u32();
            if (ctx_.is_truthy(value)) {
                set_pc(target);
            }
            break;
        }
        case BytecodeOp::JmpFalse: {
            // TODO static verify
            auto value = read_local();
            const u32 target = read_u32();
            if (!ctx_.is_truthy(value)) {
                set_pc(target);
            }
            break;
        }
        case BytecodeOp::JmpNull: {
            // TODO static verify
            auto value = read_local();
            const u32 target = read_u32();
            if (value->is_null()) {
                set_pc(target);
            }
            break;
        }
        case BytecodeOp::JmpNotNull: {
            // TODO static verify
            auto value = read_local();
            const u32 target = read_u32();
            if (!value->is_null()) {
                set_pc(target);
            }
            break;
        }
        case BytecodeOp::Call: {
            auto func = read_local();
            const u32 count = read_u32();
            return parent_.call_function(Handle<Coroutine>(&coro_), func, count);
        }
        case BytecodeOp::LoadMethod: {
            auto object = read_local();
            const u32 name = read_u32();
            auto this_ = read_local();
            auto method = read_local();

            auto name_arg = reg(get_member(name));
            auto maybe_name = name_arg.try_cast<Symbol>();
            if (TIRO_UNLIKELY(!maybe_name)) {
                // TODO static verify
                return unwind(TIRO_FORMAT_EXCEPTION(ctx_,
                    "referenced module member must be a symbol, but got '{}'", name_arg->type()));
            }

            auto name_symbol = maybe_name.handle();

            auto res = ctx_.types().load_method(ctx_, object, name_symbol);
            if (TIRO_UNLIKELY(res.has_exception()))
                return unwind(res.exception());

            auto func = res.value();
            if (func.is<Method>()) {
                this_.set(object);
                method.set(func.must_cast<Method>().function());
            } else {
                this_.set(Value::null());
                method.set(func);
            }
            break;
        }
        case BytecodeOp::CallMethod: {
            auto method = read_local();
            const u32 count = read_u32();
            return parent_.call_method(Handle<Coroutine>(&coro_), method, count);
        }
        case BytecodeOp::Return: {
            auto value = read_local();
            return return_function(*value);
        }
        case BytecodeOp::Rethrow: {
            // TODO: Static verify usage of rethrow in bytecode
            TIRO_DEBUG_ASSERT(frame_->flags & FRAME_UNWINDING,
                "Function must be unwinding when using the rethrow instruction.");
            TIRO_DEBUG_ASSERT(
                frame_->current_exception.has_value(), "Current exception must be present.");
            return unwind(frame_->current_exception.value());
        }
        case BytecodeOp::AssertFail: {
            auto expr_arg = read_local();
            auto message_arg = read_local();

            auto maybe_expr = expr_arg.try_cast<String>();
            if (TIRO_UNLIKELY(!maybe_expr)) {
                return unwind(TIRO_FORMAT_EXCEPTION(
                    ctx_, "assertion expression must be a string, but got '{}'", expr_arg->type()));
            }

            auto maybe_message = message_arg.try_cast<Nullable<String>>();
            if (TIRO_UNLIKELY(!maybe_message)) {
                return unwind(TIRO_FORMAT_EXCEPTION(ctx_,
                    "assertion error message must be a string or null, but got '{}'",
                    message_arg->type()));
            }

            return unwind(assertion_failed_exception(
                ctx_, maybe_expr.handle(), maybe_null(maybe_message.handle())));
        }
        }
    }
}

#undef TIRO_BINOP
#undef TIRO_UNOP
#undef TIRO_CMP

bool BytecodeInterpreter::handle_exception(
    Context& ctx, CodeFrame* frame, MutHandle<Exception> ex) {

    if (frame->flags & FRAME_UNWINDING) {
        TIRO_DEBUG_ASSERT(frame->current_exception, "Frame must have an active exception.");
        frame->current_exception.value().add_secondary(ctx, ex);
        ex.set(frame->current_exception.value());
    }

    auto maybe_handlers = frame->tmpl.handlers();
    if (!maybe_handlers.has_value())
        return false; // Function has no exception handlers

    auto handlers = maybe_handlers.value();
    auto code = frame->tmpl.code().view();
    u32 pc = frame->pc - code.begin();
    if (pc > 0) // pc points after the last instruction
        --pc;

    auto entry = handlers.find_entry(pc);
    if (!entry)
        return false;

    u32 target_pc = entry->target;
    TIRO_CHECK(target_pc < code.size(),
        "Exception handler destination out of bounds."); // TODO static verify

    frame->pc = code.data() + target_pc;
    frame->flags |= FRAME_UNWINDING;
    frame->current_exception = *ex;
    return true;
}

void BytecodeInterpreter::return_function(/* UNROOTED */ Value return_value) {
    return parent_.return_function(Handle<Coroutine>(&coro_), return_value);
}

void BytecodeInterpreter::unwind(/* UNROOTED */ Exception ex) {
    return parent_.unwind(Handle<Coroutine>(&coro_), ex);
}

Value BytecodeInterpreter::get_member(u32 index) {
    Module mod = frame_->tmpl.module();
    Tuple members = mod.members();
    TIRO_CHECK(index < members.size(), "Module member index out of bounds."); // TODO Static verify

    Value member = members.get(index);
    TIRO_CHECK(!member.is<Undefined>(), "Module member is undefined."); // TODO Static verify?
    return member;
}

void BytecodeInterpreter::set_member(u32 index, Value value) {
    Module mod = frame_->tmpl.module();
    Tuple members = mod.members();
    TIRO_CHECK(index < members.size(), "Module member index out of bounds."); // TODO Static verify
    members.set(index, value);
}

void BytecodeInterpreter::reserve_stack(u32 n) {
    auto new_stack = reserve_values(ctx_, Handle<Coroutine>(&coro_), n);
    if (TIRO_UNLIKELY(new_stack)) {
        TIRO_DEBUG_ASSERT(
            new_stack->same(coro_.stack().value()), "Must be the coroutine's current stack.");
        TIRO_DEBUG_ASSERT(new_stack->top_frame(), "New stack must have a frame.");
        TIRO_DEBUG_ASSERT(new_stack->top_frame()->type == FrameType::Code,
            "Top frame must still be a user frame.");
        stack_ = *new_stack;
        frame_ = static_cast<CodeFrame*>(stack_.top_frame());
    }
}

void BytecodeInterpreter::push_stack(Value v) {
    must_push_value(stack_, v);
}

BytecodeOp BytecodeInterpreter::read_op() {
    // TODO static verify
    TIRO_DEBUG_ASSERT(readable_bytes() >= 1, "Not enough available bytes.");

    u8 opcode = *frame_->pc++;
    TIRO_DEBUG_ASSERT(valid_opcode(opcode), "Invalid opcode.");
    return static_cast<BytecodeOp>(opcode);
}

i64 BytecodeInterpreter::read_i64() {
    // TODO static verify
    TIRO_DEBUG_ASSERT(readable_bytes() >= 8, "Not enough available bytes.");
    return static_cast<i64>(read_big_endian<u64>(frame_->pc));
}

f64 BytecodeInterpreter::read_f64() {
    // TODO static verify
    TIRO_DEBUG_ASSERT(readable_bytes() >= 8, "Not enough available bytes.");
    // FIXME float serialization in some helper function, see also compiler/binary.hpp
    static_assert(sizeof(f64) == sizeof(u64));
    u64 as_u64 = read_big_endian<u64>(frame_->pc);
    f64 d;
    std::memcpy(&d, &as_u64, sizeof(f64));
    return d;
}

u32 BytecodeInterpreter::read_u32() {
    // TODO static verify
    TIRO_DEBUG_ASSERT(readable_bytes() >= 4, "Not enough available bytes.");
    return read_big_endian<u32>(frame_->pc);
}

MutHandle<Value> BytecodeInterpreter::read_local() {
    // TODO static verify local index.
    const u32 local = read_u32();
    return MutHandle<Value>(CoroutineStack::local(frame_, local));
}

[[maybe_unused]] size_t BytecodeInterpreter::readable_bytes() const {
    return static_cast<size_t>(frame_->tmpl.code().view().end() - frame_->pc);
}

[[maybe_unused]] bool BytecodeInterpreter::pc_in_bounds(u32 offset) const {
    return offset < frame_->tmpl.code().size();
}

void BytecodeInterpreter::set_pc(u32 offset) {
    TIRO_DEBUG_ASSERT(pc_in_bounds(offset), "Jump destination is out of bounds.");
    frame_->pc = frame_->tmpl.code().data() + offset;
}

void Interpreter::init(Context& ctx) {
    ctx_ = &ctx;
}

Coroutine Interpreter::make_coroutine(Handle<Value> func, MaybeHandle<Tuple> arguments) {
    TIRO_CHECK(!func->is_null(), "Invalid function object.");

    Scope sc(ctx());
    Local stack = sc.local(CoroutineStack::make(ctx(), CoroutineStack::initial_size));
    Local name_builder = sc.local(StringBuilder::make(ctx(), 32));
    name_builder->format(ctx(), "Coroutine-{}", next_id_++);
    Local name = sc.local(name_builder->to_string(ctx()));
    return Coroutine::make(ctx(), name, func, arguments, stack);
}

void Interpreter::run(Handle<Coroutine> coro) {
    TIRO_DEBUG_ASSERT(!running_, "The interpreter is already running.");

    running_ = true;
    ScopeExit reset_running = [&] { running_ = false; };
    ScopeExit reset_regs = [&] { regs_.reset(); };

    TIRO_DEBUG_ASSERT(!coro->is_null(), "Invalid coroutine.");
    TIRO_DEBUG_ASSERT(!coro->stack().is_null(), "Coroutine must have a valid stack.");
    TIRO_DEBUG_ASSERT(coro->state() != CoroutineState::Done, "Coroutine must not be completed.");

    run_until_block(coro);
    TIRO_DEBUG_ASSERT(coro->state() == CoroutineState::Ready
                          || coro->state() == CoroutineState::Waiting
                          || coro->state() == CoroutineState::Done,
        "Invalid coroutine state after running, must be either Ready, Waiting or Done.");

    if (coro->state() == CoroutineState::Done) {
        TIRO_DEBUG_ASSERT(!coro->result().is_null(), "Completed coroutines must have a result.");
        coro->stack(Nullable<CoroutineStack>());
    }
}

void Interpreter::run_until_block(Handle<Coroutine> coro) {
    TIRO_DEBUG_ASSERT(is_runnable(coro->state()), "Coroutine must be in a runnable state.");

    // Arrange for the initial function call on a new coroutine.
    if (coro->state() == CoroutineState::Started) {
        auto func = reg(coro->function());
        auto args = reg(coro->arguments());

        const u32 argc = args->is_null() ? 0 : args->value().size();
        if (argc > 0) {
            reserve_values(ctx(), coro, argc);
            for (u32 i = 0; i < argc; ++i)
                must_push_value(coro, args->value().get(i));
        }

        call_function(coro, func, argc);
    }

    // Interpret call frames until yield or done
    coro->state(CoroutineState::Running);
    while (coro->state() == CoroutineState::Running) {
        // WARNING: Invalidated by stack growth!
        auto frame = current_stack(coro).top_frame();
        TIRO_DEBUG_ASSERT(frame, "Running coroutines must have call frames.");
        switch (frame->type) {
        case FrameType::Code:
            run_frame(coro, static_cast<CodeFrame*>(frame));
            break;
        case FrameType::Sync:
            run_frame(coro, static_cast<SyncFrame*>(frame));
            break;
        case FrameType::Async:
            run_frame(coro, static_cast<AsyncFrame*>(frame));
            break;
        case FrameType::Catch:
            run_frame(coro, static_cast<CatchFrame*>(frame));
            break;
        }

        TIRO_DEBUG_ASSERT(
            coro->state() == CoroutineState::Running || coro->state() == CoroutineState::Waiting
                || coro->state() == CoroutineState::Ready || coro->state() == CoroutineState::Done,
            "Unexpected coroutine state.");
    }
}

void Interpreter::run_frame(Handle<Coroutine> coro, CodeFrame* frame) {
    // TODO: Investigate performance impact. The additional object exists for convenient
    // invariant (a coroutine is present, as is a valid stack). We could cache that instance
    // in a lazily initialized optional.
    TIRO_DEBUG_ASSERT(child_ == nullptr, "Must not have an active bytecode interpreter.");
    TIRO_DEBUG_ASSERT(
        coro->state() == CoroutineState::Running, "The coroutine must be marked as running.");

    // Register the child interpreter to allow the garbage collector
    // to trace it.
    BytecodeInterpreter child(ctx(), *this, regs_, *coro, frame);
    child_ = &child;
    ScopeExit reset_child = [&] { child_ = nullptr; };
    return child.run();
}

// Sync functions return immediately, the stack frame exists only for simplicity and for error reporting (-> stack trace).
void Interpreter::run_frame(Handle<Coroutine> coro, SyncFrame* frame) {
    TIRO_DEBUG_ASSERT(frame == current_stack(coro).top_frame(), "Expected the topmost frame.");
    TIRO_DEBUG_ASSERT(frame->type == FrameType::Sync, "Expected a sync frame.");
    TIRO_DEBUG_ASSERT(
        coro->state() == CoroutineState::Running, "The coroutine must be marked as running.");

    ScopeExit reset_regs = [&] { regs_.reset(); };

    auto result = reg(Value::null());
    NativeFunctionFrame native_frame(ctx(), coro, frame, result);
    frame->func.function().invoke_sync(native_frame);

    // "Waiting" is allowed for std.yield (a cleaner solution would need more powerful native coroutines,
    // which are not yet implemented here).
    TIRO_DEBUG_ASSERT(coro->state() == CoroutineState::Running
                          || coro->state() == CoroutineState::Ready
                          || coro->state() == CoroutineState::Waiting,
        "Illegal modification of the coroutine's state.");

    if (frame->flags & FRAME_UNWINDING) {
        if (result->type() != ValueType::Exception) {
            result.set(TIRO_FORMAT_EXCEPTION(*ctx_,
                "native function attempted to throw a non-exception type '{}'.", result->type()));
        }
        return unwind(coro, result->must_cast<Exception>());
    }
    return return_function(coro, *result);
}

// When an async function frame is entered for the first time, we call the native initiating function.
// When it is entered the second time, this second time must be because it was resumed by the native function,
// we will then return the return value to the caller.
// The initiating function may resume immediately, in which case the coroutine will not yield.
void Interpreter::run_frame(Handle<Coroutine> coro, AsyncFrame* frame) {
    TIRO_DEBUG_ASSERT(frame == current_stack(coro).top_frame(), "Expected the topmost frame.");
    TIRO_DEBUG_ASSERT(frame->type == FrameType::Async, "Expected an async frame.");
    TIRO_DEBUG_ASSERT(
        coro->state() == CoroutineState::Running, "The coroutine must be marked as running.");
    TIRO_DEBUG_ASSERT(
        (frame->flags & FRAME_ASYNC_CALLED) == 0 || (frame->flags & FRAME_ASYNC_RESUMED) != 0,
        "Must have resumed if the async function already yielded.");

    ScopeExit reset_regs = [&] { regs_.reset(); };

    if ((frame->flags & FRAME_ASYNC_CALLED) == 0) {
        NativeAsyncFunctionFrame native_frame(ctx(), coro, frame);
        frame->func.function().invoke_async(std::move(native_frame));

        // Async function did not resume immediately, put it to sleep.
        frame->flags |= FRAME_ASYNC_CALLED;
        if ((frame->flags & FRAME_ASYNC_RESUMED) == 0) {
            coro->state(CoroutineState::Waiting);
        }
        return;
    }

    if ((frame->flags & FRAME_ASYNC_RESUMED) != 0)
        return return_function(coro, frame->return_value);
}

void Interpreter::run_frame(Handle<Coroutine> coro, CatchFrame* frame) {
    TIRO_DEBUG_ASSERT(frame == current_stack(coro).top_frame(), "Expected the topmost frame.");
    TIRO_DEBUG_ASSERT(frame->type == FrameType::Catch, "Expected a catch frame.");
    TIRO_DEBUG_ASSERT(
        coro->state() == CoroutineState::Running, "The coroutine must be marked as running.");
    TIRO_DEBUG_ASSERT(frame->args == 1, "Expected a single argument.");

    // Fresh frame.
    if ((frame->flags & FRAME_CATCH_STARTED) == 0) {
        TIRO_DEBUG_ASSERT((frame->flags & FRAME_UNWINDING) == 0,
            "Cannot be unwinding before the frame has been started");
        frame->flags |= FRAME_CATCH_STARTED;

        auto arg = Handle<Value>(current_stack(coro).arg(frame, 0));
        return call_function(coro, arg, 0);
    }

    // Function was called and panicked.
    if (frame->flags & FRAME_UNWINDING) {
        TIRO_DEBUG_ASSERT(frame->exception.has_value(), "Exception must be set");
        return return_function(coro, Result::make_failure(ctx(), Handle<Value>(&frame->exception)));
    }

    // Function was called and returned normally, so there must be a return value on the stack.
    auto stack = current_stack(coro);
    TIRO_DEBUG_ASSERT(stack.top_value_count() > 0, "Missing return value on the stack.");
    return return_function(coro, Result::make_success(ctx(), Handle<Value>(stack.top_value())));
}

void Interpreter::call_function(Handle<Coroutine> coro, Handle<Value> function, u32 argc) {
    TIRO_DEBUG_ASSERT(current_stack(coro).top_value_count() >= argc,
        "The value stack must contain all arguments.");
    auto local_function = reg(function);
    return enter_function(coro, local_function, argc, false);
}

void Interpreter::call_method(Handle<Coroutine> coro, Handle<Value> method, u32 argc) {
    TIRO_DEBUG_ASSERT(current_stack(coro).top_value_count() >= argc + 1,
        "The value stack must contain the all arguments, including `this`.");

    auto local_method = reg(method);
    bool is_method = !current_stack(coro).top_value(argc)->is_null();
    return enter_function(coro, local_method, argc + (is_method ? 1 : 0), !is_method);
}

void Interpreter::enter_function(
    Handle<Coroutine> coro, MutHandle<Value> function_register, u32 argc, bool pop_one_more) {
again:

#ifdef TIRO_TRACE_CALLS
    trace_call(ctx(), coro, function_register, argc);
#endif

    auto frame_flags = [&]() {
        u8 flags = 0;
        flags |= pop_one_more ? FRAME_POP_ONE_MORE : 0;
        return flags;
    };

    auto missing_args = [&](u32 actual, u32 required) {
        return unwind(coro, TIRO_FORMAT_EXCEPTION(ctx(),
                                "insufficient number of function arguments (need {}, but have {})",
                                required, actual));
    };

    const ValueType function_type = function_register->type();
    switch (function_type) {

    // Invokes a user defined function call. A new stack frame is pushed onto the stack, then we return.
    // The interpeter will continue the evaluation in the new function frame.
    // The final return instruction in the callee will restore the stack.
    // If `pop_one_more` is true, an additional value will be popped after returning from the callee.
    // This can happen if a normal function is called via the LOAD_METHOD / CALL_METHOD instruction pair,
    // in that case the unused `this` argument is on the stack but remains unused (it must still be popped, though).
    case ValueType::CodeFunction: {
        auto func = function_register.must_cast<CodeFunction>();

        auto tmpl = reg(func->tmpl());
        auto closure = reg(func->closure());
        if (tmpl->params() != argc)
            return missing_args(tmpl->params(), argc);

        push_user_frame(ctx(), coro, tmpl, closure, frame_flags());
        return;
    }

    // Invokes a member function with a bound "this" parameter.
    case ValueType::BoundMethod: {
        auto bound = function_register.must_cast<BoundMethod>();
        TIRO_DEBUG_ASSERT(bound->function().type() != ValueType::BoundMethod,
            "Bound methods must not be nested into each other.");

        // Make room for the additional parameter.
        reserve_values(ctx(), coro, 1);
        must_push_value(coro, Value::null());

        // Shift all existing arguments by one slot and
        // add the "this" parameter at the front.
        auto args = current_stack(coro).top_values(argc + 1);
        std::memmove(args.data() + 1, args.data(), argc * sizeof(Value));
        args[0] = bound->object();

        // Replace the callee.
        function_register.set(bound->function());
        ++argc;
        goto again;
    }

    // Invokes a native c function.
    case ValueType::NativeFunction: {
        auto native_func = function_register.must_cast<NativeFunction>();
        if (argc < native_func->params())
            return missing_args(native_func->params(), argc);

        switch (native_func->function().type()) {
        case NativeFunctionType::Sync: {
            push_sync_frame(ctx(), coro, native_func, argc, frame_flags());
            return;
        }
        case NativeFunctionType::Async: {
            push_async_frame(ctx(), coro, native_func, argc, frame_flags());
            return;
        }
        default:
            TIRO_UNREACHABLE("Invalid native function type.");
        }
    }

    // Invokes a special runtime function
    case ValueType::MagicFunction: {
        auto magic_func = function_register.must_cast<MagicFunction>();

        // Currently there is only one magic function, which takes one argument.
        if (argc < 1)
            return missing_args(1, argc);

        switch (magic_func->which()) {
        case MagicFunction::Catch:
            push_catch_frame(ctx(), coro, argc, frame_flags());
            return;
        default:
            TIRO_UNREACHABLE("Invalid magic function type.");
        }
    }

    default:
        return unwind(coro, function_call_not_supported_exception(ctx(), function_register));
    }
}

void Interpreter::return_function(Handle<Coroutine> coro, /* UNROOTED */ Value return_value) {
    pop_frame(coro);

    auto stack = current_stack(coro);
    if (stack.top_frame()) {
        TIRO_DEBUG_ASSERT(stack.value_capacity_remaining() > 0,
            "Popping the frame must make at least one value slot available.");
        must_push_value(stack, return_value);
    } else {
        auto safe_value = reg(return_value);
        coro->result(Result::make_success(ctx(), safe_value));
        coro->state(CoroutineState::Done);
    }
}

void Interpreter::unwind(Handle<Coroutine> coro, Exception unsafe_ex) {
    auto ex = reg(unsafe_ex);
    auto stack = current_stack(coro);
    while (1) {
        auto frame = stack.top_frame();
        if (!frame)
            break;

        if (unwind_into(frame, ex))
            return; // Evaluation continues in frame

        pop_frame(coro);
    }

    coro->result(Result::make_failure(ctx(), ex));
    coro->state(CoroutineState::Done);
}

bool Interpreter::unwind_into(CoroutineFrame* frame, MutHandle<Exception> ex) {
    switch (frame->type) {
    case FrameType::Async:
    case FrameType::Sync:
        return false; // Native functions cannot catch panics at the moment

    case FrameType::Code:
        return BytecodeInterpreter::handle_exception(ctx(), static_cast<CodeFrame*>(frame), ex);

    case FrameType::Catch: {
        CatchFrame* cf = static_cast<CatchFrame*>(frame);
        TIRO_DEBUG_ASSERT(cf->flags & FRAME_CATCH_STARTED, "catch frame must have been started.");
        cf->flags |= FRAME_UNWINDING;
        cf->exception = *ex;
        return true;
    }
    }

    TIRO_UNREACHABLE("Invalid coroutine frame type.");
}

void Interpreter::pop_frame(Handle<Coroutine> coro) {
    CoroutineStack stack = current_stack(coro);

    auto frame = stack.top_frame();
    TIRO_DEBUG_ASSERT(frame, "Invalid frame.");

    u32 pop_args = frame->args;
    if (frame->flags & FRAME_POP_ONE_MORE) {
        // Normal function invoked via CALL_METHOD, pop the additional value,
        // see the comment for call_method.
        ++pop_args;
    }

    stack.pop_frame();
    stack.pop_values(pop_args); // Function arguments
}

} // namespace tiro::vm
