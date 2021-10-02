#include "api/internal.hpp"

#include "compiler/syntax/dump.hpp"
#include "compiler/syntax/syntax_tree.hpp"

using namespace tiro;
using namespace tiro::api;

static constexpr tiro_compiler_settings_t default_compiler_settings = []() {
    tiro_compiler_settings_t settings{};
    settings.message_callback = [](const tiro_compiler_message_t* msg, void*) {
        try {
            FILE* out = stdout;

            std::string_view filename = to_internal(msg->file);
            if (filename.empty())
                filename = "<N/A>";

            std::string_view text = to_internal(msg->text);

            fmt::print(out, "{} {}:{}:{}: {}\n", tiro_severity_str(msg->severity), filename,
                msg->line, msg->column, text);
            std::fflush(out);
        } catch (...) {
        }
    };
    return settings;
}();

static void report(tiro_compiler_t comp, const Diagnostics::Message& message) {
    if (!comp || !comp->message_callback)
        return;

    auto get_severity = [](Diagnostics::Level level) {
        switch (level) {
        case Diagnostics::Error:
            return TIRO_SEVERITY_ERROR;
        case Diagnostics::Warning:
            return TIRO_SEVERITY_WARNING;
        }

        TIRO_UNREACHABLE("Invalid diagnostic level.");
        return TIRO_SEVERITY_ERROR;
    };

    auto& compiler = comp->compiler;

    tiro_compiler_message_t msg{};
    msg.severity = get_severity(message.level);
    if (message.range) {
        auto pos = compiler.cursor_pos(message.range);
        msg.file = to_external(compiler.sources().filename(message.range.id()));
        msg.line = pos.line();
        msg.column = pos.column();
    } else {
        msg.file = to_external("");
        msg.line = 0;
        msg.column = 0;
    }
    msg.text = to_external(message.text);
    comp->message_callback(msg);
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

tiro_compiler_t tiro_compiler_new(
    tiro_string_t module_name, const tiro_compiler_settings_t* settings, tiro_error_t* err) {
    return entry_point(err, nullptr, [&]() -> tiro_compiler_t {
        if (!valid_string(module_name) || module_name.length == 0)
            return TIRO_REPORT(err, TIRO_ERROR_BAD_ARG), nullptr;

        const auto& actual_settings = settings ? *settings : default_compiler_settings;
        return new tiro_compiler(to_internal(module_name), actual_settings);
    });
}

void tiro_compiler_free(tiro_compiler_t compiler) {
    delete compiler;
}

void tiro_compiler_add_file(
    tiro_compiler_t comp, tiro_string_t file_name, tiro_string_t file_content, tiro_error_t* err) {
    return entry_point(err, [&]() {
        if (!comp || !valid_string(file_name) || !valid_string(file_content))
            return TIRO_REPORT(err, TIRO_ERROR_BAD_ARG);

        if (comp->compiler.started())
            return TIRO_REPORT(err, TIRO_ERROR_BAD_STATE);

        std::string_view file_name_view = to_internal(file_name);
        std::string_view file_content_view = to_internal(file_content);
        if (file_name_view.empty())
            return TIRO_REPORT(err, TIRO_ERROR_BAD_ARG);

        comp->compiler.add_file(std::string(file_name_view), std::string(file_content_view));
    });
}

void tiro_compiler_run(tiro_compiler_t comp, tiro_error_t* err) {
    return entry_point(err, [&]() {
        if (!comp)
            return TIRO_REPORT(err, TIRO_ERROR_BAD_ARG);

        if (comp->compiler.started() || comp->result)
            return TIRO_REPORT(err, TIRO_ERROR_BAD_STATE);

        Compiler& compiler = comp->compiler;

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
