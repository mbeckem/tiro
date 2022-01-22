#ifndef TIRO_OBJECTS_H_INCLUDED
#define TIRO_OBJECTS_H_INCLUDED

/**
 * \file
 * \brief Functions and type definitions for working with objects of the tiro virtual machine.
 *
 */

#include "tiro/def.h"
#include "tiro/error.h"

#ifdef __cplusplus
extern "C" {
#endif

/** Represents the kind of a tiro value. */
typedef enum tiro_kind {
    TIRO_KIND_NULL = 0,        ///< Value is null
    TIRO_KIND_BOOLEAN,         ///< Value is true or false
    TIRO_KIND_INTEGER,         ///< Value is an integer
    TIRO_KIND_FLOAT,           ///< Value is a floating point number
    TIRO_KIND_STRING,          ///< Value is a string
    TIRO_KIND_FUNCTION,        ///< Value is a function
    TIRO_KIND_TUPLE,           ///< Value is a tuple
    TIRO_KIND_RECORD,          ///< Value is a record
    TIRO_KIND_ARRAY,           ///< Value is an array
    TIRO_KIND_RESULT,          ///< Value is a result
    TIRO_KIND_EXCEPTION,       ///< Value is an exception
    TIRO_KIND_COROUTINE,       ///< Value is a coroutine
    TIRO_KIND_MODULE,          ///< Value is a module
    TIRO_KIND_TYPE,            ///< Value is a type
    TIRO_KIND_NATIVE,          ///< Value is a native object
    TIRO_KIND_INTERNAL = 1000, ///< Value is some other, internal type
    TIRO_KIND_INVALID,         ///< Invalid value (e.g. null handle)
} tiro_kind_t;

/**
 * Returns the name of the kind, formatted as a string.
 * The string points into static storage and must not be freed.
 */
TIRO_API const char* tiro_kind_str(tiro_kind_t kind);

/**
 * Represents a value in the tiro language.
 *
 * Values cannot be used directly through the API. Instead, all operations on values must
 * be done through a `tiro_handle_t`. Handles are a wrapper type around a value which ensures that
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
TIRO_API tiro_kind_t tiro_value_kind(tiro_vm_t vm, tiro_handle_t value);

/** Returns true if and only if `a` and `b` refer to exactly the same value. */
TIRO_API bool tiro_value_same(tiro_vm_t vm, tiro_handle_t a, tiro_handle_t b);

/** Outputs a string representing the given value. The string is assigned to `result`. */
TIRO_API void
tiro_value_to_string(tiro_vm_t vm, tiro_handle_t value, tiro_handle_t result, tiro_error_t* err);

/** Copies the current `value` to `result`. */
TIRO_API void
tiro_value_copy(tiro_vm_t vm, tiro_handle_t value, tiro_handle_t result, tiro_error_t* err);

/**
 * Returns the type of the given `value` by assigning it to `result`.
 * This function will fail with an error when attempting to access an internal type.
 */
TIRO_API void
tiro_value_type(tiro_vm_t vm, tiro_handle_t value, tiro_handle_t result, tiro_error_t* err);

/**
 * Retrieves the type instance that corresponds to the given `kind` and assigns it to `result`.
 * `kind` must represent a valid, exported value kind, otherwise an error is returned instead.
 */
TIRO_API void
tiro_kind_type(tiro_vm_t vm, tiro_kind_t kind, tiro_handle_t result, tiro_error_t* err);

/** Sets the given `result` handle to null. */
TIRO_API void tiro_make_null(tiro_vm_t vm, tiro_handle_t result);

/** Returns the specified boolean value via the output argument `result`. */
TIRO_API void tiro_make_boolean(tiro_vm_t vm, bool value, tiro_handle_t result, tiro_error_t* err);

/**
 * Returns `value` converted to a boolean value. `false` and `null` are considered false, all other values will return `true`.
 */
TIRO_API bool tiro_boolean_value(tiro_vm_t vm, tiro_handle_t value);

/** Constructs an integer with the given value. Returns `TIRO_ERROR_ALLOC` on allocation failure. */
TIRO_API void
tiro_make_integer(tiro_vm_t vm, int64_t value, tiro_handle_t result, tiro_error_t* err);

/**
 * Returns `value` converted to an integer. This function supports conversion for floating point values
 * (they are truncated to an integer). All other values return 0 (use `tiro_value_kind` to disambiguate between types).
 */
TIRO_API int64_t tiro_integer_value(tiro_vm_t vm, tiro_handle_t value);

/** Constructs a float with the given value. Returns `TIRO_ERROR_ALLOC` on allocation failure. */
TIRO_API void tiro_make_float(tiro_vm_t vm, double value, tiro_handle_t result, tiro_error_t* err);

/**
 * Returns the floating point of `value`. This function supports conversion for integer values, all
 * other values will return 0 (use `tiro_value_kind` to disambiguate between types).
 */
TIRO_API double tiro_float_value(tiro_vm_t vm, tiro_handle_t value);

/**
 * Constructs a new string with the given content.
 * Returns `TIRO_ERROR_ALLOC` on allocation failure.
 */
TIRO_API void
tiro_make_string(tiro_vm_t vm, tiro_string_t value, tiro_handle_t result, tiro_error_t* err);

/**
 * Retrieves the string's content as a (data, length)-pair without copying the data.
 * The pointer to the string's storage will be placed in `*value`.
 * Returns `TIRO_ERROR_BAD_TYPE` if the value is not actually a string.
 *
 * \warning
 *  The string content returned through `value` is a view into the string's current storage.
 *  Because objects may move on the heap (e.g. because of garbage collection), this data may be invalidated.
 *  The data may only be used immediately after calling this function in native code that is guaranteed to NOT allocate on the tiro heap.
 *  It MUST NOT be used as input tiro an allocating function (which includes most functions of this API), or after such a function has been called.
 *
 * \warning
 *  The string returned by this function is not zero terminated.
 */
TIRO_API void
tiro_string_value(tiro_vm_t vm, tiro_handle_t string, tiro_string_t* value, tiro_error_t* err);

/**
 * Retrieves the string's content and creates a new zero terminated c string, which is assigned to `*result`.
 * `*result` is only assigned to if the construction of the string succeeded (in which case `TIRO_OK` will be returned).
 * Returns `TIRO_ERROR_BAD_TYPE` if the value is not actually a string.
 *
 * If `TIRO_OK` was returned, then `*result` must be passed to `free` for cleanup.
 */
TIRO_API void
tiro_string_cstr(tiro_vm_t vm, tiro_handle_t string, char** result, tiro_error_t* err);

/** Constructs a new tuple with `size` entries. All entries are initially null. Returns `TIRO_ERROR_ALLOC` on allocation failure. */
TIRO_API void tiro_make_tuple(tiro_vm_t vm, size_t size, tiro_handle_t result, tiro_error_t* err);

/** Returns the tuple's size, or 0 if the given value is not a tuple (use `tiro_value_kind` to disambiguate between types). */
TIRO_API size_t tiro_tuple_size(tiro_vm_t vm, tiro_handle_t tuple);

/**
 * Retrieves the tuple element at the given `index` from `tuple` and assigns it to `result`, unless an error occurs.
 * Returns `TIRO_ERROR_BAD_TYPE` if the instance is not a tuple, or `TIRO_ERROR_OUT_OF_BOUNDS` if the index is out of bounds.
 */
TIRO_API void tiro_tuple_get(
    tiro_vm_t vm, tiro_handle_t tuple, size_t index, tiro_handle_t result, tiro_error_t* err);

/**
 * Sets the tuple's element at position `index` to `value`.
 * Returns `TIRO_ERROR_BAD_TYPE` if the instance is not a tuple, or `TIRO_ERROR_OUT_OF_BOUNDS` if the index is out of bounds.
 */
TIRO_API void tiro_tuple_set(
    tiro_vm_t vm, tiro_handle_t tuple, size_t index, tiro_handle_t value, tiro_error_t* err);

/**
 * Constructs a new record with the given key names. `keys` must be an array consisting of strings (which mus be unique).
 * The specified keys will be valid property names on the new record. The value associated with each key will be initialized to null.
 *
 * Returns `TIRO_ERROR_BAD_TYPE` if `keys` is not an array, or if its contents are not all strings.
 * On success, the constructed record will be assigned to `result`.
 */
TIRO_API void
tiro_make_record(tiro_vm_t vm, tiro_handle_t keys, tiro_handle_t result, tiro_error_t* err);

/** Retrieves an array of valid keys for the given record. Returns `TIRO_ERROR_BAD_TYPE` if the instance is not a record. */
TIRO_API void
tiro_record_keys(tiro_vm_t vm, tiro_handle_t record, tiro_handle_t result, tiro_error_t* err);

/**
 * Retrieves the value associated with the given key on this record. On success, the value will be assigned to `result`.
 * Returns `TIRO_ERROR_BAD_TYPE` if the instance is not a record, or if `key` is not a string.
 * Returns `TIRO_ERROR_BAD_KEY` if the key is invalid for this record.
 */
TIRO_API void tiro_record_get(
    tiro_vm_t vm, tiro_handle_t record, tiro_handle_t key, tiro_handle_t result, tiro_error_t* err);

/**
 * Sets the record's value associated with the given `key` to `value`.
 * Returns `TIRO_ERROR_BAD_TYPE` if the instance is not a record, or if `key` is not a string.
 * Returns `TIRO_ERROR_BAD_KEY` if the key is invalid for this record.
 */
TIRO_API void tiro_record_set(
    tiro_vm_t vm, tiro_handle_t record, tiro_handle_t key, tiro_handle_t value, tiro_error_t* err);

/** Constructs a new, empty array with the given initial capacity. Returns `TIRO_ERROR_ALLOC` on allocation failure. */
TIRO_API void
tiro_make_array(tiro_vm_t vm, size_t initial_capacity, tiro_handle_t result, tiro_error_t* err);

/** Returns the array's size, or 0 if the given value is not an array (use `tiro_value_kind` to disambiguate between types). */
TIRO_API size_t tiro_array_size(tiro_vm_t vm, tiro_handle_t array);

/**
 * Retrieves the array element at the given `index` from `array` and assigns it to `result`, unless an error occurs.
 * Returns `TIRO_ERROR_BAD_TYPE` if the instance is not an array, or `TIRO_ERROR_OUT_OF_BOUNDS` if the index is out of bounds.
 */
TIRO_API void tiro_array_get(
    tiro_vm_t vm, tiro_handle_t array, size_t index, tiro_handle_t result, tiro_error_t* err);

/**
 * Sets the array's element at position `index` to `value`.
 * Returns `TIRO_ERROR_BAD_TYPE` if the instance is not an array, or `TIRO_ERROR_OUT_OF_BOUNDS` if the index is out of bounds.
 */
TIRO_API void tiro_array_set(
    tiro_vm_t vm, tiro_handle_t array, size_t index, tiro_handle_t value, tiro_error_t* err);

/** Appends `value` to the given `array`. */
TIRO_API void
tiro_array_push(tiro_vm_t vm, tiro_handle_t array, tiro_handle_t value, tiro_error_t* err);

/**
 * Removes the last element from the given `array`. Does nothing (successfully) if the array is already empty.
 * Returns `TIRO_ERROR_OUT_OF_BOUNDS` if the array is already empty.
 */
TIRO_API void tiro_array_pop(tiro_vm_t vm, tiro_handle_t array, tiro_error_t* err);

/** Removes all elements from the given `array`. Returns `TIRO_ERROR_BAD_TYPE` if the value is not an array. */
TIRO_API void tiro_array_clear(tiro_vm_t vm, tiro_handle_t array, tiro_error_t* err);

/**
 * Constructs a new successful result with the given value. The new object will be placed into `result`.
 */
TIRO_API void
tiro_make_success(tiro_vm_t vm, tiro_handle_t value, tiro_handle_t result, tiro_error_t* err);

/**
 * Constructs a new error result with the given error. The new object will be placed into `result`.
 */
TIRO_API void
tiro_make_error(tiro_vm_t vm, tiro_handle_t error, tiro_handle_t result, tiro_error_t* err);

/** Returns true if the result in `instance` represents success. */
TIRO_API bool tiro_result_is_success(tiro_vm_t vm, tiro_handle_t instance);

/** Returns true if the result in `instance` represents an error. */
TIRO_API bool tiro_result_is_error(tiro_vm_t vm, tiro_handle_t instance);

/**
 * Retrieves the value from the result in `instance` and writes it into `out`.
 * Returns `TIRO_ERROR_BAD_STATE` if the result does not represent success, or `TIRO_ERROR_BAD_TYPE`
 * if the instance is no result at all.
 */
TIRO_API void
tiro_result_value(tiro_vm_t vm, tiro_handle_t instance, tiro_handle_t out, tiro_error_t* err);

/**
 * Retrieves the error from the result in `instance` and writes it into `out`.
 * Returns `TIRO_ERROR_BAD_STATE` if the result does not represent an error, or `TIRO_ERROR_BAD_TYPE`
 * if the instance is no result at all.
 */
TIRO_API void
tiro_result_error(tiro_vm_t vm, tiro_handle_t instance, tiro_handle_t out, tiro_error_t* err);

/**
 * Retrieves the message from the exception in `instance` and writes it into `result`.
 * If this call is successful, `result` will reference a string.
 * Returns `TIRO_ERROR_BAD_TYPE` if the instance is no exception.
 */
TIRO_API void tiro_exception_message(
    tiro_vm_t vm, tiro_handle_t instance, tiro_handle_t result, tiro_error_t* err);

/**
 * Retrieves the exception's call stack trace and writes it into `result`.
 * If this call is successful, `result` will reference a string (if stack traces are enabled
 * and one could be retrieved) or null otherwise.
 * Returns `TIRO_ERROR_BAD_TYPE` if the instance is no exception.
 */
TIRO_API void
tiro_exception_trace(tiro_vm_t vm, tiro_handle_t instance, tiro_handle_t result, tiro_error_t* err);

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
TIRO_API void tiro_make_coroutine(tiro_vm_t vm, tiro_handle_t func, tiro_handle_t arguments,
    tiro_handle_t result, tiro_error_t* err);

/** Returns true if the coroutine has been started, false otherwise. */
TIRO_API bool tiro_coroutine_started(tiro_vm_t vm, tiro_handle_t coroutine);

/** Returns true if the coroutine has finished its execution, false otherwise. */
TIRO_API bool tiro_coroutine_completed(tiro_vm_t vm, tiro_handle_t coroutine);

/**
 * Returns the coroutine's result by assigning it to `result`.
 * The coroutine must have completed execution, i.e. `tiro_coroutine_completed()` must return true (for example, when invoked from a coroutine's completion callback).
 * If the coroutine terminated with an uncaught panic, the result will hold an error.
 */
TIRO_API void tiro_coroutine_result(
    tiro_vm_t vm, tiro_handle_t coroutine, tiro_handle_t result, tiro_error_t* err);

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
typedef void (*tiro_coroutine_callback)(tiro_vm_t vm, tiro_handle_t coro, void* userdata);

/**
 * Represents a cleanup function associated with a coroutine callback. Cleanup functions are always executed immediately
 * after the callback was invoked, or when the virtual machine is shutting down prior to the coroutine's completion.
 * The cleanup function is always executed exactly once.
 *
 * \param userdata
 *      The original userdata pointer that was provided when the callback was registered.
 *
 * TODO: Does the cleanup function need access to the vm instance?
 */
typedef void (*tiro_coroutine_cleanup)(void* userdata);

/**
 * Schedules the given callback to be invoked once the coroutine completes.
 *
 * `coroutine` must be a handle to a coroutine, otherwise `TIRO_ERROR_BAD_TYPE` is returned.
 *
 * `callback` will be invoked when the coroutine completes its execution.
 * A coroutine completes when the outermost function returns normally or if an uncaught panic is thrown from that function.
 * The callback receives a handle to the completed coroutine, which can be inspected in order to retrieve the coroutine's result,
 * and the original `userdata` argument.
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
TIRO_API void
tiro_coroutine_set_callback(tiro_vm_t vm, tiro_handle_t coroutine, tiro_coroutine_callback callback,
    tiro_coroutine_cleanup cleanup, void* userdata, tiro_error_t* err);

/**
 * Starts the given coroutine by scheduling it for execution. The coroutine must not have been started before.
 * Coroutines are not invoked from this function. They will be executed from within one of the `tiro_vm_run*` functions.
 * Returns `TIRO_ERROR_BAD_TYPE` if the argument is not a coroutine, or `TIRO_ERROR_BAD_STATE` if the coroutine cannot be started.
 */
TIRO_API void tiro_coroutine_start(tiro_vm_t vm, tiro_handle_t coroutine, tiro_error_t* err);

typedef struct tiro_module_member_t {
    tiro_string_t name;
    tiro_handle_t value;
} tiro_module_member_t;

/**
 * Creates a new module with the given `name` from the given `members` list.
 *
 * `name` must be a non-empty string.
 * `members` must be a valid pointer that points to `members_length` entries.
 *  When the module has been created successfully, it will be written to `result`.
 *
 * \note All members listed in this function call will be exported by the module.
 *
 * TODO: Do we need an API for non-exported members?
 */
TIRO_API void tiro_make_module(tiro_vm_t vm, tiro_string_t name, tiro_module_member_t* members,
    size_t members_length, tiro_handle_t result, tiro_error_t* err);

/**
 * Attempts to retrieve the exported module member called `export_name` from the given module.
 * On success, the member will be written to `result`.
 *
 * Returns `TIRO_ERROR_BAD_TYPE` if the module is not actually of kind `TIRO_KIND_MODULE`.
 * Returns `TIRO_ERROR_EXPORT_NOT_FOUND` if no exported member with that name exists in this module.
 */
TIRO_API void tiro_module_get_export(tiro_vm_t vm, tiro_handle_t module, tiro_string_t export_name,
    tiro_handle_t result, tiro_error_t* err);

/** Retrieves the name of this `type` and assigns it to `result`. Returns `TIRO_ERROR_BAD_TYPE` if the value is not a type. */
TIRO_API void
tiro_type_name(tiro_vm_t vm, tiro_handle_t type, tiro_handle_t result, tiro_error_t* err);

/**
 * Describes a native object type to the tiro runtime. Instances of this type must be provided
 * to the API when constructing a new native object.
 *
 * Native objects that are created with a certain type will continue refencing that type instance by its address.
 * The lifetime of `tiro_native_type_t` instances is not managed by the runtime, they must remain valid for
 * as long as there are native objects referring to them.
 *
 * \todo DRAFT API. Will probably be replaced with native user defined types.
 *
 * \warning The native type instance must not be changed while it is being referenced by native objects!
 */
typedef struct tiro_native_type {
    /**
     * The human readable name of this type, mainly for debugging.
     * Must be a valid zero terminated string.
     */
    const char* name;

    /**
     * This function will be invoked exactly once for each object when it is being garbage collected.
     * It may be NULL if no finalization is needed.
     *
     * \param data The address to the native object's data. Any resourced owned by `data` should be freed.
     * \param size The size of the native object, in bytes.
     */
    void (*finalizer)(void* data, size_t size);
} tiro_native_type_t;

/**
 * Constructs a new native object of the given type and size.
 *
 * `type_descriptor` must describe the properties of the new object. The native object will store a copy of this pointer,
 * accessible via `tiro_native_type()`. The pointer must remain valid for the lifetime of the object, which
 * usually means the lifetime of the vm itself.
 *
 * `size` (in bytes) specifies the size that will be allocated as user storage and must be greater than 0.
 *
 * On success, the constructed object will be written to `result` and the object's user storage will be accessible
 * via `tiro_native_data()`.
 */
TIRO_API void tiro_make_native(tiro_vm_t vm, const tiro_native_type_t* type_descriptor, size_t size,
    tiro_handle_t result, tiro_error_t* err);

/**
 * Returns the address of the `tiro_native_type_t` instance that was used to create the given native object.
 * Returns NULL on error.
 */
TIRO_API const tiro_native_type_t* tiro_native_type_descriptor(tiro_vm_t vm, tiro_handle_t object);

/**
 * Returns the address of the allocated user storage of the given native object.
 * Returns NULL on error.
 *
 * \warning
 *  The pointer returned by this function points into the object's current storage.
 *  Because objects may move on the heap (e.g. because of garbage collection), this data may be invalidated.
 *  The data may only be used immediately after calling this function in native code that is guaranteed to NOT allocate on the tiro heap.
 *  It MUST NOT be used as input tiro an allocating function (which includes most functions of this API), or after such a function has been called.
 */
TIRO_API void* tiro_native_data(tiro_vm_t vm, tiro_handle_t object);

/**
 * Returns the size (in bytes) of the given native object's user storage.
 * Returns 0 on error.
 */
TIRO_API size_t tiro_native_size(tiro_vm_t vm, tiro_handle_t object);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif // TIRO_OBJECTS_H_INCLUDED
