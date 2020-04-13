#include "tiro/api.h"

#include "tiro/bytecode/module.hpp"
#include "tiro/compiler/compiler.hpp"
#include "tiro/core/format.hpp"
#include "tiro/core/math.hpp"
#include "tiro/heap/handles.hpp"
#include "tiro/modules/modules.hpp"
#include "tiro/vm/context.hpp"
#include "tiro/vm/load.hpp"

using namespace tiro;

struct tiro_context {
    vm::Context vm;
    tiro_settings settings;

    explicit tiro_context(const tiro_settings& settings_)
        : settings(settings_) {}

    tiro_context(const tiro_context&) = delete;
    tiro_context& operator=(const tiro_context&) = delete;

    void report(const char* error) noexcept;
};

struct tiro_compiler {
    tiro_context* ctx;
    tiro_compiler_settings settings;
    std::optional<Compiler> compiler;
    std::optional<CompilerResult> result;

    explicit tiro_compiler(
        tiro_context* ctx_, const tiro_compiler_settings& settings_)
        : ctx(ctx_)
        , settings(settings_) {
        TIRO_DEBUG_NOT_NULL(ctx);
    }

    tiro_compiler(const tiro_compiler&) = delete;
    tiro_compiler& operator=(const tiro_compiler&) = delete;

    void report(const Diagnostics::Message& message);

    BytecodeModule* get_module();
};

void tiro_context::report(const char* error) noexcept {
    if (!settings.error_log || !error) {
        return;
    }

    settings.error_log(error, settings.error_log_data);
}

void tiro_compiler::report(const Diagnostics::Message& message) {
    if (!settings.message_callback || !compiler)
        return;

    const tiro_severity severity = [&]() {
        switch (message.level) {
        case Diagnostics::Error:
            return TIRO_SEVERITY_ERROR;
        case Diagnostics::Warning:
            return TIRO_SEVERITY_WARNING;
        }

        TIRO_UNREACHABLE("Invalid diagnostic level.");
        return TIRO_SEVERITY_ERROR;
    }();

    CursorPosition pos = compiler->cursor_pos(message.source);
    settings.message_callback(severity, pos.line(), pos.column(),
        message.text.c_str(), settings.message_data);
}

BytecodeModule* tiro_compiler::get_module() {
    if (!result || !result->success || !result->module)
        return nullptr;

    return &*result->module;
}

static constexpr tiro_settings default_settings = []() {
    tiro_settings settings{};
    settings.error_log_data = nullptr;
    settings.error_log = [](const char* message, void*) {
        try {
            FILE* err = stderr;
            fmt::print(err, "{}\n", message);
            std::fflush(err);
        } catch (...) {
        }
    };
    return settings;
}();

static constexpr tiro_compiler_settings default_compiler_settings = []() {
    tiro_compiler_settings settings{};
    settings.message_callback = [](tiro_severity severity, uint32_t line,
                                    uint32_t column, const char* message,
                                    void*) {
        try {
            FILE* out = stdout;
            fmt::print(out, "{} [{}:{}]: {}\n", tiro_severity_str(severity),
                line, column, message);
            std::fflush(out);
        } catch (...) {
        }
    };
    return settings;
}();

static char* to_cstr(const std::string_view str) {
    const size_t string_size = str.size();

    size_t alloc_size = string_size;
    if (!checked_add<size_t>(alloc_size, 1))
        throw std::bad_alloc();

    char* result = static_cast<char*>(::malloc(alloc_size));
    if (!result)
        throw std::bad_alloc();

    std::memcpy(result, str.data(), string_size);
    result[string_size] = '\0';
    return result;
}

