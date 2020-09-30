#ifndef TIRO_VM_H_INCLUDED
#define TIRO_VM_H_INCLUDED

#include "tiro/def.h"
#include "tiro/error.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * The tiro_vm_settings structure can be provided to `tiro_vm_new` as a
 * configuration parameter.
 * Use tiro_vm_settings_init to initialize this struct to default values.
 */
typedef struct tiro_vm_settings {
    /**
     * Arbitrary user data that will be accessible by calling `tiro_vm_userdata()`. This value
     * is never interpreted in any way. This value is NULL by default.
     */
    void* userdata;
} tiro_vm_settings_t;

/**
 * Initializes the given tiro settings object with default values.
 */
TIRO_API void tiro_vm_settings_init(tiro_vm_settings_t* settings);

/**
 * An instance of the tiro virtual machine. All virtual machines
 * are isolated from each other.
 */
struct tiro_vm;

/**
 * Allocates a new virtual machine instance.
 * Reads settings from the given `settings` objects, if it is not NULL.
 * Otherwise uses default values.
 *
 * Returns NULL on allocation failure.
 */
TIRO_API TIRO_WARN_UNUSED tiro_vm_t tiro_vm_new(
    const tiro_vm_settings_t* settings, tiro_error_t* err);

/**
 * Free a virtual machine. Must be called exactly once
 * for every vm created with `tiro_vm_new`.
 *
 * Does nothing if `vm` is NULL.
 */
TIRO_API void tiro_vm_free(tiro_vm_t vm);

/**
 * Returns the userdata pointer that was passed in the settings struct during vm construction.
 */
TIRO_API void* tiro_vm_userdata(tiro_vm_t vm);

/**
 * Load the default modules provided by the runtime.
 *
 * TODO: Configuration?
 */
TIRO_API void tiro_vm_load_std(tiro_vm_t vm, tiro_error_t* err);

/**
 * Loads the compiled module into the virtual machine.
 * 
 * Note: this function does *not* take ownership of the module parameter.
 */
TIRO_API void tiro_vm_load_bytecode(tiro_vm_t vm, tiro_module_t module, tiro_error_t* err);

/**
 * Loads the given module object into the virtual machine.
 * Returns `TIRO_ERROR_MODULE_EXISTS` if a module with the same name already exists.
 * Returns `TIRO_ERROR_BAD_TYPE` if the argument is not actually a module.
 */
TIRO_API void tiro_vm_load_module(tiro_vm_t vm, tiro_handle_t module, tiro_error_t* err);

/**
 * Attempts to find the exported value with the given name in the specified module. The found function value
 * will be stored in the `result` handle, which must not be NULL.
 * 
 * Returns `TIRO_ERROR_MODULE_NOT_FOUND` if the specified module was not loaded.
 * Returns `TIRO_ERROR_EXPORT_NOT_FOUND` if the module does not contain an exported member with that name.
 */
TIRO_API void tiro_vm_get_export(tiro_vm_t vm, const char* module_name, const char* function_name,
    tiro_handle_t result, tiro_error_t* err);

/**
 * Calls the given function and places the function's return value into `result` (if present).
 * 
 * FIXME: Remove this, calling must be async.
 * 
 * \param vm        The virtual machine instance.
 * \param function  The function to call. Must not be NULL.
 * \param arguments The function call arguments. Must be a tuple if arguments shall be passed, or a null value or NULL pointer
 *                  to indicate zero arguments.
 * \param result    A handle in which the function's return value will be placed. Can be NULL.
 * \param err       An optional error handle for detailed error information. 
 */
TIRO_API void tiro_vm_call(tiro_vm_t vm, tiro_handle_t function, tiro_handle_t arguments,
    tiro_handle_t result, tiro_error_t* err);

/**
 * Runs all ready coroutines. Returns (and does not block) when all coroutines are either waiting or done.
 */
TIRO_API void tiro_vm_run_ready(tiro_vm_t vm, tiro_error_t* err);

/** 
 * Returns true if the virtual machine has at least one coroutine ready for execution, false otherwise.
 */
TIRO_API bool tiro_vm_has_ready(tiro_vm_t vm);

/**
 * Allocates a new global handle. Global handles point to a single rooted object slot that can hold
 * an arbitrary value. Slots are always initialized to null.
 * 
 * When a global handle is no longer required, it should be freed by calling `tiro_global_free`.
 * 
 * Returns NULL on allocation failure.
 */
TIRO_API TIRO_WARN_UNUSED tiro_handle_t tiro_global_new(tiro_vm_t vm, tiro_error_t* err);

/**
 * Frees a global handle allocated with `tiro_global_new`.
 * 
 * Note: remaining globals are automatically freed when a vm is freed.
 */
TIRO_API void tiro_global_free(tiro_vm_t vm, tiro_handle_t global);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif // TIRO_VM_H_INCLUDED
