#ifndef HAMMER_API_H
#define HAMMER_API_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#if defined(_WIN32) || defined(__CYGWIN__)
#    ifdef HAMMER_BUILDING_DLL
#        ifdef __GNUC__
#            define HAMMER_API __attribute__((dllexport))
#        else
#            define HAMMER_API __declspec(dllexport)
#        endif
#    else
#        ifdef __GNUC__
#            define HAMMER_API __attribute__((dllimport))
#        else
#            define HAMMER_API __declspec(dllimport)
#        endif
#    endif
#else
#    define HAMMER_API __attribute__((visibility("default")))
#endif

#ifdef __cplusplus
extern "C" {
#endif

typedef enum hammer_error {
    HAMMER_OK = 0,
    HAMMER_ERROR_BAD_ARG,       /* Invalid argument */
    HAMMER_ERROR_BAD_SOURCE,    /* Invalid source code */
    HAMMER_ERROR_MODULE_EXISTS, /* Module name defined more than once */
    HAMMER_ERROR_ALLOC,         /* Allocation failure */
    HAMMER_ERROR_INTERNAL,      /* Internal error */
} hammer_error;

/**
 * Returns the string representation of the given error code.
 * The returned string is allocated in static storage and MUST NOT
 * be freed.
 */
HAMMER_API const char* hammer_error_str(hammer_error error);

/**
 * The hammer_settings structure can be provided to `hammer_context_new` as a
 * configuration parameter.
 */
typedef struct hammer_settings {
    /* Userdata pointer that will be passed to error_log. Defaults to NULL. */
    void* error_log_data;

    /* Logging function for internal errors. Defaults to a function that prints to stdout. */
    void (*error_log)(const char* error_message, void* error_log_data);
} hammer_settings;

typedef struct hammer_context hammer_context;
typedef struct hammer_diagnostics hammer_diagnostics;

/**
 * Initializes the given hammer settings object with default values.
 */
HAMMER_API void hammer_settings_init(hammer_settings* settings);

/**
 * Allocates a new context.
 * Reads settings from the given `settings` objects, if it is not NULL.
 * Otherwise uses default values.
 * 
 * Returns NULL on allocation failure.
 */
HAMMER_API hammer_context* hammer_context_new(const hammer_settings* settings);

/**
 * Free an existing context. Must be called exactly once
 * for every context created with `hammer_context_new`.
 * The freed context must not be used anymore.
 * 
 * Does nothing if `ctx` is NULL.
 */
HAMMER_API void hammer_context_free(hammer_context* ctx);

/**
 * Attempts to compile and load the given module source code.
 * The module will be registered under the name `module_name`.
 * 
 * If `diag` is not null, then errors and warnings that occurr during
 * parsing or compilation of the source code will be stored in that object.
 */
HAMMER_API hammer_error hammer_context_load(hammer_context* ctx,
    const char* module_name, const char* module_source,
    hammer_diagnostics* diag);

/**
 * Allocates a new diagnostics instance.
 * Diagnostics instances are used to store diagnostic error/warning messages
 * that occurr during parsing/compilation.
 * 
 * The diagnostics instance must not outlive the context object.
 * 
 * Returns NULL on allocation failure.
 */
HAMMER_API hammer_diagnostics* hammer_diagnostics_new(hammer_context* ctx);

/**
 * Free an existing diagnostics instance. Must be called exactly once
 * for every diagnostics instance created with `hammer_diagnostics_new`.
 * The freed instance must not be used anymore.
 * 
 * Does nothing if `diag` is NULL.
 */
HAMMER_API void hammer_diagnostics_free(hammer_diagnostics* diag);

/**
 * Resets the messages in the given diagnostics instance.
 */
HAMMER_API void hammer_diagnostics_clear(hammer_diagnostics* diag);

/**
 * Returns true if the given `diag` object contains any messages.
 */
HAMMER_API bool hammer_diagnostics_has_messages(hammer_diagnostics* diag);

/**
 * Prints the messages in the given diagnostics instance to stdout.
 * 
 * TODO: More options for printing.
 */
HAMMER_API hammer_error hammer_diagnostics_print_stdout(
    hammer_diagnostics* diag);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* HAMMER_API_H */
