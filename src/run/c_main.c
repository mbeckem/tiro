#include "tiro/api.h"

#include <errno.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static bool compile_file(tiro_compiler* comp, const char* file_name,
    bool print_ast, bool print_ir, bool diassemble);
static char* read_source_file(const char* name);

int main(int argc, char** argv) {
    if (argc < 2) {
        printf("Usage: %s FILENAME\n", argv[0]);
        return 1;
    }

    const char* file_name = argv[1];

    tiro_settings settings;
    tiro_settings_init(&settings);

    int ret = 1;
    tiro_error error = TIRO_OK;
    tiro_context* ctx = NULL;
    tiro_compiler* comp = NULL;
    bool print_ast = true;
    bool print_ir = true;
    bool disassemble = true;

    ctx = tiro_context_new(&settings);
    if (!ctx) {
        printf("Failed to allocate context.\n");
        goto error_exit;
    }

    tiro_compiler_settings comp_settings;
    tiro_compiler_settings_init(&comp_settings);
    comp_settings.enable_dump_ast = print_ast;
    comp_settings.enable_dump_ir = print_ir;
    comp_settings.enable_dump_bytecode = disassemble;

    comp = tiro_compiler_new(ctx, &comp_settings);
    if (!comp) {
        printf("Failed to allocate diagnostics.\n");
        goto error_exit;
    }

    if (!compile_file(comp, file_name, print_ast, print_ir, disassemble)) {
        goto error_exit;
    }

    if ((error = tiro_context_load_defaults(ctx)) != TIRO_OK) {
        printf("Failed to load default modules: %s\n", tiro_error_str(error));
        goto error_exit;
    }

    if ((error = tiro_context_load(ctx, comp)) != TIRO_OK) {
        printf(
            "Failed to load the compiled module: %s\n", tiro_error_str(error));
        goto error_exit;
    }

    ret = 0;

error_exit:
    tiro_compiler_free(comp);
    tiro_context_free(ctx);
    return ret;
}

/*
 * Performs the compilation steps for the given source code file.
 * Returns false on error.
 */
bool compile_file(tiro_compiler* comp, const char* file_name, bool print_ast,
    bool print_ir, bool disassemble) {
    char* file_content = NULL;
    bool success = false;
    tiro_error error = TIRO_OK;

    file_content = read_source_file(file_name);
    if (!file_content)
        goto end;

    if ((error = tiro_compiler_add_file(comp, "source", file_content))
        != TIRO_OK) {
        printf("Failed to load source: %s.\n", tiro_error_str(error));
        goto end;
    }

    bool compiler_ok = true;
    if ((error = tiro_compiler_run(comp)) != TIRO_OK) {
        compiler_ok = false;
        printf("Failed to compile source: %s.\n", tiro_error_str(error));
    }

    /* We can often dump a partial ast, even if the compilation failed. */
    if (print_ast) {
        char* string = NULL;
        if ((error = tiro_compiler_dump_ast(comp, &string)) != TIRO_OK) {
            printf("Failed to dump ast: %s\n", tiro_error_str(error));
            goto end;
        }

        printf("%s\n", string);
        free(string);
    }

    if (!compiler_ok) {
        printf("Compilation failed, aborting.\n");
        goto end;
    }

    if (print_ir) {
        char* string = NULL;
        if ((error = tiro_compiler_dump_ir(comp, &string)) != TIRO_OK) {
            printf("Failed to dump disassembly: %s\n", tiro_error_str(error));
            goto end;
        }

        printf("%s\n", string);
        free(string);
    }

    if (disassemble) {
        char* string = NULL;
        if ((error = tiro_compiler_dump_bytecode(comp, &string)) != TIRO_OK) {
            printf("Failed to dump disassembly: %s\n", tiro_error_str(error));
            goto end;
        }

        printf("%s\n", string);
        free(string);
    }

    success = true;

end:
    free(file_content);
    return success;
}

/*
 * Reads the complete file into memory, as a null terminated string.
 * Returns NULL on I/O failure.
 */
char* read_source_file(const char* name) {
    FILE* input = NULL;
    char* buffer = NULL;
    char* result = NULL;

    input = fopen(name, "rb");
    if (!input) {
        int err = errno;
        printf("Failed to open input file %s: %s.\n", name, strerror(err));
        goto end;
    }

    size_t buffer_position = 0;
    size_t buffer_capacity = 0;
    while (1) {
        if (buffer_position == buffer_capacity) {
            if (buffer_capacity >= (SIZE_MAX / 2)) {
                printf("File %s is too large.\n", name);
                goto end;
            }

            size_t next_capacity = buffer_capacity ? (buffer_capacity + 1) * 2
                                                   : 4096;

            buffer = realloc(buffer, next_capacity);
            if (!buffer) {
                printf("Failed to allocate buffer.\n");
                goto end;
            }

            /* Always reserve one additional byte for null terminator. */
            buffer_capacity = next_capacity - 1;
        }

        size_t n = fread(buffer + buffer_position, 1,
            buffer_capacity - buffer_position, input);
        buffer_position += n;

        if (feof(input)) {
            break;
        }
        if (ferror(input)) {
            int err = errno;
            printf("Failed to read from %s: %s.\n", name, strerror(err));
            goto end;
        }
    }

    /* Ensure null termination.
     * We always have one byte spare because of the alloc strategy. */
    buffer[buffer_position] = 0;
    result = buffer;
    buffer = NULL;

end:
    free(buffer);
    if (input) {
        fclose(input);
    }
    return result;
}
