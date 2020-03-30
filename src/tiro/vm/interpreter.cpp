#include "tiro/vm/interpreter.hpp"

#include "tiro/bytecode/opcode.hpp"
#include "tiro/core/byte_order.hpp"
#include "tiro/objects/arrays.hpp"
#include "tiro/objects/buffers.hpp"
#include "tiro/objects/classes.hpp"
#include "tiro/objects/functions.hpp"
#include "tiro/objects/hash_tables.hpp"
#include "tiro/objects/modules.hpp"
#include "tiro/objects/strings.hpp"
#include "tiro/vm/context.hpp"
#include "tiro/vm/math.hpp"

#include "tiro/objects/coroutines.ipp"
#include "tiro/vm/context.ipp"

#include <cstring>

namespace tiro::vm {

static Opcode read_op(UserFrame* frame);
static i64 read_i64(UserFrame* frame);
static f64 read_f64(UserFrame* frame);
static u32 read_u32(UserFrame* frame);
static MutableHandle<Value> read_local(UserFrame* frame, CoroutineStack stack);
static size_t readable(UserFrame* frame);
static bool offset_in_bounds(UserFrame* frame, u32 offset);

template<typename T>
static T read_big_endian(const byte*& ptr) {
    T value;
    std::memcpy(&value, ptr, sizeof(T));
    value = be_to_host(value);
    ptr += sizeof(T);
    return value;
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
    TIRO_CHECK(index < members.size(), "Module member index out of bounds.");

    Value member = members.get(index);
    TIRO_CHECK(!member.is<Undefined>(), "Module member is undefined.");
    return member;
}

static void set_module_member(UserFrame* frame, u32 index, Value value) {
    Module mod = frame->tmpl.module();
    Tuple members = mod.members();
    TIRO_CHECK(index < members.size(), "Module member index out of bounds.");
    members.set(index, value);
}

template<typename Func>
void binop(UserFrame* frame, CoroutineStack stack, Func&& fn) {
    auto lhs = read_local(frame, stack);
    auto rhs = read_local(frame, stack);
    auto target = read_local(frame, stack);
    fn(lhs, rhs, target);
}

void Interpreter::init(Context& ctx) {
    ctx_ = &ctx;
}

Coroutine Interpreter::make_coroutine(
    Handle<Value> func, /* nullable */ Handle<Tuple> arguments) {
    TIRO_CHECK(!func->is_null(), "Invalid function object.");

    Root stack(
        ctx(), CoroutineStack::make(ctx(), CoroutineStack::initial_size));
    Root name(ctx(), String::make(ctx(), "Coro-1")); // TODO name
    return Coroutine::make(ctx(), name, func, arguments, stack);
}

void Interpreter::run(Handle<Coroutine> coro) {
    TIRO_ASSERT(current_.is_null(), "Must not be running a coroutine.");
    TIRO_ASSERT(!coro->is_null(), "Invalid coroutine.");

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
        TIRO_ASSERT(stack_.top_value_count() == 1,
            "Must have left one value on the stack.");
        current_.result(Handle<Value>::from_slot(stack_.top_value()));
        current_.stack({});
    } else {
        TIRO_ASSERT(current_.state() == CoroutineState::Waiting,
            "Invalid coroutine state after running, must be either Done or "
            "Waiting.");
    }
}

