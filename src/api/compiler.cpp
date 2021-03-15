#include "api/internal.hpp"

using namespace tiro;
using namespace tiro::api;

static constexpr tiro_compiler_settings_t default_compiler_settings = []() {
    tiro_compiler_settings_t settings{};
    settings.message_callback = [](tiro_severity_t severity, uint32_t line, uint32_t column,
                                    const char* message, void*) {
        try {
            FILE* out = stdout;
            fmt::print(out, "{} [{}:{}]: {}\n", tiro_severity_str(severity), line, column, message);
            std::fflush(out);
        } catch (...) {
        }
    };
    return settings;
}();

static void report(tiro_compiler_t comp, const Diagnostics::Message& message) {
    const auto& settings = comp->settings;
    const auto& compiler = comp->compiler;

    if (!settings.message_callback || !compiler)
        return;

    const tiro_severity_t severity = [&]() {
        switch (message.level) {
        case Diagnostics::Error:
            return TIRO_SEVERITY_ERROR;
        case Diagnostics::Warning:
            return TIRO_SEVERITY_WARNING;
        }

        TIRO_UNREACHABLE("Invalid diagnostic level.");
        return TIRO_SEVERITY_ERROR;
    }();

    CursorPosition pos = compiler->cursor_pos(message.range);
    settings.message_callback(
        severity, pos.line(), pos.column(), message.text.c_str(), settings.message_callback_data);
}

const char* tiro_severity_str(tiro_severity_t severity) {
    switch (severity) {
    case TIRO_SEVERITY_WARNING:
        return "WARNING";
    case TIRO_SEVERITY_ERROR:
        return "ERROR";
    }
    return "<INVALID SEVERITY>";
}

void tiro_compiler_settings_init(tiro_compiler_settings_t* settings) {
    if (!settings)
        return;

    *settings = default_compiler_settings;
}

tiro_compiler_t tiro_compiler_new(const tiro_compiler_settings_t* settings, tiro_error_t* err) {
    return entry_point(err, nullptr, [&] {
        const auto& actual_settings = settings ? *settings : default_compiler_settings;
        return new tiro_compiler(actual_settings);
    });
}

void tiro_compiler_free(tiro_compiler_t compiler) {
    delete compiler;
}

void tiro_compiler_add_file(
    tiro_compiler_t comp, const char* file_name, const char* file_content, tiro_error_t* err) {
    return entry_point(err, [&]() {
        if (!comp || !file_name || !file_content)
            return TIRO_REPORT(err, TIRO_ERROR_BAD_ARG);

        std::string_view file_name_view = file_name;
        std::string_view file_content_view = file_content;
        if (file_name_view.empty())
            return TIRO_REPORT(err, TIRO_ERROR_BAD_ARG);

        if (comp->compiler) // TODO: Only for as long as constructor requires file name and content
            return TIRO_REPORT(err, TIRO_ERROR_BAD_STATE);

        CompilerOptions options;
        options.analyze = options.parse = options.compile = true;
        options.keep_cst = comp->settings.enable_dump_cst;
        options.keep_ast = comp->settings.enable_dump_ast;
        options.keep_ir = comp->settings.enable_dump_ir;
        options.keep_bytecode = comp->settings.enable_dump_bytecode;
        comp->compiler.emplace(
            std::string(file_name_view), std::string(file_content_view), options);
    });
}

void tiro_compiler_run(tiro_compiler_t comp, tiro_error_t* err) {
    return entry_point(err, [&]() {
        if (!comp)
            return TIRO_REPORT(err, TIRO_ERROR_BAD_ARG);

        if (!comp->compiler || comp->result)
            return TIRO_REPORT(err, TIRO_ERROR_BAD_STATE);

        Compiler& compiler = *comp->compiler;

        comp->result = compiler.run();
        for (const auto& message : compiler.diag().messages()) {
            report(comp, message);
        }

        if (!comp->result->success)
            return TIRO_REPORT(err, TIRO_ERROR_BAD_SOURCE);
    });
}

bool tiro_compiler_has_module(tiro_compiler_t comp) {
    return comp && comp->result && comp->result->module;
}

void tiro_compiler_take_module(tiro_compiler_t comp, tiro_module_t* module, tiro_error_t* err) {
    return entry_point(err, [&]() {
        if (!comp || !module)
            return TIRO_REPORT(err, TIRO_ERROR_BAD_ARG);

        if (!tiro_compiler_has_module(comp))
            return TIRO_REPORT(err, TIRO_ERROR_BAD_STATE);

        auto& compiled = comp->result->module;

        auto result = std::make_unique<tiro_module>(std::move(compiled));
        *module = result.release();
    });
}

void tiro_compiler_dump_cst(tiro_compiler_t comp, char** string, tiro_error_t* err) {
    return entry_point(err, [&]() {
        if (!comp || !string)
            return TIRO_REPORT(err, TIRO_ERROR_BAD_ARG);

        if (!comp->result || !comp->result->cst)
            return TIRO_REPORT(err, TIRO_ERROR_BAD_STATE);

        *string = copy_to_cstr(*comp->result->cst);
    });
}

void tiro_compiler_dump_ast(tiro_compiler_t comp, char** string, tiro_error_t* err) {
    return entry_point(err, [&]() {
        if (!comp || !string)
            return TIRO_REPORT(err, TIRO_ERROR_BAD_ARG);

        if (!comp->result || !comp->result->ast)
            return TIRO_REPORT(err, TIRO_ERROR_BAD_STATE);

        *string = copy_to_cstr(*comp->result->ast);
    });
}

void tiro_compiler_dump_ir(tiro_compiler_t comp, char** string, tiro_error_t* err) {
    return entry_point(err, [&]() {
        if (!comp || !string)
            return TIRO_REPORT(err, TIRO_ERROR_BAD_ARG);

        if (!comp->result || !comp->result->ir)
            return TIRO_REPORT(err, TIRO_ERROR_BAD_STATE);

        *string = copy_to_cstr(*comp->result->ir);
    });
}

void tiro_compiler_dump_bytecode(tiro_compiler_t comp, char** string, tiro_error_t* err) {
    return entry_point(err, [&]() {
        if (!comp || !string)
            return TIRO_REPORT(err, TIRO_ERROR_BAD_ARG);

        if (!comp->result || !comp->result->bytecode)
            return TIRO_REPORT(err, TIRO_ERROR_BAD_STATE);

        *string = copy_to_cstr(*comp->result->bytecode);
    });
}

void tiro_module_free(tiro_module_t module) {
    delete module;
}
