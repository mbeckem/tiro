#ifndef TIRO_COMPILER_H_INCLUDED
#define TIRO_COMPILER_H_INCLUDED

/**
 * \file
 * \brief Contains functions and type definitions for compiling tiro source code to modules.
 *
 */

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
 * Represents a diagnostic message emitted by the compiler.
 * All fields are only valid for the duration of the `message_callback` function call.
 */
typedef struct tiro_compiler_message {
    /** The severity of this message */
    tiro_severity_t severity;

    /** The relevant source file. May be empty if there is no source file associated with this message. */
    tiro_string_t file;

    /** Source line (1 based). Zero if unavailable. */
    uint32_t line;

    /** Source column (1 based). Zero if unavailable. */
    uint32_t column;

    /** The message text. */
    tiro_string_t text;
} tiro_compiler_message_t;

/**
 * An instance of this type can be passed to the compiler to configure it.
 * Use tiro_compiler_settings_init to initialize this struct to default values.
 */
typedef struct tiro_compiler_settings {
    /* Compiler will remember the CST, this enables the `tiro_compiler_dump_cst` function. */
    bool enable_dump_cst;

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

    /*
     * Will be invoked for every diagnostic message emitted by the compiler.
     * The default function prints messages to stdout.
     *
     * Should usually return true, but may return false to indicate a fatal error (compilation will halt).
     */
    bool (*message_callback)(const tiro_compiler_message_t* message, void* userdata);
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
 * \param module_name
 *      The name of the compiled module. Must be a valid, non-empty string.
 *      Does not have to remain valid for after the completion of this function, a copy is made internally.
 *
 * \param settings
 *      The compiler settings (optional). Default values will be used if this parameter is NULL.
 *      Does not have to remain valid after the completion of this function.
 */
TIRO_API TIRO_WARN_UNUSED tiro_compiler_t tiro_compiler_new(
    tiro_string_t module_name, const tiro_compiler_settings_t* settings, tiro_error_t* err);

/**
 * Destroys and frees the given compiler instance. Must be called exactly once
 * for every instance created via `tiro_compiler_new` in order to avoid resource leaks.
 * Does nothing if `compiler` is NULL.
 */
TIRO_API void tiro_compiler_free(tiro_compiler_t compiler);

/**
 * Add a source file to the compiler's source set.
 * Can only be called before compilation started.
 *
 * Filenames should be unique within a single module.
 */
TIRO_API void tiro_compiler_add_file(tiro_compiler_t compiler, tiro_string_t file_name,
    tiro_string_t file_content, tiro_error_t* err);

/**
 * Run the compiler on the set of source files provided via `tiro_compiler_add_file`.
 * Requires at least one source file.
 * This function can only be called once for every compiler instance.
 *
 * Returns an error if the compilation fails.
 */
TIRO_API void tiro_compiler_run(tiro_compiler_t compiler, tiro_error_t* err);

/**
 * Returns true if this compiler has successfully compiled a set of source files
 * and produced a bytecode module. In order for this function to return true,
 * a previous call to `tiro_compiler_run` must have returned `TIRO_OK` and
 * the compiler must have been configured to actually produce a module.
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
 * Returns the string representation of the concrete syntax tree (CST).
 * Can only be called after `tiro_compiler_run` has been executed. The compile
 * process can have failed; a somewhat useful CST can often still be produced.
 *
 * Returns `TIRO_ERROR_BAD_STATE` if the compiler cannot produce the CST.
 *
 * Otherwise, this function returns `TIRO_OK` and returns a new string using the provided
 * output parameter. The string must be passed to `free` to release memory.
 */
TIRO_API void tiro_compiler_dump_cst(tiro_compiler_t compiler, char** string, tiro_error_t* err);

/**
 * Returns the string representation of the abstract syntax tree (AST).
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
