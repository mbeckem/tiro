#ifndef TIRO_DEF_H_INCLUDED
#define TIRO_DEF_H_INCLUDED

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

typedef struct tiro_vm tiro_vm;
typedef struct tiro_compiler tiro_compiler;
typedef struct tiro_module tiro_module;
typedef struct tiro_frame tiro_frame;
typedef struct tiro_value tiro_value;
typedef tiro_value* tiro_handle;

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif // TIRO_DEF_H_INCLUDED
