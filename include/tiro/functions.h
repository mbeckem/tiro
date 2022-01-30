#ifndef TIRO_FUNCTIONS_H_INCLUDED
#define TIRO_FUNCTIONS_H_INCLUDED

/**
 * \file
 * \brief Functions and type definitions for creating native functions that are callable from tiro.
 */

#include "tiro/def.h"
#include "tiro/error.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * The prototype of a native function callback that provides a synchronous tiro function.
 * This type of native function is appropriate for simple, nonblocking operations.
 * Use the more complex asynchronous API instead if the operation has the potential of blocking the process.
 *
 * Note that this API does not allow for custom native userdata. Use native objects instead and pass them in the closure.
 *
 * \param vm
 *      The virtual machine the function is executing on.
 * \param frame
 *      The function call frame. Use `tiro_sync_frame_arg` and `tiro_sync_frame_arg_count` to access the function
 *      call arguments. Call `tiro_sync_frame_result` to set the return value (it defaults to null if not set).
 *      The closure is also available by calling `tiro_sync_frame_closure`.
 *      The frame value is only valid for the duration of the function call.
 */
typedef void (*tiro_sync_function_t)(tiro_vm_t vm, tiro_sync_frame_t frame);

/**
 * Returns the number of function call arguments present in the given frame.
 * Returns 0 for invalid input arguments.
 */
TIRO_API size_t tiro_sync_frame_arg_count(tiro_sync_frame_t frame);

/**
 * Stores the function call argument with the given `index` into `result`.
 * Returns `TIRO_ERROR_OUT_OF_BOUNDS` if the argument index is invalid.
 */
TIRO_API void
tiro_sync_frame_arg(tiro_sync_frame_t frame, size_t index, tiro_handle_t result, tiro_error_t* err);

/** Returns the closure value which was specified when the function was created. */
TIRO_API void
tiro_sync_frame_closure(tiro_sync_frame_t frame, tiro_handle_t result, tiro_error_t* err);

/** Sets the return value for the given function call frame to the given `value`. */
TIRO_API void
tiro_sync_frame_return_value(tiro_sync_frame_t frame, tiro_handle_t value, tiro_error_t* err);

/**
 * Signals a panic from the given function call frame.
 * TODO: Allow user defined exception objects instead of plain string?
 */
TIRO_API void
tiro_sync_frame_panic_msg(tiro_sync_frame_t frame, tiro_string message, tiro_error_t* err);

/**
 * Constructs a new function object with the given name that will invoke the native function `func` when called.
 * `argc` is the number of arguments required for calling `func`. `closure` may be an arbitrary value
 * that will be passed to the function on every invocation.
 *
 * On success, the new function will be stored in `result`.
 * Returns `TIRO_BAD_TYPE` if `name` is not a string. Returns `TIRO_BAD_ARG` if the the requested number of parameters
 * is too large. The current maximum is `1024`.
 */
TIRO_API void tiro_make_sync_function(tiro_vm_t vm, tiro_handle_t name, tiro_sync_function_t func,
    size_t argc, tiro_handle_t closure, tiro_handle_t result, tiro_error_t* err);

/**
 * The prototype of a native function callback that provides an asynchronous tiro function.
 * Functions of this type should be used to implement long running operations that would otherwise block
 * the calling coroutine (for example, a socket read or write).
 *
 * Calling an asynchronous function will pause ("yield") the calling coroutine. It will be resumed when
 * a result is provided to the frame object, e.g. by calling `tiro_async_frame_return_value`. Attempting to resume
 * a coroutine multiple times is an error.
 *
 * Note that this API does not allow for custom native userdata. Use native objects instead and pass them in the closure.
 *
 * \param vm
 *      The virtual machine the function is executing on.
 * \param frame
 *      The function call frame. Use `tiro_async_frame_arg` and `tiro_async_frame_arg_count` to access the function
 *      call arguments. Call `tiro_async_frame_return_value` to set the return value (it defaults to null if not set).
 *      The closure is also available by calling `tiro_async_frame_closure`.
 *
 *      The frame remains valid until it is freed by the caller by invoking `tiro_async_frame_free` (forgetting to
 *      free a frame results in a memory leak).
 */
typedef void (*tiro_async_function_t)(tiro_vm_t vm, tiro_async_frame_t frame);

/**
 * Frees an async frame. Does nothing if `frame` is NULL.
 *
 * Frames are allocated for the caller before invoking the native callback function. They must be freed by calling
 * this function after async operation has been completed (e.g. after calling `tiro_async_frame_return_value(...)`).
 *
 * \warning *All* async call frames must be freed before the vm itself is freed. If there are pending async operations
 * when the vm shall be destroyed, always free them first (they do not have to receive a result).
 */
TIRO_API void tiro_async_frame_free(tiro_async_frame_t frame);

/** Returns the vm instance that this frame belongs to. Returns NULL on error. */
TIRO_API tiro_vm_t tiro_async_frame_vm(tiro_async_frame_t frame);

