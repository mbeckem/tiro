#ifndef TIRO_API_H
#define TIRO_API_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#if defined(_WIN32) || defined(__CYGWIN__)
#    ifdef TIRO_BUILDING_DLL
#        ifdef __GNUC__
#            define TIRO_API __attribute__((dllexport))
#        else
#            define TIRO_API __declspec(dllexport)
#        endif
#    else
#        ifdef __GNUC__
#            define TIRO_API __attribute__((dllimport))
#        else
#            define TIRO_API __declspec(dllimport)
#        endif
#    endif
#else
#    define TIRO_API __attribute__((visibility("default")))
#endif

#if defined(__GNUC__) || defined(__clang__)
#    define TIRO_WARN_UNUSED __attribute__((warn_unused_result))
#elif defined(_MSC_VER)
#    define TIRO_WARN_UNUSED _Check_return_
#else
#    define TIRO_WARN_UNUSED
#endif

#ifdef __cplusplus
extern "C" {
#endif

typedef struct tiro_compiler tiro_compiler;
typedef struct tiro_module tiro_module;

/**
 * Defines all possible error codes.
 */
typedef enum tiro_errc {
    TIRO_OK = 0,
    TIRO_ERROR_BAD_STATE,     /* Instance is not in the correct state */
    TIRO_ERROR_BAD_ARG,       /* Invalid argument */
    TIRO_ERROR_BAD_SOURCE,    /* Invalid source code */
    TIRO_ERROR_MODULE_EXISTS, /* Module name defined more than once */
    TIRO_ERROR_ALLOC,         /* Allocation failure */
    TIRO_ERROR_INTERNAL,      /* Internal error */
} tiro_errc;

/**
 * Returns the name of the given error code.
 * The string points into static storage and must not be freed.
 */
TIRO_API const char* tiro_errc_name(tiro_errc e);

/**
 * Returns the human readable description of the given error code.
 * The string points into static storage and must not be freed.
 */
TIRO_API const char* tiro_errc_message(tiro_errc e);

/**
 * Represents an execution error within a api function. Objects of this type
 * may contains rich error information such as a detailed error message
 * or file/line information where the error occurred (in debug builds).
 *
 * The general error handling approach in this library is as follows:
 *
 * All API functions that may fail return an `tiro_errc` value to indicate
 * success or error. The exception are function that simply allocate memory - those
 * return a NULL pointer on error.
 *
 * An additional argument of type `tiro_error**` may be provided by the caller to obtain
 * detailed error information. This argument is always optional and may simply be `NULL`
 * if not needed. When an api function reports an error, the `tiro_error` will be initialized
 * and must be checked (and freed!) by the caller.
 *
 *  Example:
 *      tiro_error* error = NULL;                               // Initialize to NULL
 *      if ((operation_may_fail(arg, &error)) != TIRO_OK) {     // Pass &error for details
 *          report_operation_error(error);
 *          tiro_error_free(error);
 *      }
 */
typedef struct tiro_error tiro_error;

/**
 * Frees the given error instance. Does nothing if `err` is NULL.
 */
TIRO_API void tiro_error_free(tiro_error* err);

/**
 * Returns the error code stored in the given error.
 * Returns `TIRO_OK` if `err` is NULL.
 */
TIRO_API tiro_errc tiro_error_errc(const tiro_error* err);

/**
 * Returns the name of the error code in the given error.
 */
TIRO_API const char* tiro_error_name(const tiro_error* err);

/**
 * Returns the human readable message of the error code in the given error.
 */
TIRO_API const char* tiro_error_message(const tiro_error* err);

/**
 * Returns detailed error information as a human readable string.
 * The returned string is managed by the error and will remain valid for
 * as long as the error is not modified or freed.
 */
TIRO_API const char* tiro_error_details(const tiro_error* err);

/**
 * Returns the file in which the error originated.
 * The returned string is managed by the error and will remain valid for
 * as long as the error is not modified or freed.
 *
 * The returned string will be empty if file information are not available.
 */
TIRO_API const char* tiro_error_file(const tiro_error* err);

/**
 * Returns the line in which the error originated.
 * Returns `0` if line information are not available.
 */
TIRO_API int tiro_error_line(const tiro_error* error);

/**
 * Returns the function name where the error originated.
 * The returned string is managed by the error and will remain valid for
 * as long as the error is not modified or freed.
 *
 * The returned string will be empty if function information are not available.
 */
TIRO_API const char* tiro_error_func(const tiro_error* err);

/**
 * Defines the possible values for the severity of diagnostic compiler messages.
 */
typedef enum tiro_severity {
    TIRO_SEVERITY_WARNING = 1, /* A compiler warning */
    TIRO_SEVERITY_ERROR,       /* A compiler error (compilation fails) */
} tiro_severity;

/**
 * Returns the string representation of the given severity value.
 * The returned string is allocated in static storage and MUST NOT
 * be freed.
 */
TIRO_API const char* tiro_severity_str(tiro_severity severity);

/**
 * The tiro_vm_settings structure can be provided to `tiro_vm_new` as a
 * configuration parameter.
 * Use tiro_vm_settings_init to initialize this struct to default values.
 */
typedef struct tiro_vm_settings {
    /* TODO */
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
TIRO_API TIRO_WARN_UNUSED tiro_vm*
tiro_vm_new(const tiro_vm_settings* settings);

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
TIRO_API TIRO_WARN_UNUSED tiro_errc tiro_vm_load_std(
    tiro_vm* vm, tiro_error** err);

/**
 * Loads the compiled module into virtual machine.
 */
TIRO_API TIRO_WARN_UNUSED tiro_errc tiro_vm_load(
    tiro_vm* vm, const tiro_module* module, tiro_error** err);

/**
 * An instance of this type can be passed to the compiler to configure it.
 * Use tiro_compiler_settings_init to initialize this struct to default values.
 */
typedef struct tiro_compiler_settings {
    /* Compiler will remember the AST, this enables the `tiro_compiler_dump_ast` function. */
    bool enable_dump_ast;

    /* Compiler will remember the IR, this enables the `tiro_compiler_dump_ir` function. */
    bool enable_dump_ir;

    /* Compiler will remember the diassembled bytecode, this enables
     * the `tiro_compiler_dump_bytecode` function. */
    bool enable_dump_bytecode;

    // TODO: Skip codegen flag

    /* Userdata pointer that will be passed to message_callback. Defaults to NULL. */
    void* message_callback_data;

    /* Will be invoked for every diagnostic message emitted by the compiler.
     * The default function prints messages to stdout. */
    void (*message_callback)(tiro_severity severity, uint32_t line,
        uint32_t column, const char* message, void* userdata);
} tiro_compiler_settings;

/**
 * Initializes the given compiler settings object with default values.
 */
TIRO_API void tiro_compiler_settings_init(tiro_compiler_settings* settings);

/**
 * The compiler instance translates a set of source file into a module.
 */
typedef struct tiro_compiler tiro_compiler;

/**
 * Allocates a new compiler instance. A compiler can be used to compile
 * a set of source files into a module. Warnings or errors emitted during
 * compilation can be observed through the `settings->message_callback` function.
 *
 * \param settings The compiler settings (optional). Default values will be used if
 * this parameter is NULL.
 *
 * FIXME: Currently only works for a single source files, implement _add api.
 */
TIRO_API TIRO_WARN_UNUSED tiro_compiler*
tiro_compiler_new(const tiro_compiler_settings* settings);

/**
 * Destroys and frees the given compiler instance. Must be called exactly once
 * for every instance created via `tiro_compiler_new` in order to avoid resource leaks.
 * Does nothing if `compiler` is NULL.
 */
TIRO_API void tiro_compiler_free(tiro_compiler* compiler);

/**
 * Add a source file to the compiler. Can only be called before compilation started.
 *
 * FIXME: Can only be called for a single source file currently.
 */
TIRO_API TIRO_WARN_UNUSED tiro_errc tiro_compiler_add_file(
    tiro_compiler* compiler, const char* file_name, const char* file_content,
    tiro_error** err);

/**
 * Run the compiler on the set of source files provided via `tiro_compiler_add_file`.
 * Requires at least once source file.
 * This function can only be called once for every compiler instance.
 *
 * Returns an error if the compilation fails.
 */
TIRO_API TIRO_WARN_UNUSED tiro_errc tiro_compiler_run(
    tiro_compiler* compiler, tiro_error** err);

/**
 * Returns true if this compiler has successfully compiled a set of source files
 * and produced a bytecode module. In order for this function to return true,
 * a previous call to `tiro_compiler_run` must have returned `TIRO_OK` and
 * the compiler must have beeen configured to actually produce a module.
 */
TIRO_API
bool tiro_compiler_has_module(tiro_compiler* compiler);

/**
 * Extracts the compiled module from the compiler and returns it.
 * On success, the module will placed into the location specified by `module`, which
 * must not be NULL. If a module was returned, it must be freed by calling `tiro_module_free`.
 *
 * This function fails if `tiro_compiler_has_module` returns false.
 */
TIRO_API TIRO_WARN_UNUSED tiro_errc tiro_compiler_take_module(
    tiro_compiler* compiler, tiro_module** module, tiro_error** err);

/**
 * Returns the string representation of the AST.
 * Can only be called after `tiro_compiler_run` has been executed. The compile
 * process can have failed; a somewhat useful AST can often still be produced.
 *
 * Returns `TIRO_ERROR_BAD_STATE` if the compiler cannot produce the AST.
 *
 * Otherwise, this function returns `TIRO_OK` and returns a new string using the provided
 * output parameter. The string must be passed to `free` to release memory.
 */
TIRO_API TIRO_WARN_UNUSED tiro_errc tiro_compiler_dump_ast(
    tiro_compiler* compiler, char** string, tiro_error** err);

/**
 * Returns the string representation of the internal representation immediately before
 * code generation. Can only be called after `tiro_compiler_run` has been executed successfully.
 *
 * Returns `TIRO_ERROR_BAD_STATE` if the compiler cannot produce the internal representation.
 *
 * Otherwise, this function returns `TIRO_OK` and returns a new string using the provided
 * output parameter. The string must be passed to `free` to release memory.
 */
TIRO_API TIRO_WARN_UNUSED tiro_errc tiro_compiler_dump_ir(
    tiro_compiler* compiler, char** string, tiro_error** err);

/**
 * Returns the string representation of the compiled bytecode module.
 * Can only be called after `tiro_compiler_run` has been executed successfully.
 *
 * Returns `TIRO_ERROR_BAD_STATE` if the compiler cannot produce the disassembled output.
 *
 * Otherwise, this function returns `TIRO_OK` and returns a new string using the provided
 * output parameter. The string must be passed to `free` to release memory.
 */
TIRO_API TIRO_WARN_UNUSED tiro_errc tiro_compiler_dump_bytecode(
    tiro_compiler* compiler, char** string, tiro_error** err);

/**
 * Represents a compiled module. Modules can be loaded into a vm for execution.
 * Modules instances can be created by compiling source code using the `tiro_compiler`.
 */
typedef struct tiro_module tiro_module;

/**
 * Free a module. Must be called exactly once
 * for every created module.
 *
 * Does nothing if `module` is NULL.
 */
TIRO_API void tiro_module_free(tiro_module* module);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* TIRO_API_H */
