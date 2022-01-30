#ifndef TIROPP_FUNCTIONS_HPP_INCLUDED
#define TIROPP_FUNCTIONS_HPP_INCLUDED

#include "tiropp/def.hpp"
#include "tiropp/detail/handle_check.hpp"
#include "tiropp/detail/translate.hpp"
#include "tiropp/objects.hpp"

#include "tiro/functions.h"

namespace tiro {

/// Represents the call frame of a synchronous function call.
/// References to sync_frames are only valid from within the surrounding function call.
class sync_frame final {
public:
    sync_frame(tiro_vm_t raw_vm, tiro_sync_frame_t raw_frame)
        : raw_vm_(raw_vm)
        , raw_frame_(raw_frame) {
        TIRO_ASSERT(raw_vm);
        TIRO_ASSERT(raw_frame);
    }

    sync_frame(const sync_frame&) = delete;
    sync_frame& operator=(const sync_frame&) = delete;

    /// Returns the number of arguments passed to this function call.
    size_t arg_count() const { return tiro_sync_frame_arg_count(raw_frame_); }

    /// Returns the argument at the given index (`0 <= index < argc`).
    handle arg(size_t index) const {
        handle result(raw_vm_);
        detail::check_handles(raw_vm_, result);
        tiro_sync_frame_arg(raw_frame_, index, result.raw_handle(), error_adapter());
        return result;
    }

    /// Returns the closure value referenced by this function (if any).
    handle closure() const {
        handle result(raw_vm_);
        detail::check_handles(raw_vm_, result);
        tiro_sync_frame_closure(raw_frame_, result.raw_handle(), error_adapter());
        return result;
    }

    tiro_vm_t raw_vm() const { return raw_vm_; }

    tiro_sync_frame_t raw_frame() const { return raw_frame_; }

private:
    // Both are unowned.
    tiro_vm_t raw_vm_;
    tiro_sync_frame_t raw_frame_;
};

/// Constructs a new function object with the given name that will invoke the native function when called.
/// `argc` is the number of arguments required for calling `Function`.
/// `closure` may be an arbitrary value that will be passed to the function on every invocation.
///
/// Synchronous functions are appropriate for simple, nonblocking operations.
/// Use asynchronous functions for long running operations (such as network I/O) instead.
///
/// `Function` will receive two arguments when invoked:
///     - A reference to the vm (`vm&`).
///     - A reference to the call frame (`sync_frame&`).
///       Use this reference to access call arguments.
/// Both references may only be used during the function call.
/// The function should return its return value as a handle.
template<auto Function>
function make_sync_function(vm& v, const string& name, size_t argc, const handle& closure) {
    constexpr tiro_sync_function_t func = [](tiro_vm_t raw_vm, tiro_sync_frame_t raw_frame) {
        try {
            vm& inner_v = vm::unsafe_from_raw_vm(raw_vm);
            sync_frame frame(raw_vm, raw_frame);
            handle result = Function(inner_v, frame);
            detail::check_handles(raw_vm, result);
            tiro_sync_frame_return_value(raw_frame, result.raw_handle(), error_adapter());
        } catch (const std::exception& e) {
            std::string_view message(e.what());
            tiro_sync_frame_panic_msg(raw_frame, detail::to_raw(message), nullptr);
        } catch (...) {
            tiro_sync_frame_panic_msg(raw_frame, detail::to_raw("unknown exception"), nullptr);
        }
    };

    handle result(v.raw_vm());
    detail::check_handles(v.raw_vm(), name, closure, result);
    tiro_make_sync_function(v.raw_vm(), name.raw_handle(), func, argc, closure.raw_handle(),
        result.raw_handle(), error_adapter());
    return function(std::move(result));
}

/// Represents the call frame of a asynchronous function call.
/// The lifetime of async_frames is dynamic.
/// They usually outlive their surrounding native function call, which causes the calling tiro coroutine to sleep.
/// The coroutine resumes when the frame's return value has been set.
///
/// Frames must not outlive their associated vm.
class async_frame final {
public:
    async_frame(tiro_vm_t raw_vm, tiro_async_frame_t raw_frame)
        : raw_vm_(raw_vm)
        , raw_frame_(raw_frame) {
        TIRO_ASSERT(raw_vm);
        TIRO_ASSERT(raw_frame);
    }

    async_frame(async_frame&&) noexcept = default;
    async_frame& operator=(async_frame&&) noexcept = default;

    /// Returns the number of arguments passed to this function call.
    size_t arg_count() const { return tiro_async_frame_arg_count(raw_frame_); }