/** Returns the number of function call arguments received by this frame. Returns 0 on error. */
TIRO_API size_t tiro_async_frame_arg_count(tiro_async_frame_t frame);

/**
 * Retrieves the function call argument at the specified index and stores it into `result`.
 * Returns `TIRO_ERROR_OUT_OF_BOUNDS` if the argument index is invalid.
 */
TIRO_API void tiro_async_frame_arg(
    tiro_async_frame_t frame, size_t index, tiro_handle_t result, tiro_error_t* err);

/** Returns the closure value which was specified when the function was created. */
TIRO_API void
tiro_async_frame_closure(tiro_async_frame_t frame, tiro_handle_t result, tiro_error_t* err);

/**
 * Sets the return value for the given function call frame to the given `value`.
 * Async function frames must be freed (by calling `tiro_async_frame_free`) after they have been completed.
 */
TIRO_API void
tiro_async_frame_return_value(tiro_async_frame_t frame, tiro_handle_t value, tiro_error_t* err);

/**
 * Signals a panic from the given function call frame.
 * TODO: Allow user defined exception objects instead of plain string?
 * Async function frames must be freed (by calling `tiro_async_frame_free`) after they have been completed.
 */
TIRO_API void
tiro_async_frame_panic_msg(tiro_async_frame_t frame, tiro_string message, tiro_error_t* err);

/**
 * Constructs a new function object with the given name that will invoke the native function `func` when called.
 * `argc` is the number of arguments required for calling `func`. `closure` may be an arbitrary value
 * that will be passed to the function on every invocation.
 *
 * On success, the new function will be stored in `result`.
 * Returns `TIRO_BAD_TYPE` if `name` is not a string. Returns `TIRO_BAD_ARG` if the the requested number of parameters
 * is too large. The current maximum is `1024`.
 */
TIRO_API void tiro_make_async_function(tiro_vm_t vm, tiro_handle_t name, tiro_async_function_t func,
    size_t argc, tiro_handle_t closure, tiro_handle_t result, tiro_error_t* err);

/**
 * The prototype of a native function that implements a resumable function.
 * Resumable functions are the most versatile and most complex kind of functions in the native API.
 * They may return or yield any number of times and may also call other tiro functions.
 * Because of the cooperative nature of coroutines in tiro, they must be implemented as state machines.
 *
 * When a resumable function is invoked by the vm, a new call frame is created
 * on the active coroutine's stack. This frame stores the function call's state (initially `TIRO_RESUMABLE_STATE_START`).
 * The vm will continue to call the native function until it reaches the `TIRO_RESUMABLE_STATE_END` state either by
 * performing a final return or by panicking.
 * Until then, the function may manipulate its own state or invoke other functions by calling the frame's associated functions.
 *
 * When a resumable function has either returned or panicked, the native function will be called
 * one last time with a special `TIRO_RESUMABLE_STATE_CLEANUP` state that allows it to release any acquired resources.
 *
 * \param vm
 *  The virtual machine the function is executing on.
 * \param frame
 *  The function call frame. Use associated functions to manipulate the frame's state, to access
 *  function call arguments, etc.
 *
 * TODO: yield?
 */
typedef void (*tiro_resumable_function_t)(tiro_vm_t vm, tiro_resumable_frame_t frame);

/**
 * Lists well known state values used by resumable functions.
 *
 * All positive integers can be used freely by the application.
 */
typedef enum tiro_resumable_state {
    /** The initial state value. */
    TIRO_RESUMABLE_STATE_START = 0,

    /** Signals that the function has finished executing. */
    TIRO_RESUMABLE_STATE_END = -1,
} tiro_resumable_state_t;

/**
 * Returns the number of function call arguments present in the given frame.
 * Returns 0 for invalid input arguments.
 */
TIRO_API size_t tiro_resumable_frame_arg_count(tiro_resumable_frame_t frame);

/**
 * Stores the function call argument with the given `index` into `result`.
 * Returns `TIRO_ERROR_OUT_OF_BOUNDS` if the argument index is invalid.
 *
 * NOTE: this could return a handle directly, provided that the handle
 * is only used while the frame pointer is valid.
 */
TIRO_API void tiro_resumable_frame_arg(
    tiro_resumable_frame_t frame, size_t index, tiro_handle_t result, tiro_error_t* err);

/**
 * Returns the number of local values available in the given frame.
 * Returns 0 for invalid input arguments.
 */
TIRO_API size_t tiro_resumable_frame_local_count(tiro_resumable_frame_t frame);

/**
 * Stores the frame's local value with the given `index` into `result`.
 * Returns `TIRO_ERROR_OUT_OF_BOUNDS` if the local index is invalid.
 *
 * Local values are private to a function frame (i.e. the current function execution)
 * and persist between calls to the implementing native function.
 * They can be used to transport values between yields or nested function calls.
 *
 * NOTE: this could return a mutable handle directly, provided that the handle
 * is only used while the frame pointer is valid.
 */
