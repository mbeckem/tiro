#include "api/internal.hpp"

#include "tiro/functions.h"

#include "vm/objects/function.hpp"
#include "vm/objects/native.hpp"

using namespace tiro;
using namespace tiro::api;

static vm::NativeFunctionStorage wrap_function(tiro_sync_function_t sync_func);
static vm::NativeFunctionStorage wrap_function(tiro_async_function_t async_func);
static vm::NativeFunctionStorage wrap_function(tiro_resumable_function_t resumable_func);

template<typename FunctionPtr>
static void make_native_function(tiro_vm_t vm, tiro_handle_t name, FunctionPtr func, size_t argc,
    tiro_handle_t closure, tiro_handle_t result, tiro_error_t* err);

template<typename Frame>
static size_t get_argc(Frame* frame);

template<typename Frame>
static void get_arg(Frame frame, size_t index, tiro_handle_t result, tiro_error_t* err);

template<typename Frame>
void get_closure(Frame frame, tiro_handle_t result, tiro_error_t* err);

size_t tiro_sync_frame_argc(tiro_sync_frame_t frame) {
    return get_argc(frame);
}

void tiro_sync_frame_arg(
    tiro_sync_frame_t frame, size_t index, tiro_handle_t result, tiro_error_t* err) {
    return get_arg(frame, index, result, err);
}

void tiro_sync_frame_closure(tiro_sync_frame_t frame, tiro_handle_t result, tiro_error_t* err) {
    return get_closure(frame, result, err);
}

void tiro_sync_frame_return_value(tiro_sync_frame_t frame, tiro_handle_t value, tiro_error_t* err) {
    return entry_point(err, [&] {
        if (!frame || !value)
            return TIRO_REPORT(err, TIRO_ERROR_BAD_ARG);

        auto internal_frame = to_internal(frame);
        auto value_handle = to_internal(value);
        internal_frame->return_value(*value_handle);
    });
}

void tiro_sync_frame_panic_msg(tiro_sync_frame_t frame, tiro_string message, tiro_error_t* err) {
    return entry_point(err, [&] {
        if (!frame || !valid_string(message))
            return TIRO_REPORT(err, TIRO_ERROR_BAD_ARG);

        auto internal_frame = to_internal(frame);
        auto message_view = to_internal(message);

        // TODO: Ensure that function always goes into panic mode, even if the following throws!
        auto& ctx = internal_frame->ctx();
        vm::Scope sc(ctx);
        vm::Local message_string = sc.local(vm::String::make(ctx, message_view));
        internal_frame->panic(vm::Exception::make(ctx, message_string));
    });
}

void tiro_make_sync_function(tiro_vm_t vm, tiro_handle_t name, tiro_sync_function_t func,
    size_t argc, tiro_handle_t closure, tiro_handle_t result, tiro_error_t* err) {
    return make_native_function(vm, name, func, argc, closure, result, err);
}

void tiro_async_frame_free(tiro_async_frame_t frame) {
    delete to_internal(frame);
}

tiro_vm_t tiro_async_frame_vm(tiro_async_frame_t frame) {
    return entry_point(nullptr, nullptr, [&]() -> tiro_vm_t {
        if (!frame)
            return nullptr;

        return vm_from_context(to_internal(frame)->ctx());
    });
}

size_t tiro_async_frame_argc(tiro_async_frame_t frame) {
    return get_argc(frame);
}

void tiro_async_frame_arg(
    tiro_async_frame_t frame, size_t index, tiro_handle_t result, tiro_error_t* err) {
    return get_arg(frame, index, result, err);
}

void tiro_async_frame_closure(tiro_async_frame_t frame, tiro_handle_t result, tiro_error_t* err) {
    return get_closure(frame, result, err);
}

void tiro_async_frame_return_value(
    tiro_async_frame_t frame, tiro_handle_t value, tiro_error_t* err) {
    return entry_point(err, [&] {
        if (!frame || !value)
            return TIRO_REPORT(err, TIRO_ERROR_BAD_ARG);

        auto internal_frame = to_internal(frame);
        auto value_handle = to_internal(value);
        internal_frame->return_value(*value_handle);
    });
}

void tiro_async_frame_panic_msg(tiro_async_frame_t frame, tiro_string message, tiro_error_t* err) {
    return entry_point(err, [&] {
        if (!frame || !valid_string(message))
            return TIRO_REPORT(err, TIRO_ERROR_BAD_ARG);

        auto internal_frame = to_internal(frame);
        auto message_view = to_internal(message);

        // TODO: Ensure that function always goes into panic mode, even if the following throws!
        auto& ctx = internal_frame->ctx();
        vm::Scope sc(ctx);
        vm::Local message_string = sc.local(vm::String::make(ctx, message_view));
        internal_frame->panic(vm::Exception::make(ctx, message_string));
    });
}

