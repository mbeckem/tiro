#include "hammer/api.h"

#include <stddef.h>
#include <stdio.h>

const char* source =
    "func f() {\n"
    "  var i = 3;\n"
    "  return i * 2;\n"
    "}\n";

int main(void) {
    hammer_settings settings;
    hammer_settings_init(&settings);

    int ret = 0;
    hammer_error error = HAMMER_OK;
    hammer_context* ctx = NULL;
    hammer_diagnostics* diag = NULL;

    ctx = hammer_context_new(&settings);
    if (!ctx) {
        printf("Failed to allocate context.\n");
        goto error_exit;
    }

    diag = hammer_diagnostics_new(ctx);
    if (!diag) {
        printf("Failed to allocate diagnostics.\n");
        goto error_exit;
    }

    if ((error = hammer_context_load(ctx, "module", source, diag))
        != HAMMER_OK) {
        printf("Failed to load module source: %s.\n", hammer_error_str(error));
        hammer_diagnostics_print_stdout(diag);
        goto error_exit;
    }

    printf("Module was loaded.\n");

error_exit:
    hammer_diagnostics_free(diag);
    hammer_context_free(ctx);
    return ret;
}
