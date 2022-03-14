#ifndef TIRO_VM_H_INCLUDED
#define TIRO_VM_H_INCLUDED

/**
 * \file
 * \brief Functions and type definitions for working with the tiro virtual machine.
 *
 */

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
     * The size (in bytes) of heap pages allocated by the virtual machine for the storage of most objects.
     * Must be a power of two between 2^16 and 2^24 or zero to use the default value.
     *
     * Smaller pages waste less memory if only small workloads are to be expected.
     * Larger page sizes can be more performant because fewer chunks need to be allocated for the same number of objects.
     *
     * Note that objects that do not fit into a single page reasonably well will be
     * allocated "on the side" using a separate allocation.
     */
    size_t page_size;

    /**
     * The maximum size (in bytes) that can be occupied by the virtual machine's heap.
     * The virtual machine will throw out of memory errors if this limit is reached.
     *
     * The default value (0) will apply a sane default memory limit.
     * Use `SIZE_MAX` for an unconstrained heap size.
     */
    size_t max_heap_size;

    /**
     * Arbitrary user data that will be accessible by calling `tiro_vm_userdata()`. This value
     * is never interpreted in any way. This value is NULL by default.
     */
    void* userdata;

    /**
     * This callback is invoked when the vm attempts to print to the standard output stream,
     * for example when `std.print(...)` has been called. When this is set to NULL (the default),
     * the message will be printed to the process's standard output.
     *
     * \param message The string to print. Not guaranteed to be null terminated.
     * \param userdata The userdata pointer set in this settings instance.
     */
    void (*print_stdout)(tiro_string_t message, void* userdata);

    /**
     * Set this to true to enable capturing of the current call stack trace when an exception
     * is created during a panic.
     * Capturing stack traces has a significant performance impact because many call frames on the
     * call stack have to be visited.
     *
     * Defaults to `false`.
     */
    bool enable_panic_stack_trace;
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
 * Returns the vm's page size (in bytes).
 */
TIRO_API size_t tiro_vm_page_size(tiro_vm_t vm);

/**
 * Returns the vm's maximum heap size (in bytes).
 */
TIRO_API size_t tiro_vm_max_heap_size(tiro_vm_t vm);

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
 * Attempts to find the exported value with the given name in the specified module.
 * The found value will be stored in the `result` handle, which must not be NULL.
 *
 * Returns `TIRO_ERROR_MODULE_NOT_FOUND` if the specified module was not loaded.
 * Returns `TIRO_ERROR_EXPORT_NOT_FOUND` if the module does not contain an exported member with that name.
 */
TIRO_API void tiro_vm_get_export(tiro_vm_t vm, tiro_string_t module_name, tiro_string_t export_name,
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
 *
 * Warning: This function may *NOT* be called after the associated vm has been destroyed.
 */
TIRO_API void tiro_global_free(tiro_handle_t global);

/**
 * Given a global handle allocated via `tiro_global_new`, returns the associated vm.
 *
 * Warning: May only be used if the handle and the associated vm are still valid.
 */
TIRO_API tiro_vm_t tiro_global_get_vm(tiro_handle_t global);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif // TIRO_VM_H_INCLUDED
