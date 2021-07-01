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
    tiro_error_t error = NULL;
    tiro_vm_t vm = NULL;
    tiro_module_t module = NULL;
    tiro_compiler_t comp = NULL;
    tiro_handle_t func_handle = NULL;
    tiro_handle_t result_handle = NULL;
    tiro_handle_t value_handle = NULL;
    tiro_handle_t string_handle = NULL;
    char* result = NULL;
    bool print_ast = true;
    bool print_ir = true;
    bool disassemble = true;
    bool panic = false;

    vm = tiro_vm_new(&settings, &error);
    if (error) {
        printf("Failed to allocate context: %s.\n", tiro_error_message(error));
        goto error_exit;
    }

    tiro_compiler_settings_t comp_settings;
    tiro_compiler_settings_init(&comp_settings);
    comp_settings.enable_dump_ast = print_ast;
    comp_settings.enable_dump_ir = print_ir;
    comp_settings.enable_dump_bytecode = disassemble;

    comp = tiro_compiler_new(&comp_settings, &error);
    if (!comp) {
        printf("Failed to allocate compiler: %s.\n", tiro_error_message(error));
        goto error_exit;
    }

    if (!compile_file(comp, file_name, print_ast, print_ir, disassemble, &module)) {
        goto error_exit;
    }

    tiro_vm_load_std(vm, &error);
    if (error) {
        printf("Failed to load default modules: %s\n", tiro_error_message(error));
        goto error_exit;
    }

    tiro_vm_load_bytecode(vm, module, &error);
    if (error) {
        printf("Failed to load the compiled module: %s\n", tiro_error_message(error));
        goto error_exit;
    }

    func_handle = tiro_global_new(vm, &error);
    result_handle = tiro_global_new(vm, &error);
    value_handle = tiro_global_new(vm, &error);
    string_handle = tiro_global_new(vm, &error);
    if (error) {
        printf("Failed to allocate handles for values: %s\n", tiro_error_message(error));
        goto error_exit;
    }

    tiro_vm_get_export(vm, "main", func_name, func_handle, &error);
    if (error) {
        printf(
            "Failed to find the function called '%s': %s\n", func_name, tiro_error_message(error));
        goto error_exit;
    }

    tiro_vm_call(vm, func_handle, NULL, result_handle, &error);
    if (error) {
        printf("Failed to execute function: %s\n", tiro_error_message(error));
        goto error_exit;
    }

    if (tiro_value_kind(vm, result_handle) != TIRO_KIND_RESULT) {
        printf("Unexpected function return type, expected result: %s\n",
            tiro_kind_str(tiro_value_kind(vm, result_handle)));
        goto error_exit;
    }

    if (tiro_result_is_success(vm, result_handle)) {
        panic = false;
        tiro_result_value(vm, result_handle, value_handle, &error);
        if (error) {
            printf("Failed to retrieve return value: %s\n", tiro_error_message(error));
            goto error_exit;
        }
    } else {
        panic = true;
        tiro_result_reason(vm, result_handle, value_handle, &error);
        if (error) {
            printf("Failed to retrieve panic value: %s\n", tiro_error_message(error));
            goto error_exit;
        }
    }

    tiro_value_to_string(vm, value_handle, string_handle, &error);
    if (error) {
        printf("Failed to convert function result to a string: %s\n", tiro_error_message(error));
        goto error_exit;
    }

    tiro_string_cstr(vm, string_handle, &result, &error);
    if (error) {
        printf(
            "Failed to construct a c string from a tiro string: %s\n", tiro_error_message(error));
        goto error_exit;
    }

    printf("%s value: %s\n", panic ? "Panic" : "Return", result);

    ret = 0;

error_exit:
    free(result);
    tiro_global_free(vm, func_handle);
    tiro_global_free(vm, result_handle);
    tiro_global_free(vm, value_handle);
    tiro_global_free(vm, string_handle);
    tiro_compiler_free(comp);
    tiro_module_free(module);
    tiro_vm_free(vm);
    tiro_error_free(error);
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
    tiro_error_t error = NULL;

    file_content = read_source_file(file_name);
    if (!file_content)
        goto end;

    tiro_compiler_add_file(comp, "main", file_content, &error);
    if (error) {
        printf("Failed to load source: %s.\n", tiro_error_message(error));
        goto end;
    }

    bool compiler_ok = true;
    tiro_compiler_run(comp, &error);
    if (error) {
        compiler_ok = false;
        printf("Failed to compile source: %s.\n", tiro_error_message(error));
    }

    /* We can often dump a partial ast, even if the compilation failed. */
    if (print_ast) {
        char* string = NULL;
        tiro_compiler_dump_ast(comp, &string, &error);
        if (error) {
            printf("Failed to dump ast: %s\n", tiro_error_message(error));
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
        tiro_compiler_dump_ir(comp, &string, &error);
        if (error) {
            printf("Failed to dump disassembly: %s\n", tiro_error_message(error));
            goto end;
        }

        printf("%s\n", string);
        free(string);
    }

    if (disassemble) {
        char* string = NULL;
        tiro_compiler_dump_bytecode(comp, &string, &error);
        if (error) {
            printf("Failed to dump disassembly: %s\n", tiro_error_message(error));
            goto end;
        }

        printf("%s\n", string);
        free(string);
    }

    tiro_compiler_take_module(comp, module, &error);
    if (error) {
        printf("Failed to retrieve module: %s\n", tiro_error_message(error));
        goto end;
    }

    success = true;

end:
    free(file_content);
    tiro_error_free(error);
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
