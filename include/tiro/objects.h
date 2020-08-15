#ifndef TIRO_OBJECTS_H_INCLUDED
#define TIRO_OBJECTS_H_INCLUDED

#include "tiro/def.h"
#include "tiro/error.h"

#ifdef __cplusplus
extern "C" {
#endif

/** 
 * Represents the kind of a tiro value.
 */
typedef enum tiro_kind {
    TIRO_KIND_NULL = 0,        /* Value is null */
    TIRO_KIND_BOOLEAN,         /* Value is true or false */
    TIRO_KIND_INTEGER,         /* Value is an integer */
    TIRO_KIND_FLOAT,           /* Value is a floating point number */
    TIRO_KIND_STRING,          /* Value is a string */
    TIRO_KIND_FUNCTION,        /* Value is a function */
    TIRO_KIND_TUPLE,           /* Value is a tuple */
    TIRO_KIND_ARRAY,           /* Value is an array */
    TIRO_KIND_COROUTINE,       /* Value is a coroutine */
    TIRO_KIND_TYPE,            /* Value is a type */
    TIRO_KIND_INTERNAL = 1000, /* Value is some other, internal type */
    TIRO_KIND_INVALID,         /* Invalid value (e.g. null handle) */
} tiro_kind;

/**
 * Returns the name of the kind, formatted as a string.
 * The string points into static storage and must not be freed.
 */
TIRO_API const char* tiro_kind_str(tiro_kind kind);

/**
 * Represents a value in the tiro language.
 * 
 * Values cannot be used directly through the API. Instead, all operations on values must
 * be done through a `tiro_handle`. Handles are a wrapper type around a value which ensures that
 * their inner value always remains valid, even if garbage collection is triggered.
 * 
 * \warning 
 *      Handles may only be used when obtained from a function of this API (such as `tiro_frame_slot()`).
 *      They must never be initialized manually!
 * 
 * A value stored in a valid handle is always considered *live*, which means that the garbage collector
 * will not destroy it. If the garbage collector decides to move a value (which would change its address), 
 * the handles refering to that address will be updated automatically in a process that is completely transparent to the user.
 */
struct tiro_value;

/** Returns the kind of the handle's current value. */
TIRO_API tiro_kind tiro_value_kind(tiro_vm* vm, tiro_handle value);

/** Outputs a string representing the given value. The string is assigned to `result`. */
TIRO_API tiro_errc tiro_value_to_string(
    tiro_vm* vm, tiro_handle value, tiro_handle result, tiro_error** err);

/**
 * Returns the type of the given `value` by assigning it to `result`.
 * This function will fail with an error when attempting to access an internal type.
 */
TIRO_API tiro_errc tiro_value_type(
    tiro_vm* vm, tiro_handle value, tiro_handle result, tiro_error** err);

/** 
 * Retrieves the type instance that corresponds to the given `kind` and assigns it to `result`.
 * `kind` must represent a valid, exported value kind, otherwise an error is returned instead.
 */
TIRO_API tiro_errc tiro_kind_type(
    tiro_vm* vm, tiro_kind kind, tiro_handle result, tiro_error** err);

/** Sets the given `result` handle to null. */
TIRO_API void tiro_make_null(tiro_vm* vm, tiro_handle result);

/** Returns the specified boolean value via the output argument `result`. */
TIRO_API tiro_errc tiro_make_boolean(tiro_vm* vm, bool value, tiro_handle result, tiro_error** err);

/** 
 * Returns `value` converted to a boolean value. `false` and `null` are considered false, all other values will return `true`.
 */
TIRO_API bool tiro_boolean_value(tiro_vm* vm, tiro_handle value);

/** Constructs an integer with the given value. Returns `TIRO_ERROR_ALLOC` on allocation failure. */
TIRO_API tiro_errc tiro_make_integer(
    tiro_vm* vm, int64_t value, tiro_handle result, tiro_error** err);

/** 
 * Returns `value` converted to an integer. This function supports conversion for floating point values 
 * (they are truncated to an integer). All other values return 0 (use `tiro_value_kind` to disambiguate between types).
 */
TIRO_API int64_t tiro_integer_value(tiro_vm* vm, tiro_handle value);

/** Constructs a float with the given value. Returns `TIRO_ERROR_ALLOC` on allocation failure. */
TIRO_API tiro_errc tiro_make_float(tiro_vm* vm, double value, tiro_handle result, tiro_error** err);

/**
 * Returns the floating point of `value`. This function supports conversion for integer values, all
 * other values will return 0 (use `tiro_value_kind` to disambiguate between types).
 */
TIRO_API double tiro_float_value(tiro_vm* vm, tiro_handle value);

/** 
 * Constructs a new string with the given content. `value` must be zero terminated or NULL. Passing NULL 
 * for `value` creates an empty string. Returns `TIRO_ERROR_ALLOC` on allocation failure.
 */
TIRO_API tiro_errc tiro_make_string(
    tiro_vm* vm, const char* value, tiro_handle result, tiro_error** err);

/**
 * Constructs a new string with the given content. `data` must consist of `length` readable bytes.
 * Returns `TIRO_ERROR_ALLOC` on allocation failure.
 */
TIRO_API tiro_errc tiro_make_string_from_data(
    tiro_vm* vm, const char* data, size_t length, tiro_handle result, tiro_error** err);

/** 
 * Retrieves the string's content as a (data, length)-pair without copying the data. The pointer to the string's
 * storage will be placed in `*data`, and the length (in bytes) will be assigned to `*length`.
 * Returns `TIRO_ERROR_BAD_TYPE` if the value is not actually a string.
 * 
 * \warning 
 *  The string content returned through `data` and `length` is a view into the string's current storage. Because objects
 *  may move on the heap (e.g. because of garbage collection), this data may be invalidated. The data may only be used
 *  immediately after calling this function, and must not be used after another possibly allocating tiro_* function has been called.
 * 
 * \warning 
 *  The string returned by this function is not zero terminated.
 */
TIRO_API tiro_errc tiro_string_value(
    tiro_vm* vm, tiro_handle string, const char** data, size_t* length, tiro_error** err);

/**
 * Retrieves the string's content and creates a new zero terminated c string, which is assigned to `*result`.
 * `*result` is only assigned to if the construction of the string succeeded (in which case `TIRO_OK` will be returned).
 * Returns `TIRO_ERROR_BAD_TYPE` if the value is not actually a string.
 * 
 * If `TIRO_OK` was returned, then `*result` must be passed to `free` for cleanup.
 */
TIRO_API tiro_errc tiro_string_cstr(
    tiro_vm* vm, tiro_handle string, char** result, tiro_error** err);

/** Constructs a new tuple with `size` entries. All entries are initially null. Returns `TIRO_ERROR_ALLOC` on allocation failure. */
TIRO_API tiro_errc tiro_make_tuple(tiro_vm* vm, size_t size, tiro_handle result, tiro_error** err);

/** Returns the tuple's size, or 0 if the given value is not a tuple (use `tiro_value_kind` to disambiguate between types). */
TIRO_API size_t tiro_tuple_size(tiro_vm* vm, tiro_handle tuple);

/** 
 * Retrieves the tuple element at the given `index` from `tuple` and assigns it to `result`, unless an error occcurs.
 * Returns `TIRO_ERROR_BAD_TYPE` if the instance is not a tuple, or `TIRO_ERROR_OUT_OF_BOUNDS` if the index is out of bounds.
 */
TIRO_API tiro_errc tiro_tuple_get(
    tiro_vm* vm, tiro_handle tuple, size_t index, tiro_handle result, tiro_error** err);

/**
 * Sets the tuple's element at position `index` to `value`. 
 * Returns `TIRO_ERROR_BAD_TYPE` if the instance is not a tuple, or `TIRO_ERROR_OUT_OF_BOUNDS` if the index is out of bounds.
 */
TIRO_API tiro_errc tiro_tuple_set(
    tiro_vm* vm, tiro_handle tuple, size_t index, tiro_handle value, tiro_error** err);

/** Constructs a new, empty array with the given initial capacity. Returns `TIRO_ERROR_ALLOC` on allocation failure. */
TIRO_API tiro_errc tiro_make_array(
    tiro_vm* vm, size_t initial_capacity, tiro_handle result, tiro_error** err);

/** Returns the array's size, or 0 if the given value is not an array (use `tiro_value_kind` to disambiguate between types). */
TIRO_API size_t tiro_array_size(tiro_vm* vm, tiro_handle array);

/** 
 * Retrieves the array element at the given `index` from `array` and assigns it to `result`, unless an error occcurs.
 * Returns `TIRO_ERROR_BAD_TYPE` if the instance is not an array, or `TIRO_ERROR_OUT_OF_BOUNDS` if the index is out of bounds.
 */
TIRO_API tiro_errc tiro_array_get(
    tiro_vm* vm, tiro_handle array, size_t index, tiro_handle result, tiro_error** err);

/**
 * Sets the array's element at position `index` to `value`. 
 * Returns `TIRO_ERROR_BAD_TYPE` if the instance is not an array, or `TIRO_ERROR_OUT_OF_BOUNDS` if the index is out of bounds.
 */
TIRO_API tiro_errc tiro_array_set(
    tiro_vm* vm, tiro_handle array, size_t index, tiro_handle value, tiro_error** err);

/** Appends `value` to the given `array`. */
TIRO_API tiro_errc tiro_array_push(
    tiro_vm* vm, tiro_handle array, tiro_handle value, tiro_error** err);

/**
 * Removes the last element from the given `array`. Does nothing (successfully) if the array is already empty.
 * Returns `TIRO_ERROR_OUT_OF_BOUNDS` if the array is already empty.
 */
TIRO_API tiro_errc tiro_array_pop(tiro_vm* vm, tiro_handle array, tiro_error** err);

/** Removes all elements from the given `array`. Returns `TIRO_ERROR_BAD_TYPE` if the value is not an array. */
TIRO_API tiro_errc tiro_array_clear(tiro_vm* vm, tiro_handle array, tiro_error** err);

/**
 * Constructs a new coroutine that will execute the given function. 
 * Note that the coroutine will not be started before passing it to `tiro_coroutine_start`.
 * 
 * `func` must be a value of kind `TIRO_KIND_FUNCTION`, otherwise `TIRO_ERROR_BAD_TYPE` is returned.
 * 
 * `arguments` may be a NULL handle, a handle referencing a null value (to pass no arguments) or a tuple of values
 * that will be passed to the function as arguments.
 * 
 * Returns `TIRO_ERROR_ALLOC` on allocation failure.
 */
TIRO_API tiro_errc tiro_make_coroutine(
    tiro_vm* vm, tiro_handle func, tiro_handle arguments, tiro_handle result, tiro_error** err);

/** Returns true if the coroutine has been started, false otherwise. */
TIRO_API bool tiro_coroutine_started(tiro_vm* vm, tiro_handle coroutine);

/** Returns true if the coroutine has finished its execution, false otherwise. */
TIRO_API bool tiro_coroutine_completed(tiro_vm* vm, tiro_handle coroutine);

/**
 * Returns the coroutine's result by assigning it to `result`. The coroutine must have completed execution, i.e. 
 * `tiro_coroutine_completed()` must return true (for example, when invoked from a coroutine's completion callback).
 */
TIRO_API tiro_errc tiro_coroutine_result(
    tiro_vm* vm, tiro_handle coroutine, tiro_handle result, tiro_error** err);

/**
 * Represents a coroutine completion callback. These are invoked when a coroutine finishes execution,
 * either successfully or with an error.
 * 
 * \param vm
 *      The virtual machine the coroutine was running on.
 * \param coro
 *      The completed coroutine.
 * \param userdata
 *      The original userdata pointer that was provided when the callback was registered.
 */
typedef void (*tiro_coroutine_callback)(tiro_vm* vm, tiro_handle coro, void* userdata);

/**
 * Represents a cleanup function associated with a coroutine callback. Cleanup functions are always executed immediately
 * after the callback was invoked, or when the virtual machine is shutting down prior to the coroutine's completion.
 * The cleanup function is always executed exactly once.
 * 
 * \param vm 
 *      The virtual machine the coroutine was running on.
 * \param userdata 
 *      The original userdata pointer that was provided when the callback was registered.
 */
typedef void (*tiro_coroutine_cleanup)(tiro_vm* vm, void* userdata);

/**
 * Schedules the given callback to be invoked once the coroutine completes.
 * 
 * `coroutine` must be a handle to a coroutine, otherwise `TIRO_ERROR_BAD_TYPE` is returned.
 * 
 * `callback` will be invoked when the coroutine completes its execution. A coroutine completes when the outermost function
 * returns normally or if an uncaught exception is thrown from that function. The callback receives a handle to the completed coroutine,
 * which can be inspected in order to retrieve the coroutine's result, and the original `userdata` argument.
 * It will *not* be invoked if the virtual machine shuts down before the coroutine has completed.
 * The callback must not be NULL, otherwise `TIRO_ERROR_BAD_ARG` will be returned.
 * 
 * Note: all callback invocations happen from within one of the `tiro_vm_run*` functions.
 * 
 * `cleanup` will be invoked to release state that may be owned by the callback. It will be called directly after the callback
 * has been invoked, or as part of the virtual machine's shutdown procedure. The cleanup function receives the original `userdata` argument.
 * When present, `cleanup` will be called exactly once. The `cleanup` function is optional.
 * 
 * `userdata` will be passed to `callback` and `cleanup` when appropriate, and it will not be used in any other way.
 */
TIRO_API tiro_errc tiro_coroutine_set_callback(tiro_vm* vm, tiro_handle coroutine,
    tiro_coroutine_callback callback, tiro_coroutine_cleanup cleanup, void* userdata,
    tiro_error** err);

/**
 * Starts the given coroutine by scheduling it for execution. The coroutine must not have been started before.
 * Coroutines are not invoked from this function. They will be executed from within one of the `tiro_vm_run*` functions.
 * Returns `TIRO_ERROR_BAD_TYPE` if the argument is not a coroutine, or `TIRO_ERROR_BAD_STATE` if the coroutine cannot be started.
 */
TIRO_API tiro_errc tiro_coroutine_start(tiro_vm* vm, tiro_handle coroutine, tiro_error** err);

/** Retrieves the name of this `type` and assigns it to `result`. Returns `TIRO_ERROR_BAD_TYPE` if the value is not a type. */
TIRO_API tiro_errc tiro_type_name(
    tiro_vm* vm, tiro_handle type, tiro_handle result, tiro_error** err);

/**
 * Represents an array of rooted values with dynamic lifetime. A frame is composed of slots, each of which
 * can hold an arbitrary value. Frames are registered with the garbage collector, so
 * their slots will be visited during collection (with their values being marked as live).
 */
struct tiro_frame;

/**
 * Constructs a new frame with the given number of slots. 
 * Frame must be destroyed by calling `tiro_frame_free`.
 * A frame belongs to its `vm` instance and must be destroyed before the vm is destroyed.
 * 
 * Returns NULL on allocation failure, or if `vm` is NULL.
 */
TIRO_API TIRO_WARN_UNUSED tiro_frame* tiro_frame_new(tiro_vm* vm, size_t slots);

/**
 * Frees a frame. Must be called exactly once for every frame created with `tiro_frame_new`.
 * 
 * Does nothing if `frame` is NULL.
 */
TIRO_API void tiro_frame_free(tiro_frame* frame);

/**
 * Returns the number of slots in this frame.
 */
TIRO_API size_t tiro_frame_size(tiro_frame* frame);

/** 
 * Returns the frame's slot at the given `slot_index` as a handle.
 * Returns NULL if the slot index is invalid.
 * 
 * All slot indices between 0 (inclusive) and `tiro_frame_size(frame)` (exclusive) can be used.
 */
TIRO_API tiro_handle tiro_frame_slot(tiro_frame* frame, size_t slot_index);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif // TIRO_OBJECTS_H_INCLUDED
