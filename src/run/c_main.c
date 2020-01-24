#include "tiro/api.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

const char* source =
    "func f() {\n"
    "  var i = 3;\n"
    "  return i * 2;\n"
    "}\n";

int main(void) {
    tiro_settings settings;
    tiro_settings_init(&settings);

    int ret = 0;
    tiro_error error = TIRO_OK;
    tiro_context* ctx = NULL;
    tiro_compiler* comp = NULL;
    bool ast = true;
    bool disassemble = true;

    ctx = tiro_context_new(&settings);
    if (!ctx) {
        printf("Failed to allocate context.\n");
        goto error_exit;
    }

    comp = tiro_compiler_new(ctx, NULL);
    if (!comp) {
        printf("Failed to allocate diagnostics.\n");
        goto error_exit;
    }

    if ((error = tiro_compiler_add_file(comp, "source", source)) != TIRO_OK) {
        printf("Failed to load source: %s.\n", tiro_error_str(error));
        goto error_exit;
    }

    bool compile_ok = true;
    if ((error = tiro_compiler_run(comp)) != TIRO_OK) {
        printf("Failed to compile source: %s.\n", tiro_error_str(error));
        compile_ok = false;
    }

    if (ast) {
        char* string = NULL;
        if ((error = tiro_compiler_dump_ast(comp, &string)) != TIRO_OK) {
            printf("Failed to dump ast: %s\n", tiro_error_str(error));
            goto error_exit;
        }

        printf("%s\n", string);
        free(string);
    }

    if (disassemble) {
        char* string = NULL;
        if ((error = tiro_compiler_disassemble(comp, &string)) != TIRO_OK) {
            printf("Failed to dump disassembly: %s\n", tiro_error_str(error));
            goto error_exit;
        }

        printf("%s\n", string);
        free(string);
    }

    if (compile_ok) {
        printf("Module was compiled.\n");
    }

error_exit:
    tiro_compiler_free(comp);
    tiro_context_free(ctx);
    return ret;
}
