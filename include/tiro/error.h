#ifndef TIRO_ERROR_H_INCLUDED
#define TIRO_ERROR_H_INCLUDED

#include "tiro/def.h"

#ifdef __cplusplus
extern "C" {
#endif

/** Defines all possible error codes. */
typedef enum tiro_errc_t {
    TIRO_OK = 0,
    TIRO_ERROR_BAD_STATE,          /* Instance is not in the correct state */
    TIRO_ERROR_BAD_ARG,            /* Invalid argument */
    TIRO_ERROR_BAD_SOURCE,         /* Invalid source code */
    TIRO_ERROR_BAD_TYPE,           /* Operation not supported on type */
    TIRO_ERROR_MODULE_EXISTS,      /* Module name defined more than once */
    TIRO_ERROR_MODULE_NOT_FOUND,   /* Requested module does not exist */
    TIRO_ERROR_FUNCTION_NOT_FOUND, /* Requested function does not exist */
    TIRO_ERROR_OUT_OF_BOUNDS,      /* Argument was out of bounds */
    TIRO_ERROR_ALLOC,              /* Allocation failure */
    TIRO_ERROR_INTERNAL = 1000,    /* Internal error */
} tiro_errc_t;

/**
 * Returns the name of the given error code.
 * The string points into static storage and must not be freed.
 */
TIRO_API const char* tiro_errc_name(tiro_errc_t e);

/**
 * Returns a human readable description of the given error code.
 * The string points into static storage and must not be freed.
 */
TIRO_API const char* tiro_errc_message(tiro_errc_t e);

/**
 * Represents an execution error within an api function. Objects of this type
 * may contains rich error information such as a detailed error message
 * or file/line information where the error occurred (in debug builds).
 *
 * The general error handling approach in this library is as follows:
 *
 * All API functions that may fail return an `tiro_errc_t` value to indicate
 * success or error. The exception to this scheme are functions that simply allocate memory - those
 * return a NULL pointer on error.
 *
 * An additional argument of type `tiro_error_t*` may be provided by the caller to obtain
 * detailed error information. This argument is always optional and may simply be `NULL`
 * if not needed. When an api function reports an error, the `tiro_error` will be initialized
 * and must be checked (and freed!) by the caller.
 *
 *  Example:
 *      tiro_error_t error = NULL;                               // Initialize to NULL
 *      if ((operation_may_fail(arg, &error)) != TIRO_OK) {     // Pass &error for details
 *          report_operation_error(error);
 *          tiro_error_free(error);
 *      }
 */
struct tiro_error;

/**
 * Frees the given error instance. Does nothing if `err` is NULL.
 */
TIRO_API void tiro_error_free(tiro_error_t err);

/**
 * Returns the error code stored in the given error.
 * Returns `TIRO_OK` if `err` is NULL.
 */
TIRO_API tiro_errc_t tiro_error_errc(tiro_error_t err);

/**
 * Returns the name of the error code in the given error.
 */
TIRO_API const char* tiro_error_name(tiro_error_t err);

/**
 * Returns the human readable message of the error code in the given error.
 */
TIRO_API const char* tiro_error_message(tiro_error_t err);

/**
 * Returns detailed error information as a human readable string.
 * The string will never be null, but it may be empty if detailed information
 * are not available.
 * 
 * The returned string is managed by the error and will remain valid for
 * as long as the error is not modified or freed.
 */
TIRO_API const char* tiro_error_details(tiro_error_t err);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif // TIRO_ERROR_H_INCLUDED