// Eats all exceptions. This is necessary because we're being called by C code.
template<typename ApiFunc>
[[nodiscard]] static tiro_error
api_wrap(tiro_context* ctx, ApiFunc&& fn) noexcept {
    TIRO_DEBUG_NOT_NULL(ctx);

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
    case TIRO_ERROR_BAD_STATE:
        return "ERROR_BAD_STATE";
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

const char* tiro_severity_str(tiro_severity severity) {
    switch (severity) {
    case TIRO_SEVERITY_WARNING:
        return "WARNING";
    case TIRO_SEVERITY_ERROR:
        return "ERROR";
    }
    return "UNKNOWN SEVERITY";
}

void tiro_settings_init(tiro_settings* settings) {
    if (!settings) {
        return;
    }

    *settings = default_settings;
}

tiro_context* tiro_context_new(const tiro_settings* settings) {
    try {
        return new tiro_context(settings ? *settings : default_settings);
    } catch (...) {
        return nullptr;
    }
}

void tiro_context_free(tiro_context* ctx) {
    delete ctx;
}

tiro_error tiro_context_load_defaults(tiro_context* ctx) {
    if (!ctx)
        return TIRO_ERROR_BAD_ARG;

    return api_wrap(ctx, [&]() {
        vm::Root<vm::Module> module(ctx->vm);

        module.set(vm::create_std_module(ctx->vm));
        if (!ctx->vm.add_module(module))
            return TIRO_ERROR_MODULE_EXISTS;

        module.set(vm::create_io_module(ctx->vm));
        if (!ctx->vm.add_module(module))
            return TIRO_ERROR_MODULE_EXISTS;

        return TIRO_OK;
    });
}

tiro_error tiro_context_load(tiro_context* ctx, tiro_compiler* comp) {
    if (!ctx || !comp || ctx != comp->ctx)
        return TIRO_ERROR_BAD_ARG;

    BytecodeModule* compiled_module = comp->get_module();
    if (!compiled_module)
        return TIRO_ERROR_BAD_ARG;

    return api_wrap(ctx, [&]() {
        vm::Root<vm::Module> module(
            ctx->vm, vm::load_module(
                         ctx->vm, *compiled_module, comp->compiler->strings()));
        if (!ctx->vm.add_module(module))
            return TIRO_ERROR_MODULE_EXISTS;

        return TIRO_OK;
    });
}

void tiro_compiler_settings_init(tiro_compiler_settings* settings) {
    if (!settings)
        return;

    *settings = default_compiler_settings;
}

tiro_compiler*
tiro_compiler_new(tiro_context* ctx, const tiro_compiler_settings* settings) {
    try {
        return new tiro_compiler(
            ctx, settings ? *settings : default_compiler_settings);
    } catch (...) {
        return nullptr;
    }
}

void tiro_compiler_free(tiro_compiler* compiler) {
    delete compiler;
}

tiro_error tiro_compiler_add_file(
    tiro_compiler* comp, const char* file_name, const char* file_content) {
    if (!comp || !file_name || !file_content)
        return TIRO_ERROR_BAD_ARG;

    std::string_view file_name_view = file_name;
    std::string_view file_content_view = file_content;
    if (file_name_view.empty() || file_content_view.empty())
        return TIRO_ERROR_BAD_ARG;

    if (comp->compiler) // Only for as long as constructor requires file name and contnet
        return TIRO_ERROR_BAD_STATE;

    return api_wrap(comp->ctx,
        [&]() { comp->compiler.emplace(file_name_view, file_content_view); });
}

tiro_error tiro_compiler_run(tiro_compiler* comp) {
    if (!comp)
        return TIRO_ERROR_BAD_ARG;

    if (!comp->compiler || comp->result)
        return TIRO_ERROR_BAD_STATE;

    return api_wrap(comp->ctx, [&]() {
        Compiler& compiler = *comp->compiler;

        comp->result = compiler.run();
        for (const auto& message : compiler.diag().messages()) {
            comp->report(message);
        }

        if (!comp->result->success)
            return TIRO_ERROR_BAD_SOURCE;

        return TIRO_OK;
    });
}

bool tiro_compiler_has_module(tiro_compiler* comp) {
    return comp->get_module() != nullptr;
}

tiro_error tiro_compiler_dump_ast(tiro_compiler* comp, char** string) {
    if (!comp || !string)
        return TIRO_ERROR_BAD_ARG;

    return api_wrap(comp->ctx, [&]() {
        if (!comp->result || !comp->result->ast)
            return TIRO_ERROR_BAD_STATE;

        *string = to_cstr(*comp->result->ast);
        return TIRO_OK;
    });
}

tiro_error tiro_compiler_dump_ir(tiro_compiler* comp, char** string) {
    if (!comp || !string)
        return TIRO_ERROR_BAD_ARG;

    return api_wrap(comp->ctx, [&]() {
        if (!comp->result || !comp->result->ir)
            return TIRO_ERROR_BAD_STATE;

        *string = to_cstr(*comp->result->ir);
        return TIRO_OK;
    });
}

tiro_error tiro_compiler_dump_bytecode(tiro_compiler* comp, char** string) {
    if (!comp || !string)
        return TIRO_ERROR_BAD_ARG;

    return api_wrap(comp->ctx, [&]() {
        if (!comp->result || !comp->result->bytecode)
            return TIRO_ERROR_BAD_STATE;

        *string = to_cstr(*comp->result->bytecode);
        return TIRO_OK;
    });
}