void Interpreter::run_until_block() {
    TIRO_ASSERT(is_runnable(current_.state()),
        "Coroutine must be in a runnable state.");

    // This is the first time the coroutine runs. Start interpreting the job function.
    if (current_.state() == CoroutineState::New) {
        current_.state(CoroutineState::Running);

        auto func = reg(current_.function());
        auto args = reg(current_.arguments());

        const u32 argc = args->is_null() ? 0 : args->size();
        if (argc > 0) {
            reserve_values(argc);
            for (u32 i = 0; i < argc; ++i) {
                must_push_value(args->get(i));
            }
        }

        switch (call_function(func, argc)) {
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
        TIRO_ASSERT(frame_, "Invalid frame.");

        switch (frame_->type) {
        case FrameType::User:
            state = run_frame();
            break;
        case FrameType::Async:
            state = run_async_frame();
            break;
        }

        TIRO_ASSERT(state == CoroutineState::Running
                        || state == CoroutineState::Waiting
                        || state == CoroutineState::Done,
            "Unexpected coroutine state.");
    }

    current_.state(state);
}

CoroutineState Interpreter::run_frame() {
    auto frame = [&] {
        TIRO_ASSERT(frame_, "Invalid frame.");
        TIRO_ASSERT(frame_->type == FrameType::User,
            "Current frame is not a user frame.");
        return static_cast<UserFrame*>(frame_);
    };

    while (1) {
        // TODO static verify
        if (TIRO_UNLIKELY(frame()->pc == frame()->tmpl.code().view().end())) {
            TIRO_ERROR(
                "Invalid program counter: end of code reached "
                "without return from function.");
        }

        const Opcode op = read_op(frame());
        // fmt::print("Running op {}\n", to_string(op));

        ScopeExit reset_registers = [&] { registers_used_ = 0; };
        switch (op) {
        case Opcode::LoadNull: {
            auto target = read_local(frame(), stack_);
            target.set(Value::null());
            break;
        }
        case Opcode::LoadFalse: {
            auto target = read_local(frame(), stack_);
            target.set(ctx().get_boolean(false));
            break;
        }
        case Opcode::LoadTrue: {
            auto target = read_local(frame(), stack_);
            target.set(ctx().get_boolean(true));
            break;
        }
        case Opcode::LoadInt: {
            const i64 value = read_i64(frame());
            auto target = read_local(frame(), stack_);
            target.set(ctx().get_integer(value));
            break;
        }
        case Opcode::LoadFloat: {
            const f64 value = read_f64(frame());
            auto target = read_local(frame(), stack_);
            target.set(Float::make(ctx(), value));
            break;
        }
        case Opcode::LoadParam: {
            // TODO static verify param index
            const u32 source = read_u32(frame());
            auto target = read_local(frame(), stack_);
            TIRO_ASSERT(
                source < frame()->args, "Parameter index out of bounds.");

            target.set(*stack_.arg(source));
            break;
        }
        case Opcode::StoreParam: {
            // TODO static verify param index
            auto source = read_local(frame(), stack_);
            const u32 target = read_u32(frame());
            TIRO_ASSERT(
                target < frame()->args, "Parameter index out of bounds.");

            *stack_.arg(target) = source;
            break;
        }
        case Opcode::LoadModule: {
            const u32 source = read_u32(frame());
            auto target = read_local(frame(), stack_);
            target.set(get_module_member(frame(), source));
            break;
        }
        case Opcode::StoreModule: {
            auto source = read_local(frame(), stack_);
            const u32 index = read_u32(frame());
            set_module_member(frame(), index, source);
            break;
        }
        case Opcode::LoadMember: {
            auto object = read_local(frame(), stack_);
            const u32 name = read_u32(frame());
            auto target = read_local(frame(), stack_);

            auto name_symbol = reg(get_module_member(frame(), name));
            TIRO_CHECK(name_symbol->is<Symbol>(),
                "The module member at index {} must be a symbol.", name);

            auto found = ctx().types().load_member(
                ctx(), object, name_symbol.cast<Symbol>());
            TIRO_CHECK(found, "Failed to load property {} in value of type {}.",
                name_symbol->as<Symbol>().name().view(),
                to_string(object->type())); // TODO nicer

            target.set(*found);
            break;
        }
        case Opcode::StoreMember: {
            auto source = read_local(frame(), stack_);
            auto object = read_local(frame(), stack_);
            const u32 name = read_u32(frame());

            auto name_symbol = reg(get_module_member(frame(), name));
            TIRO_CHECK(name_symbol->is<Symbol>(),
                "The module member at index {} must be a symbol.", name);

            bool ok = ctx().types().store_member(
                ctx(), object, name_symbol.cast<Symbol>(), source);
            TIRO_CHECK(ok, "Failed to store property {} in value of type {}.",
                name_symbol->as<Symbol>().name().view(),
                to_string(object->type())); // TODO nicer
            break;
        }
        case Opcode::LoadTupleMember: {
            auto tuple = read_local(frame(), stack_);
            const u32 index = read_u32(frame());
            auto target = read_local(frame(), stack_);

            TIRO_CHECK(tuple->is<Tuple>(), "The value must be a tuple.");

            // TODO: try_cast on handles
            auto tuple_obj = tuple.cast<Tuple>();
            TIRO_CHECK(index < tuple_obj->size(),
                "Tuple index {} is too large for tuple of size {}.", index,
                tuple_obj->size());

            target.set(tuple_obj->get(index));
            break;
        }
        case Opcode::StoreTupleMember: {
            auto source = read_local(frame(), stack_);
            auto tuple = read_local(frame(), stack_);
            const u32 index = read_u32(frame());

            TIRO_CHECK(tuple->is<Tuple>(), "The value must be a tuple.");

            auto tuple_obj = tuple.cast<Tuple>();
            TIRO_CHECK(index < tuple_obj->size(),
                "Tuple index {} is too large for tuple of size {}.", index,
                tuple_obj->size());

            tuple_obj->set(index, source);
            break;
        }
        case Opcode::LoadIndex: {
            auto array = read_local(frame(), stack_);
            auto index = read_local(frame(), stack_);
            auto target = read_local(frame(), stack_);

            auto value = ctx().types().load_index(ctx(), array, index);
            target.set(value);
            break;
        }
        case Opcode::StoreIndex: {
            auto source = read_local(frame(), stack_);
            auto array = read_local(frame(), stack_);
            auto index = read_local(frame(), stack_);
            ctx().types().store_index(ctx(), array, index, source);
            break;
        }
        case Opcode::LoadClosure: {
            auto target = read_local(frame(), stack_);

            TIRO_CHECK(!frame()->closure.is_null(),
                "Function does not have a closure.");
            target.set(frame()->closure);
            break;
        }
        case Opcode::LoadEnv: {
            auto env = read_local(frame(), stack_);
            const u32 level = read_u32(frame());
            const u32 index = read_u32(frame());
            auto target = read_local(frame(), stack_);

            TIRO_CHECK(
                env->is<Environment>(), "The value is not an environment.");

            auto env_obj = reg(env.cast<Environment>().get());
            if (level != 0)
                env_obj.set(env_obj->parent(level));

            auto value = env_obj->get(index);
            if (TIRO_UNLIKELY(ctx().get_undefined().same(value))) {
                TIRO_ERROR("Closure variable is undefined.");
            }

            target.set(value);
            break;
        }
        case Opcode::StoreEnv: {
            auto source = read_local(frame(), stack_);
            auto env = read_local(frame(), stack_);
            const u32 level = read_u32(frame());
            const u32 index = read_u32(frame());

            TIRO_CHECK(
                env->is<Environment>(), "The value is not an environment.");

            auto env_obj = reg(env.cast<Environment>().get());
            if (level != 0)
                env_obj.set(env_obj->parent(level));

            env_obj->set(index, source);
            break;
        }
        case Opcode::Add: {
            binop(frame(), stack_, [&](auto lhs, auto rhs, auto target) {
                target.set(add(ctx(), lhs, rhs));
            });
            break;
        }
        case Opcode::Sub: {
            binop(frame(), stack_, [&](auto lhs, auto rhs, auto target) {
                target.set(sub(ctx(), lhs, rhs));
            });
            break;
        }
        case Opcode::Mul: {
            binop(frame(), stack_, [&](auto lhs, auto rhs, auto target) {
                target.set(mul(ctx(), lhs, rhs));
            });
            break;
        }
        case Opcode::Div: {
            binop(frame(), stack_, [&](auto lhs, auto rhs, auto target) {
                target.set(div(ctx(), lhs, rhs));
            });
            break;
        }
        case Opcode::Mod: {
            binop(frame(), stack_, [&](auto lhs, auto rhs, auto target) {
                target.set(mod(ctx(), lhs, rhs));
            });
            break;
        }
        case Opcode::Pow: {
            binop(frame(), stack_, [&](auto lhs, auto rhs, auto target) {
                target.set(pow(ctx(), lhs, rhs));
            });
            break;
        }
        case Opcode::UAdd: {
            auto value = read_local(frame(), stack_);
            auto target = read_local(frame(), stack_);
            target.set(unary_plus(ctx(), value));
            break;
        }
        case Opcode::UNeg: {
            auto value = read_local(frame(), stack_);
            auto target = read_local(frame(), stack_);
            target.set(unary_minus(ctx(), value));
            break;
        }

        // TODO
        case Opcode::LSh:
        case Opcode::RSh:
        case Opcode::BAnd:
        case Opcode::BOr:
        case Opcode::BXor:
            TIRO_ERROR("Instruction not implemented yet: {}.", op);

        case Opcode::BNot: {
            auto value = read_local(frame(), stack_);
            auto target = read_local(frame(), stack_);
            target.set(bitwise_not(ctx(), value));
            break;
        }

        case Opcode::Gt: {
            binop(frame(), stack_, [&](auto lhs, auto rhs, auto target) {
                target.set(ctx().get_boolean(compare(lhs, rhs) > 0));
            });
            break;
        }
        case Opcode::Gte: {
            binop(frame(), stack_, [&](auto lhs, auto rhs, auto target) {
                target.set(ctx().get_boolean(compare(lhs, rhs) >= 0));
            });
            break;
        }
        case Opcode::Lt: {
            binop(frame(), stack_, [&](auto lhs, auto rhs, auto target) {
                target.set(ctx().get_boolean(compare(lhs, rhs) < 0));
            });
            break;
        }
        case Opcode::Lte: {
            binop(frame(), stack_, [&](auto lhs, auto rhs, auto target) {
                target.set(ctx().get_boolean(compare(lhs, rhs) <= 0));
            });
            break;
        }
        case Opcode::Eq: {
            binop(frame(), stack_, [&](auto lhs, auto rhs, auto target) {
                target.set(ctx().get_boolean(equal(lhs, rhs)));
            });
            break;
        }
        case Opcode::NEq: {
            binop(frame(), stack_, [&](auto lhs, auto rhs, auto target) {
                target.set(ctx().get_boolean(!equal(lhs, rhs)));
            });
            break;
        }
        case Opcode::LNot: {
            auto value = read_local(frame(), stack_);
            auto target = read_local(frame(), stack_);
            target.set(ctx().get_boolean(ctx().is_truthy(value)));
            break;
        }
        case Opcode::Array: {
            const u32 count = read_u32(frame());
            auto target = read_local(frame(), stack_);
            // TODO HandleSpan
            const Span<const Value> values = stack_.top_values(count);

            target.set(Array::make(ctx(), values));
            stack_.pop_values(count);
            break;
        }
        case Opcode::Tuple: {
            const u32 count = read_u32(frame());
            auto target = read_local(frame(), stack_);
            const Span<const Value> values = stack_.top_values(count);

            target.set(Tuple::make(ctx(), values));
            stack_.pop_values(count);
            break;
        }

        // TODO
        case Opcode::Set:
            TIRO_ERROR("Instruction not implemented yet: {}.", op);

        case Opcode::Map: {
            // FIXME overflow protection
            const u32 count = read_u32(frame());
            auto target = read_local(frame(), stack_);

            TIRO_CHECK(count % 2 == 0,
                "Map instruction requires an even number of arguments (keys "
                "and values).");
            const Span<Value> pairs = stack_.top_values(count);

            auto map = reg(HashTable::make(ctx(), count));
            for (u32 i = 0; i < count; i += 2) {
                auto key = Handle<Value>::from_slot(pairs.data() + i);
                auto value = Handle<Value>::from_slot(pairs.data() + i + 1);
                map->set(ctx(), key, value);
            }

            target.set(map);
            stack_.pop_values(count);
            break;
        }
        case Opcode::Env: {
            auto parent = read_local(frame(), stack_);
            const u32 size = read_u32(frame());
            auto target = read_local(frame(), stack_);

            TIRO_CHECK(parent->is_null() || parent->is<Environment>(),
                "Parent must be null or a another environment.");
            target.set(
                Environment::make(ctx(), size, parent.cast<Environment>()));
            break;
        }
        case Opcode::Closure: {
            auto template_ = read_local(frame(), stack_);
            auto env = read_local(frame(), stack_);
            auto target = read_local(frame(), stack_);

            TIRO_CHECK(template_->is<FunctionTemplate>(),
                "Template must be a function template.");
            TIRO_CHECK(env->is_null() || env->is<Environment>(),
                "Env must be null or an environment.");

            target.set(
                Function::make(ctx(), template_.strict_cast<FunctionTemplate>(),
                    env.cast<Environment>()));
            break;
        }

        case Opcode::Formatter: {
            // Initial capacity would improve performance!
            auto target = read_local(frame(), stack_);
            target.set(StringBuilder::make(ctx()));
            break;
        }
        case Opcode::AppendFormat: {
            auto value = read_local(frame(), stack_);
            auto formatter = read_local(frame(), stack_);
            TIRO_CHECK(formatter->is<StringBuilder>(),
                "Formatter must be a StringBuilder.");

            to_string(ctx(), formatter.strict_cast<StringBuilder>(), value);
            break;
        }
        case Opcode::FormatResult: {
            auto formatter = read_local(frame(), stack_);
            auto target = read_local(frame(), stack_);
            TIRO_CHECK(formatter->is<StringBuilder>(),
                "Formatter must be a StringBuilder.");

            target.set(
                formatter.strict_cast<StringBuilder>()->make_string(ctx()));
            break;
        }
        case Opcode::Copy: {
            auto source = read_local(frame(), stack_);
            auto target = read_local(frame(), stack_);
            target.set(source);
            break;
        }
        case Opcode::Swap: {
            auto a = read_local(frame(), stack_);
            auto b = read_local(frame(), stack_);
            auto t = a.get();
            a.set(b);
            b.set(t);
            break;
        };
        case Opcode::Push: {
            reserve_values(1);

            auto value = read_local(frame(), stack_);
            must_push_value(value);
            break;
        }
        case Opcode::Pop: {
            TIRO_CHECK(
                stack_.top_value_count() > 0, "Cannot pop any more values.");
            stack_.pop_value();
            break;
        }
        case Opcode::PopTo: {
            TIRO_CHECK(
                stack_.top_value_count() > 0, "Cannot pop any more values.");

            auto target = read_local(frame(), stack_);
            target.set(*stack_.top_value());
            stack_.pop_value();
            break;
        }
        case Opcode::Jmp: {
            // TODO static verify
            const u32 target = read_u32(frame());
            jump(frame(), target);
            break;
        }
        case Opcode::JmpTrue: {
            // TODO static verify
            auto value = read_local(frame(), stack_);
            const u32 target = read_u32(frame());
            if (ctx().is_truthy(value)) {
                jump(frame(), target);
            }
            break;
        }
        case Opcode::JmpFalse: {
            // TODO static verify
            auto value = read_local(frame(), stack_);
            const u32 target = read_u32(frame());
            if (!ctx().is_truthy(value)) {
                jump(frame(), target);
            }
            break;
        }
        case Opcode::Call: {
            auto func = read_local(frame(), stack_);
            const u32 count = read_u32(frame());
            switch (call_function(func, count)) {
            case CallResult::Continue:
            case CallResult::Evaluated:
                return CoroutineState::Running;
            case CallResult::Yield:
                return CoroutineState::Waiting;
            }
            break;
        }
        case Opcode::LoadMethod: {
            auto object = read_local(frame(), stack_);
            const u32 name = read_u32(frame());
            auto this_ = read_local(frame(), stack_);
            auto method = read_local(frame(), stack_);

            auto name_symbol = reg(get_module_member(frame(), name));
            TIRO_CHECK(name_symbol->is<Symbol>(),
                "Referenced module member must be a symbol.");

            auto func = reg(Value::null());
            if (auto opt = ctx().types().load_method(
                    ctx(), object, name_symbol.strict_cast<Symbol>());
                TIRO_LIKELY(opt)) {
                func.set(*opt);
            } else {
                TIRO_ERROR("Failed to find attribute {} on object of type {}.",
                    name_symbol->as<Symbol>().name().view(),
                    to_string(object->type()));
            }

            if (func->is<Method>()) {
                this_.set(object);
                method.set(func.strict_cast<Method>()->function());
            } else {
                this_.set(Value::null());
                method.set(func);
            }
            break;
        }
        case Opcode::CallMethod: {
            auto method = read_local(frame(), stack_);
            const u32 count = read_u32(frame());
            switch (call_method(method, count)) {
            case CallResult::Continue:
            case CallResult::Evaluated:
                return CoroutineState::Running;
            case CallResult::Yield:
                return CoroutineState::Waiting;
            }
            break;
        }
        case Opcode::Return: {
            auto value = read_local(frame(), stack_);
            return exit_function(value);
        }
        case Opcode::AssertFail: {
            auto expr = read_local(frame(), stack_);
            auto message = read_local(frame(), stack_);

            TIRO_CHECK(expr->is<String>(),
                "Assertion expression message must be a string value.");
            TIRO_CHECK(message->is_null() || message->is<String>(),
                "Assertion error message must be a string or null.");

            if (message->is_null()) {
                TIRO_ERROR("Assertion `{}` failed.", expr->as<String>().view());
            } else {
                TIRO_ERROR("Assertion `{}` failed: {}",
                    expr->as<String>().view(), message->as<String>().view());
            }
            break;
        }
        }
    }
}

CoroutineState Interpreter::run_async_frame() {
    // We are entering a async function frame. That means that the initial async function
    // (which has suspended the coroutine) has resumed it. The result is ready (within the frame)
    // and we must simply return it to the caller.
    TIRO_ASSERT(frame_->type == FrameType::Async, "Expected an async frame.");
    AsyncFrame* af = static_cast<AsyncFrame*>(frame_);
    return exit_function(af->return_value);
}

Interpreter::CallResult
Interpreter::call_function(Handle<Value> function, u32 argc) {
    TIRO_ASSERT(stack_.top_value_count() >= argc,
        "The value stack must contain all arguments.");
    auto local_function = reg(function.get());
    return enter_function(local_function, argc, false);
}

Interpreter::CallResult
Interpreter::call_method(Handle<Value> method, u32 argc) {
    TIRO_ASSERT(stack_.top_value_count() >= argc + 1,
        "The value stack must contain the all arguments, including `this`.");

    auto local_method = reg(method.get());
    bool is_method = !stack_.top_value(argc)->is_null();
    return enter_function(local_method, argc + (is_method ? 1 : 0), !is_method);
}

Interpreter::CallResult Interpreter::enter_function(
    MutableHandle<Value> function_register, u32 argc, bool pop_one_more) {
again:
    auto frame_flags = [&]() {
        u8 flags = 0;
        flags |= pop_one_more ? FRAME_POP_ONE_MORE : 0;
        return flags;
    };

    const ValueType function_type = function_register->type();
    switch (function_type) {

    // Invokes a user defined function call. A new stack frame is pushed onto the stack, then we return.
    // The interpeter will continue the evaluation in the new function frame.
    // The final return instruction in the callee will restore the stack.
    // If `pop_one_more` is true, an additional value will be popped after returning from the callee.
    // This can happen if a normal function is called via the LOAD_METHOD / CALL_METHOD instruction pair,
    // in that case the unused `this` argument is on the stack but remains unused (it must still be popped, though).
    case ValueType::Function: {
        auto func = function_register.cast<Function>();
        auto tmpl = reg(func->tmpl());
        auto closure = reg(func->closure());
        if (tmpl->params() != argc) {
            TIRO_ERROR(
                "Insufficient number of function arguments (need {}, but have "
                "{}).",
                tmpl->params(), argc);
        }

        push_user_frame(tmpl, closure, frame_flags());
        return CallResult::Continue;
    }

    // Invokes a member function with a bound "this" parameter.
    case ValueType::BoundMethod: {
        auto bound = function_register.cast<BoundMethod>();
        TIRO_ASSERT(bound->type() != ValueType::BoundMethod,
            "Bound methods must not be nested into each other.");

        // Make room for the additional parameter.
        reserve_values(1);
        must_push_value(Value::null());

        // Shift all existing arguments by one slot and
        // add the "this" parameter at the front.
        auto args = stack_.top_values(argc + 1);
        std::memmove(args.data() + 1, args.data(), argc);
        args[0] = bound->object();

        // Replace the callee.
        function_register.set(bound->function());
        ++argc;
        goto again;
    }

    // Invokes a simple native function in a synchronous fashion.
    // This will evaluate the function and return to the caller with the function's result.
    case ValueType::NativeFunction: {
        auto native_func = function_register.cast<NativeFunction>();
        if (argc < native_func->params()) {
            TIRO_ERROR(
                "Invalid number of function arguments (need {}, but have {}).",
                native_func->params(), argc);
        }

        // Make sure that we always have enough space for the return value.
        if (argc == 0 && !pop_one_more)
            reserve_values(1);

        auto result = reg(Value::null());
        NativeFunction::Frame native_frame(
            ctx(), native_func, stack_.top_values(argc), result);
        native_func->function()(native_frame);

        stack_.pop_values(argc + (pop_one_more ? 1 : 0));
        must_push_value(result);
        return CallResult::Evaluated;
    }

    // Invokes a native async function. The function call below should
    // start an asynchronous action and suspend the coroutine. Once
    // the coroutine is resumed again, the interpreter will see an AsyncFrame
    // and return with the result found there.
    case ValueType::NativeAsyncFunction: {
        auto native_func = function_register.cast<NativeAsyncFunction>();
        if (argc < native_func->params()) {
            TIRO_ERROR(
                "Invalid number of function arguments (need {}, but have {}).",
                native_func->params(), argc);
        }

        push_async_frame(native_func, argc, frame_flags());

        AsyncFrame* af = static_cast<AsyncFrame*>(frame_);
        NativeAsyncFunction::Frame native_frame(ctx(),
            Handle<Coroutine>::from_slot(&current_),
            Handle<NativeAsyncFunction>::from_slot(&af->func), stack_.args(),
            MutableHandle<Value>::from_slot(&af->return_value));
        native_func->function()(std::move(native_frame));

        TIRO_ASSERT(current_.state() == CoroutineState::Running,
            "The async native function must not alter the coroutine state in "
            "its initiating call.");
        return CallResult::Yield;
    }

    default:
        TIRO_ERROR("Cannot call object of type {} as a function.",
            to_string(function_type));
    }
}

CoroutineState Interpreter::exit_function(Value return_value) {
    TIRO_ASSERT(frame_, "Invalid frame.");

    u32 pop_args = frame_->args;
    if (frame_->flags & FRAME_POP_ONE_MORE) {
        // Normal function invoked via CALL_METHOD, pop the additional value,
        // see the comment for call_method.
        ++pop_args;
    }

    pop_frame();
    TIRO_ASSERT(stack_.value_capacity_remaining() > 0,
        "Popping the frame must make at least one value slot available.");

    stack_.pop_values(pop_args);   // Function arguments
    must_push_value(return_value); // Safe, see assertion above.
    return stack_.top_frame() ? CoroutineState::Running : CoroutineState::Done;
}

void Interpreter::must_push_value(Value v) {
    if (TIRO_LIKELY(stack_.push_value(v)))
        return;

    // Programming error; use reserve_values() correctly.
    TIRO_ERROR(
        "The stack is full "
        "(failed to reserve enough capacity beforehand).");
}

void Interpreter::push_user_frame(
    Handle<FunctionTemplate> tmpl, Handle<Environment> closure, u8 flags) {
    if (TIRO_UNLIKELY(
            !stack_.push_user_frame(tmpl.get(), closure.get(), flags))) {
        grow_stack();
        [[maybe_unused]] bool ok = stack_.push_user_frame(
            tmpl.get(), closure.get(), flags);
        TIRO_ASSERT(ok, "Failed to push frame after stack growth.");
    }
    update_frame();
}

void Interpreter::push_async_frame(
    Handle<NativeAsyncFunction> func, u32 argc, u8 flags) {
    if (TIRO_UNLIKELY(!stack_.push_async_frame(func.get(), argc, flags))) {
        grow_stack();
        [[maybe_unused]] bool ok = stack_.push_async_frame(
            func.get(), argc, flags);
        TIRO_ASSERT(ok, "Failed to push frame after stack growth.");
    }
    update_frame();
}

void Interpreter::pop_frame() {
    TIRO_ASSERT(
        stack_.top_frame(), "Cannot pop a frame from an empty call stack.");
    TIRO_ASSERT(stack_.top_frame() == frame_, "Unexpected current frame.");

    stack_.pop_frame();
    update_frame();
}

void Interpreter::update_frame() {
    TIRO_ASSERT(stack_, "Null stack.");
    frame_ = stack_.top_frame();
}

void Interpreter::reserve_values(u32 value_count) {
    while (stack_.value_capacity_remaining() < value_count) {
        grow_stack();
    }
}

void Interpreter::grow_stack() {
    u32 next_size;
    if (TIRO_UNLIKELY(!checked_mul<u32>(stack_.object_size(), 2, next_size)))
        TIRO_ERROR("Overflow in stack size computation.");

    if (TIRO_UNLIKELY(next_size > CoroutineStack::max_size))
        TIRO_ERROR("Stack overflow.");

    Root old_stack(ctx(), stack_);
    Root new_stack(ctx(), CoroutineStack::grow(ctx(), old_stack, next_size));

    current_.stack(new_stack);
    stack_ = new_stack;

    TIRO_ASSERT(frame_->type == FrameType::User,
        "Other frame types not implemented yet."); // TODO
    frame_ = static_cast<UserFrame*>(stack_.top_frame());
};

void Interpreter::jump(UserFrame* frame, u32 offset) {
    TIRO_ASSERT(
        offset_in_bounds(frame, offset), "Jump destination is out of bounds.");
    frame->pc = frame->tmpl.code().data() + offset;
}

Value* Interpreter::allocate_register_slot() {
    // This error would be a programming error, the maximum number of
    // internal registers has a static upper limit.
    TIRO_CHECK(registers_used_ < registers_.size(),
        "No more registers: all are already allocated.");
    return &registers_[registers_used_++];
}

Opcode read_op(UserFrame* frame) {
    // TODO static verify
    TIRO_ASSERT(readable(frame) >= 1, "Not enough available bytes.");

    u8 opcode = *frame->pc++;
    TIRO_ASSERT(valid_opcode(opcode), "Invalid opcode.");
    return static_cast<Opcode>(opcode);
};

i64 read_i64(UserFrame* frame) {
    // TODO static verify
    TIRO_ASSERT(readable(frame) >= 8, "Not enough available bytes.");
    return static_cast<i64>(read_big_endian<u64>(frame->pc));
};

f64 read_f64(UserFrame* frame) {
    // TODO static verify
    TIRO_ASSERT(readable(frame) >= 8, "Not enough available bytes.");
    // FIXME float serialization in some helper function, see also compiler/binary.hpp
    static_assert(sizeof(f64) == sizeof(u64));
    u64 as_u64 = read_big_endian<u64>(frame->pc);
    f64 d;
    std::memcpy(&d, &as_u64, sizeof(f64));
    return d;
};

u32 read_u32(UserFrame* frame) {
    // TODO static verify
    TIRO_ASSERT(readable(frame) >= 4, "Not enough available bytes.");
    return read_big_endian<u32>(frame->pc);
};

MutableHandle<Value> read_local(UserFrame* frame, CoroutineStack stack) {
    // TODO static verify local index.
    const u32 local = read_u32(frame);
    return MutableHandle<Value>::from_slot(stack.local(local));
}

[[maybe_unused]] size_t readable(UserFrame* frame) {
    return static_cast<size_t>(frame->tmpl.code().view().end() - frame->pc);
}

[[maybe_unused]] bool offset_in_bounds(UserFrame* frame, u32 offset) {
    return offset < frame->tmpl.code().size();
}

} // namespace tiro::vm