TIRO_API void tiro_resumable_frame_local(
    tiro_resumable_frame_t frame, size_t index, tiro_handle_t result, tiro_error_t* err);

/**
 * Stores the given value into the local value slot with the given `index`.
 * Returns `TIRO_ERROR_OUT_OF_BOUNDS` if the local index is invalid.
 *
 * Local values are private to a function frame (i.e. the current function execution)
 * and persist between calls to the implementing native function.
 * They can be used to transport values between yields or nested function calls.
 *
 * NOTE: this function would not be necessary if get_local returned a handle.
 */
TIRO_API void tiro_resumable_frame_set_local(
    tiro_resumable_frame_t frame, size_t index, tiro_handle_t value, tiro_error_t* err);

/** Returns the closure value which was specified when the function was created. */
TIRO_API void
tiro_resumable_frame_closure(tiro_resumable_frame_t frame, tiro_handle_t result, tiro_error_t* err);

/**
 * Returns the current state of the given frame.
 * Returns 0 for invalid input arguments.
 */
TIRO_API int tiro_resumable_frame_state(tiro_resumable_frame_t frame);

/**
 * Sets the current state of the given frame.
 * It is usually not necessary to invoke this function directly as changing the state
 * is also implied by other functions like `tiro_resumable_frame_invoke` and `tiro_resumable_frame_return_value`.
 *
 * The calling native function should return after altering the state.
 * The new state will be active when the native function is called for the next time.
 *
 * Note that a few states have special meaning (see `tiro_resumable_state_t`).
 *
 * \param frame The resumable call frame
 * \param next_state The new state value
 */
void TIRO_API tiro_resumable_frame_set_state(
    tiro_resumable_frame_t frame, int next_state, tiro_error_t* err);

/**
 * Signals the vm that the function `func` shall be invoked with the given arguments in `args`.
 * `func` will be invoked after the native function returned to the vm.
 * The current native function will be called again when `func` has itself returned, and its return value
 * will be accessible via `tiro_resumable_frame_invoke_return(...)`.
 *
 * Calling this function implies a state change to `next_state`, which will be the frame's state
 * when the native function is called again after `func`'s execution.
 *
 * \param frame The resumable call frame
 * \param next_state The new state value
 * \param func Must refer to a valid function
 * \param args
 *      Must be either `NULL` (no arguments), refer to a null value (same) or
 *      a valid tuple (the function call arguments).
 */
TIRO_API void tiro_resumable_frame_invoke(tiro_resumable_frame_t frame, int next_state,
    tiro_handle_t func, tiro_handle_t args, tiro_error_t* err);

/**
 * Returns the result of the last function call made via `tiro_resumable_frame_invoke`.
 * Only returns a useful value when the native function is called again for the first time
 * after calling `tiro_resumable_frame_invoke` and returning to the vm.
 *
 * \param frame The resumable call frame
 * \param result Will be set to the function's return value.
 */
TIRO_API void tiro_resumable_frame_invoke_return(
    tiro_resumable_frame_t frame, tiro_handle_t result, tiro_error_t* err);

/**
 * Sets the return value for the given function call frame to the given `value`.
 * The call frame's state is also set to `END` as a result of this call.
 */
TIRO_API void tiro_resumable_frame_return_value(
    tiro_resumable_frame_t frame, tiro_handle_t value, tiro_error_t* err);

/**
 * Signals a panic from the given function call frame.
 * The call frame's state is also set to `END` as a result of this call.
 *
 * TODO: Allow user defined exception objects instead of plain string?
 */
TIRO_API void tiro_resumable_frame_panic_msg(
    tiro_resumable_frame_t frame, tiro_string message, tiro_error_t* err);

/** Represents construction parameters for a resumable function. */
typedef struct tiro_resumable_frame_desc {
    /**
     * The function name. Must be set to a valid string.
     */
    tiro_handle_t name;

    /**
     * The number of function parameters (0 by default).
     * The maximum number of function arguments is limited to a reasonable amount (currently `1024`).
     */
    size_t arg_count;

    /**
     * The number of locals usable by a function frame.
     * The maximum number of function locals is limited to a reasonable amount (currently `2 ** 14`).
     */
    size_t local_count;

    /**
     * An arbitrary value (may be NULL) that will be passed to the function (via the frame) on each invocation.
     * The value is shared by all call frames created for this function.
     */
    tiro_handle_t closure;

    /**
     * The native function pointer that implements the resumable function.
     */
    tiro_resumable_function_t func;
} tiro_resumable_frame_desc_t;

/**
 * Constructs a new function object with the given description.
 *
 * On success, the new function will be stored in `result`.
 */
TIRO_API void tiro_make_resumable_function(
    tiro_vm_t vm, const tiro_resumable_frame_desc_t* desc, tiro_handle_t result, tiro_error_t* err);

#ifdef __cplusplus
}
#endif

#endif // TIRO_FUNCTIONS_H_INCLUDED
