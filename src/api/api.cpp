#include "tiro/api.h"

#include "tiro/compiler/compiler.hpp"
#include "tiro/vm/context.hpp"
#include "tiro/vm/heap/handles.hpp"
#include "tiro/vm/load.hpp"

using namespace tiro;
using namespace tiro::compiler;

struct tiro_context {
    vm::Context vm;
    tiro_settings settings;

    tiro_context(const tiro_settings& settings_);

    tiro_context(const tiro_context&) = delete;
    tiro_context& operator=(const tiro_context&) = delete;

    void report(const char* error) noexcept;
};

struct tiro_diagnostics {
    tiro_context* ctx;
    std::vector<std::pair<CursorPosition, std::string>> messages;

    tiro_diagnostics(tiro_context* ctx_)
        : ctx(ctx_) {
        TIRO_ASSERT_NOT_NULL(ctx);
    }

    tiro_diagnostics(const tiro_diagnostics&) = delete;
    tiro_diagnostics& operator=(const tiro_diagnostics&) = delete;
};

tiro_context::tiro_context(const tiro_settings& settings_)
    : vm()
    , settings(settings_) {}

void tiro_context::report(const char* error) noexcept {
    if (!settings.error_log || !error) {
        return;
    }

    settings.error_log(error, settings.error_log_data);
}

// Eats all exceptions. This is necessary because we're being called by C code.
template<typename ApiFunc>
[[nodiscard]] static tiro_error
api_wrap(tiro_context* ctx, ApiFunc&& fn) noexcept {
    TIRO_ASSERT_NOT_NULL(ctx);

    try {
        if constexpr (std::is_same_v<decltype(fn()), void>) {
            fn();
            return TIRO_OK;
        } else {
            return fn();
        }
    } catch (const std::bad_alloc& e) {
        ctx->report(e.what());
        return TIRO_ERROR_ALLOC;
    } catch (const std::exception& e) {
        // TODO: Meaningful translation from exception to error code
        ctx->report(e.what());
        return TIRO_ERROR_INTERNAL;
    } catch (...) {
        ctx->report("Unknown internal error.");
        return TIRO_ERROR_INTERNAL;
    }
}

const char* tiro_error_str(tiro_error error) {
    switch (error) {
    case TIRO_OK:
        return "OK";
    case TIRO_ERROR_BAD_ARG:
        return "ERROR_BAD_ARG";
    case TIRO_ERROR_BAD_SOURCE:
        return "ERROR_BAD_SOURCE";
    case TIRO_ERROR_MODULE_EXISTS:
        return "ERROR_MODULE_EXISTS";
    case TIRO_ERROR_ALLOC:
        return "ERROR_ALLOC";
    case TIRO_ERROR_INTERNAL:
        return "ERROR_INTERNAL";
    }

    return "UNKOWN ERROR CODE";
}

void tiro_settings_init(tiro_settings* settings) {
    if (!settings) {
        return;
    }

    settings->error_log_data = nullptr;
    settings->error_log = [](const char* message, void*) {
        try {
            FILE* err = stderr;
            fmt::print(err, "{}\n", message);
            std::fflush(err);
        } catch (...) {
        }
    };
}

tiro_context* tiro_context_new(const tiro_settings* settings) {
    static const tiro_settings default_tiro_settings = [&] {
        tiro_settings result;
        tiro_settings_init(&result);
        return result;
    }();

    try {
        return new tiro_context(
            settings ? *settings : default_tiro_settings);
    } catch (...) {
        return nullptr;
    }
}

void tiro_context_free(tiro_context* ctx) {
    delete ctx;
}

tiro_error
tiro_context_load(tiro_context* ctx, const char* module_name_cstr,
    const char* module_source_cstr, tiro_diagnostics* diag) {

    // Note: diag is optional.
    if (!ctx || !module_name_cstr || !module_source_cstr) {
        return TIRO_ERROR_BAD_ARG;
    }

    std::string_view module_name = module_name_cstr;
    if (module_name.empty()) {
        return TIRO_ERROR_BAD_ARG;
    }

    std::string_view module_source = module_source_cstr;

    return api_wrap(ctx, [&] {
        Compiler compiler(module_name, module_source);

        auto compile_module = [&]() -> std::unique_ptr<CompiledModule> {
            if (!compiler.parse()) {
                return nullptr;
            }
            if (!compiler.analyze()) {
                return nullptr;
            }
            if (compiler.diag().has_errors()) {
                return nullptr;
            }
            return compiler.codegen();
        };

        auto module = compile_module();
        if (!module) {
            if (diag) {
                for (const auto& msg : compiler.diag().messages()) {
                    CursorPosition pos = compiler.cursor_pos(msg.source);
                    diag->messages.emplace_back(pos, msg.text); // Meh, copy?
                }
            }
            return TIRO_ERROR_BAD_SOURCE;
        }

        vm::Root module_object(
            ctx->vm, vm::load_module(ctx->vm, *module, compiler.strings()));
        if (!ctx->vm.add_module(module_object)) {
            return TIRO_ERROR_MODULE_EXISTS;
        }
        return TIRO_OK;
    });
}

tiro_diagnostics* tiro_diagnostics_new(tiro_context* ctx) {
    if (!ctx) {
        return nullptr;
    }

    tiro_diagnostics* diag = nullptr;
    tiro_error err = api_wrap(
        ctx, [&] { diag = new tiro_diagnostics(ctx); });
    return err == TIRO_OK ? diag : nullptr;
}

void tiro_diagnostics_free(tiro_diagnostics* diag) {
    delete diag;
}

void tiro_diagnostics_clear(tiro_diagnostics* diag) {
    if (!diag) {
        return;
    }

    (void) api_wrap(diag->ctx, [&] { diag->messages.clear(); });
}

bool tiro_diagnostics_has_messages(tiro_diagnostics* diag) {
    if (!diag) {
        return false;
    }

    bool has_messages = false;
    tiro_error err = api_wrap(
        diag->ctx, [&] { has_messages = diag->messages.size() > 0; });
    return err == TIRO_OK && has_messages;
}

tiro_error tiro_diagnostics_print_stdout(tiro_diagnostics* diag) {
    if (!diag) {
        return TIRO_ERROR_BAD_ARG;
    }

    return api_wrap(diag->ctx, [&] {
        FILE* out = stdout;
        for (const auto& msg : diag->messages) {
            fmt::print(out, "[{}:{}] {}\n", msg.first.line(),
                msg.first.column(), msg.second);
        }
        std::fflush(out);
    });
}
