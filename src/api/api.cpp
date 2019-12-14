#include "hammer/api.h"

#include "hammer/compiler/compiler.hpp"
#include "hammer/vm/context.hpp"
#include "hammer/vm/heap/handles.hpp"
#include "hammer/vm/load.hpp"

#include <iostream>

using namespace hammer;

struct hammer_context {
    vm::Context vm;
    hammer_settings settings;

    hammer_context(const hammer_settings& settings_);

    hammer_context(const hammer_context&) = delete;
    hammer_context& operator=(const hammer_context&) = delete;

    void report(const char* error) noexcept;
};

struct hammer_diagnostics {
    hammer_context* ctx;
    std::vector<std::pair<CursorPosition, std::string>> messages;

    hammer_diagnostics(hammer_context* ctx_)
        : ctx(ctx_) {
        HAMMER_ASSERT_NOT_NULL(ctx);
    }

    hammer_diagnostics(const hammer_diagnostics&) = delete;
    hammer_diagnostics& operator=(const hammer_diagnostics&) = delete;
};

hammer_context::hammer_context(const hammer_settings& settings_)
    : vm()
    , settings(settings_) {}

void hammer_context::report(const char* error) noexcept {
    if (!settings.error_log || !error) {
        return;
    }

    settings.error_log(error, settings.error_log_data);
}

// Eats all exceptions. This is necessary because we're being called by C code.
template<typename ApiFunc>
[[nodiscard]] static hammer_error
api_wrap(hammer_context* ctx, ApiFunc&& fn) noexcept {
    HAMMER_ASSERT_NOT_NULL(ctx);

    try {
        if constexpr (std::is_same_v<decltype(fn()), void>) {
            fn();
            return HAMMER_OK;
        } else {
            return fn();
        }
    } catch (const std::bad_alloc& e) {
        ctx->report(e.what());
        return HAMMER_ERROR_ALLOC;
    } catch (const std::exception& e) {
        // TODO: Meaningful translation from exception to error code
        ctx->report(e.what());
        return HAMMER_ERROR_INTERNAL;
    } catch (...) {
        ctx->report("Unknown internal error.");
        return HAMMER_ERROR_INTERNAL;
    }
}

const char* hammer_error_str(hammer_error error) {
    switch (error) {
    case HAMMER_OK:
        return "OK";
    case HAMMER_ERROR_BAD_ARG:
        return "ERROR_BAD_ARG";
    case HAMMER_ERROR_BAD_SOURCE:
        return "ERROR_BAD_SOURCE";
    case HAMMER_ERROR_MODULE_EXISTS:
        return "ERROR_MODULE_EXISTS";
    case HAMMER_ERROR_ALLOC:
        return "ERROR_ALLOC";
    case HAMMER_ERROR_INTERNAL:
        return "ERROR_INTERNAL";
    }

    return "UNKOWN ERROR CODE";
}

void hammer_settings_init(hammer_settings* settings) {
    if (!settings) {
        return;
    }

    settings->error_log_data = nullptr;
    settings->error_log = [](const char* message, void*) {
        try {
            std::cout << "ERROR: " << message << "\n";
        } catch (...) {
        }
    };
}

hammer_context* hammer_context_new(const hammer_settings* settings) {
    static const hammer_settings default_hammer_settings = [&] {
        hammer_settings result;
        hammer_settings_init(&result);
        return result;
    }();

    try {
        return new hammer_context(
            settings ? *settings : default_hammer_settings);
    } catch (...) {
        return nullptr;
    }
}

void hammer_context_free(hammer_context* ctx) {
    delete ctx;
}

hammer_error
hammer_context_load(hammer_context* ctx, const char* module_name_cstr,
    const char* module_source_cstr, hammer_diagnostics* diag) {

    // Note: diag is optional.
    if (!ctx || !module_name_cstr || !module_source_cstr) {
        return HAMMER_ERROR_BAD_ARG;
    }

    std::string_view module_name = module_name_cstr;
    if (module_name.empty()) {
        return HAMMER_ERROR_BAD_ARG;
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
            return HAMMER_ERROR_BAD_SOURCE;
        }

        vm::Root module_object(
            ctx->vm, vm::load_module(ctx->vm, *module, compiler.strings()));
        if (!ctx->vm.add_module(module_object)) {
            return HAMMER_ERROR_MODULE_EXISTS;
        }
        return HAMMER_OK;
    });
}

hammer_diagnostics* hammer_diagnostics_new(hammer_context* ctx) {
    if (!ctx) {
        return nullptr;
    }

    hammer_diagnostics* diag = nullptr;
    hammer_error err = api_wrap(
        ctx, [&] { diag = new hammer_diagnostics(ctx); });
    return err == HAMMER_OK ? diag : nullptr;
}

void hammer_diagnostics_free(hammer_diagnostics* diag) {
    delete diag;
}

void hammer_diagnostics_clear(hammer_diagnostics* diag) {
    if (!diag) {
        return;
    }

    (void) api_wrap(diag->ctx, [&] { diag->messages.clear(); });
}

bool hammer_diagnostics_has_messages(hammer_diagnostics* diag) {
    if (!diag) {
        return false;
    }

    bool has_messages = false;
    hammer_error err = api_wrap(
        diag->ctx, [&] { has_messages = diag->messages.size() > 0; });
    return err == HAMMER_OK && has_messages;
}

hammer_error hammer_diagnostics_print_stdout(hammer_diagnostics* diag) {
    if (!diag) {
        return HAMMER_ERROR_BAD_ARG;
    }

    return api_wrap(diag->ctx, [&] {
        for (const auto& msg : diag->messages) {
            std::cout << "[" << msg.first.line() << ":" << msg.first.column()
                      << "] " << msg.second << "\n";
        }
        std::cout << std::flush;
    });
}