void tiro_make_async_function(tiro_vm_t vm, tiro_handle_t name, tiro_async_function_t func,
    size_t argc, tiro_handle_t closure, tiro_handle_t result, tiro_error_t* err) {
    return make_native_function(vm, name, func, argc, closure, result, err);
}

int tiro_resumable_frame_state(tiro_resumable_frame_t frame) {
    return entry_point(nullptr, 0, [&]() -> int {
        if (!frame)
            return 0;

        return to_internal(frame)->state();
    });
}

void tiro_resumable_frame_set_state(
    tiro_resumable_frame_t frame, int next_state, tiro_error_t* err) {
    return entry_point(err, [&]() {
        if (!frame || next_state == TIRO_RESUMABLE_CLEANUP)
            return TIRO_REPORT(err, TIRO_ERROR_BAD_ARG);

        to_internal(frame)->set_state(next_state);
    });
}

size_t tiro_resumable_frame_argc(tiro_resumable_frame_t frame) {
    return get_argc(frame);
}

void tiro_resumable_frame_arg(
    tiro_resumable_frame_t frame, size_t index, tiro_handle_t result, tiro_error_t* err) {
    return get_arg(frame, index, result, err);
}

void tiro_resumable_frame_closure(
    tiro_resumable_frame_t frame, tiro_handle_t result, tiro_error_t* err) {
    return get_closure(frame, result, err);
}

void tiro_resumable_frame_invoke(tiro_resumable_frame_t frame, int next_state, tiro_handle_t func,
    tiro_handle_t args, tiro_error_t* err) {
    return entry_point(err, [&]() {
        if (!frame || !func || next_state == TIRO_RESUMABLE_CLEANUP)
            return TIRO_REPORT(err, TIRO_ERROR_BAD_ARG);

        auto internal_frame = to_internal(frame);
        if (internal_frame->state() == TIRO_RESUMABLE_CLEANUP)
            return TIRO_REPORT(err, TIRO_ERROR_BAD_STATE);

        auto maybe_func = to_internal(func).try_cast<vm::Function>();
        if (!maybe_func)
            return TIRO_REPORT(err, TIRO_ERROR_BAD_TYPE);

        auto maybe_args = to_internal_maybe(args);
        if (maybe_args) {
            auto args_handle = maybe_args.handle();
            if (!args_handle->is<vm::Null>() && args_handle->is<vm::Tuple>())
                return TIRO_REPORT(err, TIRO_ERROR_BAD_TYPE);
        }

        auto raw_func = *maybe_func.handle();
        auto raw_args = [&]() -> vm::Nullable<vm::Tuple> {
            auto maybe_tuple = maybe_args.try_cast<vm::Tuple>();
            if (!maybe_tuple)
                return vm::Null();
            return *maybe_tuple.handle();
        }();
        internal_frame->invoke(next_state, raw_func, raw_args);
    });
}

void tiro_resumable_frame_invoke_return(
    tiro_resumable_frame_t frame, tiro_handle_t result, tiro_error_t* err) {
    return entry_point(err, [&]() {
        if (!frame || !result)
            return TIRO_REPORT(err, TIRO_ERROR_BAD_ARG);

        auto internal_frame = to_internal(frame);
        auto result_handle = to_internal(result);
        result_handle.set(internal_frame->invoke_return());
    });
}

void tiro_resumable_frame_return_value(
    tiro_resumable_frame_t frame, tiro_handle_t value, tiro_error_t* err) {
    return entry_point(err, [&]() {
        if (!frame || !value)
            return TIRO_REPORT(err, TIRO_ERROR_BAD_ARG);

        auto internal_frame = to_internal(frame);
        if (internal_frame->state() == TIRO_RESUMABLE_CLEANUP)
            return TIRO_REPORT(err, TIRO_ERROR_BAD_STATE);

        auto internal_value = to_internal(value);
        internal_frame->return_value(*internal_value);
    });
}