    /// Returns the argument at the given index (`0 <= index < argc`).
    handle arg(size_t index) const {
        handle result(raw_vm_);
        detail::check_handles(raw_vm_, result);
        tiro_async_frame_arg(raw_frame_, index, result.raw_handle(), error_adapter());
        return result;
    }

    /// Returns the closure value referenced by this function (if any).
    handle closure() const {
        handle result(raw_vm_);
        detail::check_handles(raw_vm_, result);
        tiro_async_frame_closure(raw_frame_, result.raw_handle(), error_adapter());
        return result;
    }

    /// Sets the return value for this function call frame to the given `value`.
    void return_value(const handle& value) {
        detail::check_handles(raw_vm_, value);
        tiro_async_frame_return_value(raw_frame_, value.raw_handle(), error_adapter());
    }

    /// Signals a panic from this function call frame using the given message.
    void panic_msg(std::string_view message) {
        tiro_async_frame_panic_msg(raw_frame_, detail::to_raw(message), error_adapter());
    }

    tiro_vm_t raw_vm() const { return raw_vm_; }

    tiro_async_frame_t raw_frame() const { return raw_frame_; }

private:
    tiro_vm_t raw_vm_; // Unowned
    detail::resource_holder<tiro_async_frame_t, tiro_async_frame_free> raw_frame_;
};

/// Constructs a new function object with the given name that will invoke the native function when called.
/// `argc` is the number of arguments required for calling `Function`.
/// `closure` may be an arbitrary value that will be passed to the function on every invocation.
///
/// `Function` will receive two arguments when invoked:
///     - A reference to the vm (`vm&`).
///     - A call frame value (`async_frame`).
///       Use this value to access call arguments and to set the return value.
template<auto Function>
function make_async_function(vm& v, const string& name, size_t argc, const handle& closure) {
    constexpr tiro_async_function_t func = [](tiro_vm_t raw_vm, tiro_async_frame_t raw_frame) {
        vm& inner_v = vm::unsafe_from_raw_vm(raw_vm);
        async_frame frame(raw_vm, raw_frame);
        try {
            Function(inner_v, std::move(frame));
        } catch (...) {
            // FIXME: Bad design :(
            // Cannot panic here because the frame was already moved and the callee may have freed it.
            std::terminate();
        }
    };

    handle result(v.raw_vm());
    detail::check_handles(v.raw_vm(), name, closure, result);
    tiro_make_async_function(v.raw_vm(), name.raw_handle(), func, argc, closure.raw_handle(),
        result.raw_handle(), error_adapter());
    return function(std::move(result));
}

// TODO docs
class resumable_frame final {
public:
    /// Lists well known state values used by resumable functions.
    ///
    /// All positive integers can be used freely by the application.
    enum frame_state : int {
        /// The initial state value.
        start = TIRO_RESUMABLE_STATE_START,

        /// Signals that the function has finished executing.
        end = TIRO_RESUMABLE_STATE_END,
    };

    resumable_frame(tiro_vm_t raw_vm, tiro_resumable_frame_t raw_frame)
        : raw_vm_(raw_vm)
        , raw_frame_(raw_frame) {}

    resumable_frame(const resumable_frame&) = delete;
    resumable_frame& operator=(const resumable_frame&) = delete;

    /// Returns the number of arguments passed to this function call.
    size_t arg_count() const { return tiro_resumable_frame_arg_count(raw_frame_); }

    /// Returns the argument at the given index (`0 <= index < argc`).
    handle arg(size_t index) const {
        handle result(raw_vm_);
        detail::check_handles(raw_vm_, result);
        tiro_resumable_frame_arg(raw_frame_, index, result.raw_handle(), error_adapter());
        return result;
    }

    /// Returns the closure value referenced by this function (if any).
    handle closure() const {
        handle result(raw_vm_);
        detail::check_handles(raw_vm_, result);
        tiro_resumable_frame_closure(raw_frame_, result.raw_handle(), error_adapter());
        return result;
    }

    /// Returns the number of local values available to the function frame.
    size_t local_count() const { return tiro_resumable_frame_local_count(raw_frame_); }

    /// Returns the current value of the local slot with the given index.
    handle local(size_t index) const {
        handle result(raw_vm_);
        detail::check_handles(raw_vm_, result);
        tiro_resumable_frame_local(raw_frame_, index, result.raw_handle(), error_adapter());
        return result;
    }

