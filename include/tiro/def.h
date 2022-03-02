#ifndef TIRO_DEF_H_INCLUDED
#define TIRO_DEF_H_INCLUDED

/**
 * \file
 * \brief Contains basic type and macro definitions.
 *
 * This file is included by all other api headers.
 */

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#if TIRO_BUILDING_DOCS
#    define TIRO_API
#elif defined(_WIN32) || defined(__CYGWIN__)
#    ifdef TIRO_BUILDING_LIBRARY
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

#if TIRO_BUILDING_DOCS
#    define TIRO_WARN_UNUSED
#elif defined(__GNUC__) || defined(__clang__)
#    define TIRO_WARN_UNUSED __attribute__((warn_unused_result))
#elif defined(_MSC_VER)
#    define TIRO_WARN_UNUSED _Check_return_
#else
#    define TIRO_WARN_UNUSED
#endif

#ifdef __cplusplus
extern "C" {
#endif

typedef struct tiro_error* tiro_error_t;
typedef struct tiro_vm* tiro_vm_t;
typedef struct tiro_compiler* tiro_compiler_t;
typedef struct tiro_module* tiro_module_t;
typedef struct tiro_value* tiro_handle_t;
typedef struct tiro_sync_frame* tiro_sync_frame_t;
typedef struct tiro_async_frame* tiro_async_frame_t;
typedef struct tiro_async_token* tiro_async_token_t;
typedef struct tiro_resumable_frame* tiro_resumable_frame_t;

/**
 * Represents a string that is not necessarily zero terminated.
 * `data` must have `length` readable bytes.
 * `data` is allowed to be NULL (or any other invalid pointer) if and only if `length` is 0.
 */
typedef struct tiro_string {
    /** string data, utf8 encoded. */
    const char* data;

    /** number of bytes in `data`. */
    size_t length;
} tiro_string_t;

/**
 * Helper function to construct a tiro_string_t from a zero terminated string.
 * Internally calls strlen on non-NULL inputs to determine their length.
 *
 * \param data a zero terminated string, or NULL.
 */
inline tiro_string_t tiro_cstr(const char* data) {
    tiro_string_t result;
    result.data = data;
    result.length = data ? strlen(data) : 0;
    return result;
}

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif // TIRO_DEF_H_INCLUDED
