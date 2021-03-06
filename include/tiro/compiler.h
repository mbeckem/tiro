#ifndef TIRO_COMPILER_H_INCLUDED
#define TIRO_COMPILER_H_INCLUDED

#include "tiro/def.h"
#include "tiro/error.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Defines the possible values for the severity of diagnostic compiler messages.
 */
typedef enum tiro_severity {
    TIRO_SEVERITY_WARNING = 1, ///< A compiler warning
    TIRO_SEVERITY_ERROR = 2,   ///< A compiler error (compilation fails)
} tiro_severity_t;

/**
 * Returns the string representation of the given severity value.
 * The returned string is allocated in static storage and MUST NOT
 * be freed.
 */
TIRO_API const char* tiro_severity_str(tiro_severity_t severity);

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

    /* TODO: Skip codegen flag */

    /* Userdata pointer that will be passed to message_callback. Defaults to NULL. */
    void* message_callback_data;

    /* Will be invoked for every diagnostic message emitted by the compiler.
     * The default function prints messages to stdout. */
    void (*message_callback)(tiro_severity_t severity, uint32_t line, uint32_t column,
        const char* message, void* userdata);
} tiro_compiler_settings_t;

/**
 * Initializes the given compiler settings object with default values.
 */
TIRO_API void tiro_compiler_settings_init(tiro_compiler_settings_t* settings);

/**
 * The compiler instance translates a set of source file into a module.
 */
struct tiro_compiler;

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
TIRO_API TIRO_WARN_UNUSED tiro_compiler_t tiro_compiler_new(
    const tiro_compiler_settings_t* settings, tiro_error_t* err);

/**
 * Destroys and frees the given compiler instance. Must be called exactly once
 * for every instance created via `tiro_compiler_new` in order to avoid resource leaks.
 * Does nothing if `compiler` is NULL.
 */
TIRO_API void tiro_compiler_free(tiro_compiler_t compiler);

/**
 * Add a source file to the compiler. Can only be called before compilation started.
 *
 * FIXME: Can only be called for a single source file as of now.
 */
TIRO_API void tiro_compiler_add_file(
    tiro_compiler_t compiler, const char* file_name, const char* file_content, tiro_error_t* err);

/**
 * Run the compiler on the set of source files provided via `tiro_compiler_add_file`.
 * Requires at least once source file.
 * This function can only be called once for every compiler instance.
 *
 * Returns an error if the compilation fails.
 */
TIRO_API void tiro_compiler_run(tiro_compiler_t compiler, tiro_error_t* err);

/**
 * Returns true if this compiler has successfully compiled a set of source files
 * and produced a bytecode module. In order for this function to return true,
 * a previous call to `tiro_compiler_run` must have returned `TIRO_OK` and
 * the compiler must have beeen configured to actually produce a module.
 */
TIRO_API
bool tiro_compiler_has_module(tiro_compiler_t compiler);

/**
 * Extracts the compiled module from the compiler and returns it.
 * On success, the module will be placed into the location specified by `module`, which
 * must not be NULL. If a module was returned, it must be freed by calling `tiro_module_free`.
 *
 * This function fails if `tiro_compiler_has_module` returns false.
 */
TIRO_API void
tiro_compiler_take_module(tiro_compiler_t compiler, tiro_module_t* module, tiro_error_t* err);

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
TIRO_API void tiro_compiler_dump_ast(tiro_compiler_t compiler, char** string, tiro_error_t* err);

/**
 * Returns the string representation of the internal representation immediately before
 * code generation. Can only be called after `tiro_compiler_run` has been executed successfully.
 *
 * Returns `TIRO_ERROR_BAD_STATE` if the compiler cannot produce the internal representation.
 *
 * Otherwise, this function returns `TIRO_OK` and returns a new string using the provided
 * output parameter. The string must be passed to `free` to release memory.
 */
TIRO_API void tiro_compiler_dump_ir(tiro_compiler_t compiler, char** string, tiro_error_t* err);

/**
 * Returns the string representation of the compiled bytecode module.
 * Can only be called after `tiro_compiler_run` has been executed successfully.
 *
 * Returns `TIRO_ERROR_BAD_STATE` if the compiler cannot produce the disassembled output.
 *
 * Otherwise, this function returns `TIRO_OK` and returns a new string using the provided
 * output parameter. The string must be passed to `free` to release memory.
 */
TIRO_API void
tiro_compiler_dump_bytecode(tiro_compiler_t compiler, char** string, tiro_error_t* err);

/**
 * Represents a compiled module. Modules can be loaded into a vm for execution.
 * Modules instances can be created by compiling source code using the `tiro_compiler`.
 */
struct tiro_module;

/**
 * Free a module. Must be called exactly once
 * for every created module.
 *
 * Does nothing if `module` is NULL.
 */
TIRO_API void tiro_module_free(tiro_module_t module);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif // TIRO_COMPILER_H_INCLUDED
