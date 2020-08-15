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
    /* TODO */
    int _not_empty_; /* prevent size 0 warning (clang) */
} tiro_vm_settings;

/**
 * Initializes the given tiro settings object with default values.
 */
TIRO_API void tiro_vm_settings_init(tiro_vm_settings* settings);

/**
 * An instance of the tiro virtual machine. All virtual machines
 * are isolated from each other.
 */
typedef struct tiro_vm tiro_vm;

/**
 * Allocates a new virtual machine instance.
 * Reads settings from the given `settings` objects, if it is not NULL.
 * Otherwise uses default values.
 *
 * Returns NULL on allocation failure.
 */
TIRO_API TIRO_WARN_UNUSED tiro_vm* tiro_vm_new(const tiro_vm_settings* settings);

/**
 * Free a virtual machine. Must be called exactly once
 * for every vm created with `tiro_vm_new`.
 *
 * Does nothing if `vm` is NULL.
 */
TIRO_API void tiro_vm_free(tiro_vm* vm);

/**
 * Load the default modules provided by the runtime.
 *
 * TODO: Configuration?
 */
TIRO_API tiro_errc tiro_vm_load_std(tiro_vm* vm, tiro_error** err);

/**
 * Loads the compiled module into virtual machine.
 */
TIRO_API tiro_errc tiro_vm_load(tiro_vm* vm, const tiro_module* module, tiro_error** err);

/**
 * Attempts to find the exported function with the given name in the specified module. The found function value
 * will be stored in the `result` handle, which must not be NULL.
 * 
 * Returns `TIRO_ERROR_MODULE_NOT_FOUND` if the specified module was not loaded.
 * Returns `TIRO_ERROR_FUNCTION_NOT_FOUND` if the module does not contain the requested function.
 */
TIRO_API tiro_errc tiro_vm_find_function(tiro_vm* vm, const char* module_name,
    const char* function_name, tiro_handle result, tiro_error** err);

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
TIRO_API tiro_errc tiro_vm_call(
    tiro_vm* vm, tiro_handle function, tiro_handle arguments, tiro_handle result, tiro_error** err);

/**
 * Runs all ready coroutines. Returns (and does not block) when all coroutines are either waiting or done.
 */
TIRO_API tiro_errc tiro_vm_run_ready(tiro_vm* vm, tiro_error** err);

/** 
 * Returns true if the virtual machine has at least one coroutine ready for execution, false otherwise.
 */
TIRO_API bool tiro_vm_has_ready(tiro_vm* vm);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif // TIRO_VM_H_INCLUDED