    /// Sets the current value of the local slot with the given index to `value`.
    void set_local(size_t index, const handle& value) {
        detail::check_handles(raw_vm_, value);
        tiro_resumable_frame_set_local(raw_frame_, index, value.raw_handle(), error_adapter());
    }

    /// Returns the current state of this frame.
    int state() const { return tiro_resumable_frame_state(raw_frame_); }

    /// Sets the current state of this frame.
    /// It is usually not necessary to invoke this function directly as changing the state
    /// is also implied by other methods like `invoke()` and `return_value()`.
    ///
    /// The calling native function should return after altering the state.
    /// The new state will be active when the native function is called for the next time.
    ///
    /// Note that a few states have special meaning (see `resumable_frame_state`).
    ///
    /// \param next_state The new state value
    void set_state(int next_state) {
        tiro_resumable_frame_set_state(raw_frame_, next_state, error_adapter());
    }

    /// Signals the vm that the function `func` shall be invoked with the given arguments in `args`.
    /// `func` will be invoked after the native function returned to the vm.
    /// The current native function will be called again when `func` has itself returned, and its return value
    /// will be accessible via `invoke_return()`.
    ///
    /// Calling this function implies a state change to `next_state`, which will be the frame's state
    /// when the native function is called again after `func`'s execution.
    ///
    /// \param next_state The new state value
    /// \param func The target function to invoke
    /// \param args The call arguments
    void invoke(int next_state, const function& func, const tuple& args) {
        detail::check_handles(raw_vm_, func, args);
        tiro_resumable_frame_invoke(
            raw_frame_, next_state, func.raw_handle(), args.raw_handle(), error_adapter());
    }

    /// Like above, but calls the given function without any arguments.
    void invoke(int next_state, const function& func) {
        detail::check_handles(raw_vm_, func);
        tiro_resumable_frame_invoke(
            raw_frame_, next_state, func.raw_handle(), nullptr, error_adapter());
    }

    /// Returns the result of the last function call made via `invoke()`.
    /// Only returns a useful value when the native function is called again for the first time
    /// after calling `invoke()` and returning to the vm.
    handle invoke_return() const {
        handle result(raw_vm_);
        detail::check_handles(raw_vm_, result);
        tiro_resumable_frame_invoke_return(raw_frame_, result.raw_handle(), error_adapter());
        return result;
    }

    /// Sets the return value for the given function call frame to the given `value`.
    /// The call frame's state is also set to `END` as a result of this call.
    ///
    void return_value(const handle& value) {
        detail::check_handles(raw_vm_, value);
        tiro_resumable_frame_return_value(raw_frame_, value.raw_handle(), error_adapter());
    }

    /// Signals a panic from the given function call frame.
    /// The call frame's state is also set to `END` as a result of this call.
    ///
    /// TODO: Allow user defined exception objects instead of plain string?
    void panic_msg(std::string_view message) {
        tiro_resumable_frame_panic_msg(raw_frame_, detail::to_raw(message), error_adapter());
    }

    tiro_vm_t raw_vm() const { return raw_vm_; }

    tiro_resumable_frame_t raw_frame() const { return raw_frame_; }

private:
    // Both are unowned.
    tiro_vm_t raw_vm_;
    tiro_resumable_frame_t raw_frame_;
};

/// Creates a new resumable function with the given parameters.
template<auto Function>
function make_resumable_function(
    vm& v, const string& name, size_t argc, size_t locals, const handle& closure) {
    constexpr tiro_resumable_function_t func = [](tiro_vm_t raw_vm,
                                                   tiro_resumable_frame_t raw_frame) {
        try {
            vm& inner_v = vm::unsafe_from_raw_vm(raw_vm);
            resumable_frame frame(raw_vm, raw_frame);
            (void) Function(inner_v, frame);
        } catch (const std::exception& e) {
            std::string_view message(e.what());
            tiro_resumable_frame_panic_msg(raw_frame, detail::to_raw(message), nullptr);
        } catch (...) {
            tiro_resumable_frame_panic_msg(raw_frame, detail::to_raw("unknown exception"), nullptr);
        }
    };

    handle result(v.raw_vm());
    detail::check_handles(v.raw_vm(), name, closure, result);

    tiro_resumable_frame_desc_t desc{};
    desc.name = name.raw_handle();
    desc.arg_count = argc;
    desc.local_count = locals;
    desc.closure = closure.raw_handle();
    desc.func = func;
    tiro_make_resumable_function(v.raw_vm(), &desc, result.raw_handle(), error_adapter());
    return function(std::move(result));
}

} // namespace tiro

#endif // TIROPP_FUNCTIONS_HPP_INCLUDED
