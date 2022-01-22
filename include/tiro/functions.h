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
 *      The function call frame. Use `tiro_sync_frame_arg` and `tiro_sync_frame_argc` to access the function
 *      call arguments. Call `tiro_sync_frame_result` to set the return value (it defaults to null if not set).
 *      The closure is also available by calling `tiro_sync_frame_closure`.
 *      The frame value is only valid for the duration of the function call.
 */
typedef void (*tiro_sync_function_t)(tiro_vm_t vm, tiro_sync_frame_t frame);

/**
 * Returns the number of function call arguments present in the given frame.
 * Returns 0 for invalid input arguments.
 */
TIRO_API size_t tiro_sync_frame_argc(tiro_sync_frame_t frame);

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
 *      The function call frame. Use `tiro_async_frame_arg` and `tiro_async_frame_argc` to access the function
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
TIRO_API size_t tiro_async_frame_argc(tiro_async_frame_t frame);

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

#ifdef __cplusplus
}
#endif

#endif // TIRO_FUNCTIONS_H_INCLUDED
