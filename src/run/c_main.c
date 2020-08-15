#include "tiro/api.h"

#include <errno.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static bool compile_file(tiro_compiler_t comp, const char* file_name, bool print_ast, bool print_ir,
    bool diassemble, tiro_module_t* module);
static char* read_source_file(const char* name);

int main(int argc, char** argv) {
    if (argc < 3) {
        printf("Usage: %s FILENAME FUNCTION_NAME\n", argv[0]);
        return 1;
    }

    const char* file_name = argv[1];
    const char* func_name = argv[2];

    tiro_vm_settings_t settings;
    tiro_vm_settings_init(&settings);

    int ret = 1;
    tiro_errc_t error = TIRO_OK;
    tiro_vm_t vm = NULL;
    tiro_module_t module = NULL;
    tiro_compiler_t comp = NULL;
    tiro_frame_t frame = NULL;
    char* result = NULL;
    bool print_ast = true;
    bool print_ir = true;
    bool disassemble = true;

    vm = tiro_vm_new(&settings);
    if (!vm) {
        printf("Failed to allocate context.\n");
        goto error_exit;
    }

    tiro_compiler_settings_t comp_settings;
    tiro_compiler_settings_init(&comp_settings);
    comp_settings.enable_dump_ast = print_ast;
    comp_settings.enable_dump_ir = print_ir;
    comp_settings.enable_dump_bytecode = disassemble;

    comp = tiro_compiler_new(&comp_settings);
    if (!comp) {
        printf("Failed to allocate diagnostics.\n");
        goto error_exit;
    }

    if (!compile_file(comp, file_name, print_ast, print_ir, disassemble, &module)) {
        goto error_exit;
    }

    if ((error = tiro_vm_load_std(vm, NULL)) != TIRO_OK) {
        printf("Failed to load default modules: %s\n", tiro_errc_name(error));
        goto error_exit;
    }

    if ((error = tiro_vm_load(vm, module, NULL)) != TIRO_OK) {
        printf("Failed to load the compiled module: %s\n", tiro_errc_name(error));
        goto error_exit;
    }

    frame = tiro_frame_new(vm, 3);
    if (!frame) {
        printf("Failed to allocate handle frame.\n");
        goto error_exit;
    }

    tiro_handle_t func_handle = tiro_frame_slot(frame, 0);
    tiro_handle_t result_handle = tiro_frame_slot(frame, 1);
    tiro_handle_t string_handle = tiro_frame_slot(frame, 2);

    if ((error = tiro_vm_find_function(vm, "main", func_name, func_handle, NULL)) != TIRO_OK) {
        printf("Failed to find the function called '%s': %s\n", func_name, tiro_errc_name(error));
        goto error_exit;
    }

    if ((error = tiro_vm_call(vm, func_handle, NULL, result_handle, NULL)) != TIRO_OK) {
        printf("Failed to execute function: %s\n", tiro_errc_name(error));
        goto error_exit;
    }

    if ((error = tiro_value_to_string(vm, result_handle, string_handle, NULL)) != TIRO_OK) {
        printf("Failed to convert function result to a string: %s\n", tiro_errc_name(error));
        goto error_exit;
    }

    if ((error = tiro_string_cstr(vm, string_handle, &result, NULL)) != TIRO_OK) {
        printf("Failed to construct a c string from a tiro string: %s\n", tiro_errc_name(error));
        goto error_exit;
    }

    printf("Return value: %s\n", result);

    ret = 0;

error_exit:
    free(result);
    tiro_frame_free(frame);
    tiro_compiler_free(comp);
    tiro_module_free(module);
    tiro_vm_free(vm);
    return ret;
}

/*
 * Performs the compilation steps for the given source code file.
 * Returns false on error.
 */
bool compile_file(tiro_compiler_t comp, const char* file_name, bool print_ast, bool print_ir,
    bool disassemble, tiro_module_t* module) {
    char* file_content = NULL;
    bool success = false;
    tiro_errc_t error = TIRO_OK;

    file_content = read_source_file(file_name);
    if (!file_content)
        goto end;

    if ((error = tiro_compiler_add_file(comp, "main", file_content, NULL)) != TIRO_OK) {
        printf("Failed to load source: %s.\n", tiro_errc_name(error));
        goto end;
    }

    bool compiler_ok = true;
    if ((error = tiro_compiler_run(comp, NULL)) != TIRO_OK) {
        compiler_ok = false;
        printf("Failed to compile source: %s.\n", tiro_errc_name(error));
    }

    /* We can often dump a partial ast, even if the compilation failed. */
    if (print_ast) {
        char* string = NULL;
        if ((error = tiro_compiler_dump_ast(comp, &string, NULL)) != TIRO_OK) {
            printf("Failed to dump ast: %s\n", tiro_errc_name(error));
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
        if ((error = tiro_compiler_dump_ir(comp, &string, NULL)) != TIRO_OK) {
            printf("Failed to dump disassembly: %s\n", tiro_errc_name(error));
            goto end;
        }

        printf("%s\n", string);
        free(string);
    }

    if (disassemble) {
        char* string = NULL;
        if ((error = tiro_compiler_dump_bytecode(comp, &string, NULL)) != TIRO_OK) {
            printf("Failed to dump disassembly: %s\n", tiro_errc_name(error));
            goto end;
        }

        printf("%s\n", string);
        free(string);
    }

    if ((error = tiro_compiler_take_module(comp, module, NULL)) != TIRO_OK) {
        printf("Failed to retrieve module: %s\n", tiro_errc_name(error));
        goto end;
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

            size_t next_capacity = buffer_capacity ? (buffer_capacity + 1) * 2 : 4096;

            buffer = realloc(buffer, next_capacity);
            if (!buffer) {
                printf("Failed to allocate buffer.\n");
                goto end;
            }

            /* Always reserve one additional byte for null terminator. */
            buffer_capacity = next_capacity - 1;
        }

        size_t n = fread(buffer + buffer_position, 1, buffer_capacity - buffer_position, input);
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
