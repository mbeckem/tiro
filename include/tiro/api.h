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

#ifdef __cplusplus
extern "C" {
#endif

typedef enum tiro_error {
    TIRO_OK = 0,
    TIRO_ERROR_BAD_STATE,     /* Instance is not in the correct state */
    TIRO_ERROR_BAD_ARG,       /* Invalid argument */
    TIRO_ERROR_BAD_SOURCE,    /* Invalid source code */
    TIRO_ERROR_MODULE_EXISTS, /* Module name defined more than once */
    TIRO_ERROR_ALLOC,         /* Allocation failure */
    TIRO_ERROR_INTERNAL,      /* Internal error */
} tiro_error;

/**
 * Returns the string representation of the given error code.
 * The returned string is allocated in static storage and MUST NOT
 * be freed.
 */
TIRO_API const char* tiro_error_str(tiro_error error);

typedef enum tiro_severity {
    TIRO_SEVERITY_WARNING, /* A compiler warning */
    TIRO_SEVERITY_ERROR,   /* A compiler error (compilation fails) */
} tiro_severity;

/**
 * Returns the string representation of the given severity value.
 * The returned string is allocated in static storage and MUST NOT
 * be freed.
 */
TIRO_API const char* tiro_severity_str(tiro_severity severity);

/**
 * The tiro_settings structure can be provided to `tiro_context_new` as a
 * configuration parameter.
 * Use tiro_settings_init to initialize this struct to default values.
 */
typedef struct tiro_settings {
    /* Userdata pointer that will be passed to error_log. Defaults to NULL. */
    void* error_log_data;

    /* Logging function for internal errors. Defaults to a function that prints to stdout. */
    void (*error_log)(const char* error_message, void* error_log_data);
} tiro_settings;

/**
 * The context encapsulates an instance of the tiro runtime. All contexts
 * are isolated from each other.
 */
typedef struct tiro_context tiro_context;

/**
 * An instance of this type can be passed to the compiler to configure it.
 * Use tiro_compiler_settings_init to initialize this struct to default values.
 */
typedef struct tiro_compiler_settings {
    /* Userdata pointer that will be passed to message_callback. Defaults to NULL. */
    void* message_data;

    /* Will be invoked for every diagnostic message emitted by the compiler.
     * The default function prints messages to stdout. */
    void (*message_callback)(tiro_severity severity, uint32_t line,
        uint32_t column, const char* message, void* userdata);
} tiro_compiler_settings;

/**
 * The compiler instance compiles a set of source file to a module.
 */
typedef struct tiro_compiler tiro_compiler;

/**
 * Initializes the given tiro settings object with default values.
 */
TIRO_API void tiro_settings_init(tiro_settings* settings);

/**
 * Allocates a new context.
 * Reads settings from the given `settings` objects, if it is not NULL.
 * Otherwise uses default values.
 *
 * Returns NULL on allocation failure.
 */
TIRO_API tiro_context* tiro_context_new(const tiro_settings* settings);

/**
 * Free an existing context. Must be called exactly once
 * for every context created with `tiro_context_new`.
 * The freed context must not be used anymore.
 *
 * Does nothing if `ctx` is NULL.
 */
TIRO_API void tiro_context_free(tiro_context* ctx);

/**
 * Initializes the given compiler settings object with default values.
 */
TIRO_API void tiro_compiler_settings_init(tiro_compiler_settings* settings);

/**
 * Allocates a new compiler instance. A compiler can be used to compile
 * a set of source files into a module. Warnings or errors emitted during
 * compilation can be observed through the `settings->message_callback` function.
 *
 * \param ctx The context that this compiler shall belong to. The compiler
 * must not outlive the context.
 *
 * \param settings The compiler settings (optional). Default values will be used if
 * this parameter is NULL.
 *
 * FIXME: Currently only works for a single source files, implement _add api.
 */
TIRO_API tiro_compiler*
tiro_compiler_new(tiro_context* ctx, const tiro_compiler_settings* settings);

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
TIRO_API
tiro_error tiro_compiler_add_file(
    tiro_compiler* compiler, const char* file_name, const char* file_content);

/**
 * Run the compiler on the set of source files provided via `tiro_compiler_add_file`.
 * Requires at least once source file.
 * This function can only be called once for every compiler instance.
 *
 * Returns an error if the compilation fails.
 */
TIRO_API
tiro_error tiro_compiler_run(tiro_compiler* compiler);

/**
 * Returns the string representation of the AST.
 * Can only be called after `tiro_compiler_run` has been executed. The compile
 * process can have failed, a somewhat useful AST can often still be produced.
 *
 * Returns `TIRO_ERROR_BAD_SOURCE` if no useful string representation can be
 * produced.
 *
 * The resulting string must be passed to `free`.
 */
TIRO_API
tiro_error tiro_compiler_dump_ast(tiro_compiler* compiler, char** string);

/**
 * Disassembles the compiled module.
 * Can only be called after `tiro_compiler_run` has been executed.
 *
 * The resulting string must be passed to `free`.
 */
TIRO_API
tiro_error tiro_compiler_disassemble(tiro_compiler* compiler, char** string);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* TIRO_API_H */
