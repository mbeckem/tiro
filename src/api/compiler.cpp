#include "api/private.hpp"

using namespace tiro;
using namespace tiro::api;

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

static void report(tiro_compiler* comp, const Diagnostics::Message& message) {
    const auto& settings = comp->settings;
    const auto& compiler = comp->compiler;

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
        message.text.c_str(), settings.message_callback_data);
}

const char* tiro_severity_str(tiro_severity severity) {
    switch (severity) {
    case TIRO_SEVERITY_WARNING:
        return "WARNING";
    case TIRO_SEVERITY_ERROR:
        return "ERROR";
    }
    return "<INVALID SEVERITY>";
}

void tiro_compiler_settings_init(tiro_compiler_settings* settings) {
    if (!settings)
        return;

    *settings = default_compiler_settings;
}

tiro_compiler* tiro_compiler_new(const tiro_compiler_settings* settings) {
    try {
        return new tiro_compiler(
            settings ? *settings : default_compiler_settings);
    } catch (...) {
        return nullptr;
    }
}

void tiro_compiler_free(tiro_compiler* compiler) {
    delete compiler;
}

tiro_errc tiro_compiler_add_file(tiro_compiler* comp, const char* file_name,
    const char* file_content, tiro_error** err) {
    if (!comp || !file_name || !file_content)
        return TIRO_REPORT(err, TIRO_ERROR_BAD_ARG);

    std::string_view file_name_view = file_name;
    std::string_view file_content_view = file_content;
    if (file_name_view.empty())
        return TIRO_REPORT(err, TIRO_ERROR_BAD_ARG);

    if (comp->compiler) // TODO: Only for as long as constructor requires file name and content
        return TIRO_REPORT(err, TIRO_ERROR_BAD_STATE);

    return api_wrap(err, [&]() {
        CompilerOptions options;
        options.analyze = options.parse = options.compile = true;
        options.keep_ast = comp->settings.enable_dump_ast;
        options.keep_ir = comp->settings.enable_dump_ir;
        options.keep_bytecode = comp->settings.enable_dump_bytecode;
        comp->compiler.emplace(file_name_view, file_content_view, options);
    });
}

tiro_errc tiro_compiler_run(tiro_compiler* comp, tiro_error** err) {
    if (!comp)
        return TIRO_REPORT(err, TIRO_ERROR_BAD_ARG);

    if (!comp->compiler || comp->result)
        return TIRO_REPORT(err, TIRO_ERROR_BAD_STATE);

    return api_wrap(err, [&]() {
        Compiler& compiler = *comp->compiler;

        comp->result = compiler.run();
        for (const auto& message : compiler.diag().messages()) {
            report(comp, message);
        }

        if (!comp->result->success)
            return TIRO_REPORT(err, TIRO_ERROR_BAD_SOURCE);

        return TIRO_OK;
    });
}

bool tiro_compiler_has_module(tiro_compiler* comp) {
    return comp && comp->result && comp->result->module;
}

tiro_errc tiro_compiler_take_module(
    tiro_compiler* comp, tiro_module** module, tiro_error** err) {
    if (!comp || !module)
        return TIRO_REPORT(err, TIRO_ERROR_BAD_ARG);

    if (!tiro_compiler_has_module(comp))
        return TIRO_REPORT(err, TIRO_ERROR_BAD_STATE);

    return api_wrap(err, [&]() {
        auto& compiled = comp->result->module;

        auto result = std::make_unique<tiro_module>(std::move(compiled));
        *module = result.release();
        return TIRO_OK;
    });
}

tiro_errc
tiro_compiler_dump_ast(tiro_compiler* comp, char** string, tiro_error** err) {
    if (!comp || !string)
        return TIRO_REPORT(err, TIRO_ERROR_BAD_ARG);

    return api_wrap(err, [&]() {
        if (!comp->result || !comp->result->ast)
            return TIRO_REPORT(err, TIRO_ERROR_BAD_STATE);

        *string = copy_to_cstr(*comp->result->ast);
        return TIRO_OK;
    });
}

tiro_errc
tiro_compiler_dump_ir(tiro_compiler* comp, char** string, tiro_error** err) {
    if (!comp || !string)
        return TIRO_REPORT(err, TIRO_ERROR_BAD_ARG);

    return api_wrap(err, [&]() {
        if (!comp->result || !comp->result->ir)
            return TIRO_REPORT(err, TIRO_ERROR_BAD_STATE);

        *string = copy_to_cstr(*comp->result->ir);
        return TIRO_OK;
    });
}

tiro_errc tiro_compiler_dump_bytecode(
    tiro_compiler* comp, char** string, tiro_error** err) {
    if (!comp || !string)
        return TIRO_REPORT(err, TIRO_ERROR_BAD_ARG);

    return api_wrap(err, [&]() {
        if (!comp->result || !comp->result->bytecode)
            return TIRO_REPORT(err, TIRO_ERROR_BAD_STATE);

        *string = copy_to_cstr(*comp->result->bytecode);
        return TIRO_OK;
    });
}

void tiro_module_free(tiro_module* module) {
    delete module;
}
