#ifndef TIRO_DEF_H_INCLUDED
#define TIRO_DEF_H_INCLUDED

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#if DOXYGEN
#    define TIRO_API
#elif defined(_WIN32) || defined(__CYGWIN__)
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

#if DOXYGEN
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

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif // TIRO_DEF_H_INCLUDED
