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
    size_t argc() const { return tiro_sync_frame_argc(raw_frame_); }

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
    size_t argc() const { return tiro_async_frame_argc(raw_frame_); }

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

} // namespace tiro

#endif // TIROPP_FUNCTIONS_HPP_INCLUDED
