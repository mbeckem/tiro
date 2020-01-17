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

/**
 * The tiro_settings structure can be provided to `tiro_context_new` as a
 * configuration parameter.
 */
typedef struct tiro_settings {
    /* Userdata pointer that will be passed to error_log. Defaults to NULL. */
    void* error_log_data;

    /* Logging function for internal errors. Defaults to a function that prints to stdout. */
    void (*error_log)(const char* error_message, void* error_log_data);
} tiro_settings;

typedef struct tiro_context tiro_context;
typedef struct tiro_diagnostics tiro_diagnostics;

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
 * Attempts to compile and load the given module source code.
 * The module will be registered under the name `module_name`.
 * 
 * If `diag` is not null, then errors and warnings that occurr during
 * parsing or compilation of the source code will be stored in that object.
 */
TIRO_API tiro_error tiro_context_load(tiro_context* ctx,
    const char* module_name, const char* module_source,
    tiro_diagnostics* diag);

/**
 * Allocates a new diagnostics instance.
 * Diagnostics instances are used to store diagnostic error/warning messages
 * that occurr during parsing/compilation.
 * 
 * The diagnostics instance must not outlive the context object.
 * 
 * Returns NULL on allocation failure.
 */
TIRO_API tiro_diagnostics* tiro_diagnostics_new(tiro_context* ctx);

/**
 * Free an existing diagnostics instance. Must be called exactly once
 * for every diagnostics instance created with `tiro_diagnostics_new`.
 * The freed instance must not be used anymore.
 * 
 * Does nothing if `diag` is NULL.
 */
TIRO_API void tiro_diagnostics_free(tiro_diagnostics* diag);

/**
 * Resets the messages in the given diagnostics instance.
 */
TIRO_API void tiro_diagnostics_clear(tiro_diagnostics* diag);

/**
 * Returns true if the given `diag` object contains any messages.
 */
TIRO_API bool tiro_diagnostics_has_messages(tiro_diagnostics* diag);

/**
 * Prints the messages in the given diagnostics instance to stdout.
 * 
 * TODO: More options for printing.
 */
TIRO_API tiro_error tiro_diagnostics_print_stdout(
    tiro_diagnostics* diag);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* TIRO_API_H */