void tiro_resumable_frame_panic_msg(
    tiro_resumable_frame_t frame, tiro_string message, tiro_error_t* err) {
    return entry_point(err, [&]() {
        if (!frame || !valid_string(message))
            return TIRO_REPORT(err, TIRO_ERROR_BAD_ARG);

        auto internal_frame = to_internal(frame);
        if (internal_frame->state() == TIRO_RESUMABLE_CLEANUP)
            return TIRO_REPORT(err, TIRO_ERROR_BAD_STATE);

        auto message_view = to_internal(message);

        // TODO: Ensure that function always goes into panic mode, even if the following throws!
        auto& ctx = internal_frame->ctx();
        vm::Scope sc(ctx);
        vm::Local message_string = sc.local(vm::String::make(ctx, message_view));
        internal_frame->panic(vm::Exception::make(ctx, message_string));
    });
}

TIRO_API void
tiro_make_resumable_function(tiro_vm_t vm, tiro_handle_t name, tiro_resumable_function_t func,
    size_t argc, tiro_handle_t closure, tiro_handle_t result, tiro_error_t* err) {
    return make_native_function(vm, name, func, argc, closure, result, err);
}

vm::NativeFunctionStorage wrap_function(tiro_sync_function_t sync_func) {
    struct Function {
        tiro_sync_function_t func;

        void operator()(vm::SyncFrameContext& frame) {
            func(vm_from_context(frame.ctx()), to_external(&frame));
        }
    };

    return vm::NativeFunctionStorage::sync(Function{sync_func});
}

vm::NativeFunctionStorage wrap_function(tiro_async_function_t async_func) {
    struct Function {
        tiro_async_function_t func;

        void operator()(vm::AsyncFrameContext frame) {
            auto dynamic_frame = std::make_unique<vm::AsyncFrameContext>(std::move(frame));
            func(vm_from_context(frame.ctx()), to_external(dynamic_frame.release()));
        }
    };

    return vm::NativeFunctionStorage::async(Function{async_func});
}

vm::NativeFunctionStorage wrap_function(tiro_resumable_function_t resumable_func) {
    struct Function {
        tiro_resumable_function_t func;

        void operator()(vm::ResumableFrameContext& frame) {
            func(vm_from_context(frame.ctx()), to_external(&frame));
        }
    };

    return vm::NativeFunctionStorage::resumable(Function{resumable_func});
}

template<typename FunctionPtr>
void make_native_function(tiro_vm_t vm, tiro_handle_t name, FunctionPtr func, size_t argc,
    tiro_handle_t closure, tiro_handle_t result, tiro_error_t* err) {
    return entry_point(err, [&] {
        if (!vm || !name || !func || !result)
            return TIRO_REPORT(err, TIRO_ERROR_BAD_ARG);

        // Internally argc is uint32_t, this must always be smaller than 2 ** 32.
        static constexpr size_t max_argc = 1024;
        if (argc > max_argc)
            return TIRO_REPORT(err, TIRO_ERROR_BAD_ARG);

        vm::Context& ctx = vm->ctx;

        auto maybe_name = to_internal(name).try_cast<vm::String>();
        if (!maybe_name)
            return TIRO_REPORT(err, TIRO_ERROR_BAD_TYPE);

        auto name_handle = maybe_name.handle();
        auto maybe_closure = to_internal_maybe(closure);
        auto result_handle = to_internal(result);

        result_handle.set(vm::NativeFunction::make(
            ctx, name_handle, maybe_closure, static_cast<u32>(argc), 0, wrap_function(func)));
    });
}

template<typename Frame>
size_t get_argc(Frame* frame) {
    return entry_point(nullptr, 0, [&]() -> size_t {
        if (!frame)
            return 0;

        return to_internal(frame)->arg_count();
    });
}

template<typename Frame>
static void get_arg(Frame frame, size_t index, tiro_handle_t result, tiro_error_t* err) {
    return entry_point(err, [&] {
        if (!frame || !result)
            return TIRO_REPORT(err, TIRO_ERROR_BAD_ARG);

        auto internal_frame = to_internal(frame);
        if (index >= internal_frame->arg_count())
            return TIRO_REPORT(err, TIRO_ERROR_OUT_OF_BOUNDS);

        auto result_handle = to_internal(result);
        result_handle.set(internal_frame->arg(index));
    });
}

template<typename Frame>
void get_closure(Frame frame, tiro_handle_t result, tiro_error_t* err) {
    return entry_point(err, [&] {
        if (!frame || !result)
            return TIRO_REPORT(err, TIRO_ERROR_BAD_ARG);

        auto internal_frame = to_internal(frame);
        auto result_handle = to_internal(result);
        result_handle.set(internal_frame->closure());
    });
}
