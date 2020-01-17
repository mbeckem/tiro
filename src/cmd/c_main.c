#include "tiro/api.h"

#include <stddef.h>
#include <stdio.h>

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
    tiro_diagnostics* diag = NULL;

    ctx = tiro_context_new(&settings);
    if (!ctx) {
        printf("Failed to allocate context.\n");
        goto error_exit;
    }

    diag = tiro_diagnostics_new(ctx);
    if (!diag) {
        printf("Failed to allocate diagnostics.\n");
        goto error_exit;
    }

    if ((error = tiro_context_load(ctx, "module", source, diag))
        != TIRO_OK) {
        printf("Failed to load module source: %s.\n", tiro_error_str(error));
        tiro_diagnostics_print_stdout(diag);
        goto error_exit;
    }

    printf("Module was loaded.\n");

error_exit:
    tiro_diagnostics_free(diag);
    tiro_context_free(ctx);
    return ret;
}
