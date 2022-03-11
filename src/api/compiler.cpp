#include "api/internal.hpp"

#include "compiler/syntax/dump.hpp"
#include "compiler/syntax/syntax_tree.hpp"

using namespace tiro;
using namespace tiro::api;

bool tiro_compiler::default_message_callback(const tiro_compiler_message_t* msg, void*) {
    try {
        FILE* out = stdout;

        std::string_view filename = to_internal(msg->file);
        if (filename.empty())
            filename = "<N/A>";

        std::string_view text = to_internal(msg->text);

        fmt::print(out, "{} {}:{}:{}: {}\n", tiro_severity_str(msg->severity), filename, msg->line,
            msg->column, text);
        std::fflush(out);
    } catch (...) {
    }
    return true;
}

static bool
report(tiro_compiler_t comp, const Compiler& compiler, const Diagnostics::Message& message) {
    if (!comp || !comp->message_callback)
        return true;

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

    TIRO_DEBUG_ASSERT(comp->message_callback, "invalid message callback");
    return comp->message_callback(&msg, comp->message_userdata);
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

tiro_compiler_t tiro_compiler_new(tiro_string_t module_name, tiro_error_t* err) {
    return entry_point(err, nullptr, [&]() -> tiro_compiler_t {
        if (!valid_string(module_name) || module_name.length == 0)
            return TIRO_REPORT(err, TIRO_ERROR_BAD_ARG), nullptr;
        return new tiro_compiler(to_internal(module_name));
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

        if (comp->started)
            return TIRO_REPORT(err, TIRO_ERROR_BAD_STATE);

        std::string_view file_name_view = to_internal(file_name);
        std::string_view file_content_view = to_internal(file_content);
        if (file_name_view.empty())
            return TIRO_REPORT(err, TIRO_ERROR_BAD_ARG);

        comp->files.push_back({std::string(file_name_view), std::string(file_content_view)});
    });
}

void tiro_compiler_set_message_callback(tiro_compiler_t comp,
    tiro_message_callback_t message_callback, void* userdata, tiro_error_t* err) {
    return entry_point(err, [&]() {
        if (!comp)
            return TIRO_REPORT(err, TIRO_ERROR_BAD_ARG);

        if (!message_callback) {
            comp->message_callback = tiro_compiler::default_message_callback;
            comp->message_userdata = nullptr;
            return;
        }

        comp->message_callback = message_callback;
        comp->message_userdata = userdata;
    });
}

void tiro_compiler_request_attachment(
    tiro_compiler_t comp, tiro_attachment_t attachment, tiro_error_t* err) {
    return entry_point(err, [&]() {
        if (!comp)
            return TIRO_REPORT(err, TIRO_ERROR_BAD_ARG);

        if (comp->started || comp->result)
            return TIRO_REPORT(err, TIRO_ERROR_BAD_STATE);

        switch (attachment) {
        case TIRO_ATTACHMENT_CST:
            comp->dump_cst = true;
            break;
        case TIRO_ATTACHMENT_AST:
            comp->dump_ast = true;
            break;
        case TIRO_ATTACHMENT_IR:
            comp->dump_ir = true;
            break;
        case TIRO_ATTACHMENT_BYTECODE:
            comp->dump_bytecode = true;
            break;
        default:
            return TIRO_REPORT(err, TIRO_ERROR_BAD_ARG);
        }
    });
}

void tiro_compiler_run(tiro_compiler_t comp, tiro_error_t* err) {
    return entry_point(err, [&]() {
        if (!comp)
            return TIRO_REPORT(err, TIRO_ERROR_BAD_ARG);

        if (comp->started || comp->result)
            return TIRO_REPORT(err, TIRO_ERROR_BAD_STATE);

        comp->started = true;

        CompilerOptions options;
        options.analyze = options.parse = options.compile = true;
        options.keep_cst = comp->dump_cst;
        options.keep_ast = comp->dump_ast;
        options.keep_ir = comp->dump_ir;
        options.keep_bytecode = comp->dump_bytecode;

        Compiler compiler(comp->module_name, options);
        for (auto& file : comp->files) {
            compiler.add_file(std::move(file.name), std::move(file.content));
        }
        comp->files.clear();

        comp->result = compiler.run();
        for (const auto& message : compiler.diag().messages()) {
            if (!report(comp, compiler, message)) {
                return TIRO_REPORT(
                    err, TIRO_ERROR_BAD_STATE, [&] { return "fatal error in message callback"; });
            }
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

void tiro_compiler_get_attachment(
    tiro_compiler_t comp, tiro_attachment_t attachment, char** string, tiro_error_t* err) {
    return entry_point(err, [&]() {
        if (!comp || !string)
            return TIRO_REPORT(err, TIRO_ERROR_BAD_ARG);

        auto& result = comp->result;
        if (!result)
            return TIRO_REPORT(err, TIRO_ERROR_BAD_STATE);

        switch (attachment) {
        case TIRO_ATTACHMENT_CST:
            if (!result->cst)
                return TIRO_REPORT(err, TIRO_ERROR_BAD_STATE);
            *string = copy_to_cstr(*result->cst);
            break;
        case TIRO_ATTACHMENT_AST:
            if (!result->ast)
                return TIRO_REPORT(err, TIRO_ERROR_BAD_STATE);
            *string = copy_to_cstr(*result->ast);
            break;
        case TIRO_ATTACHMENT_IR:
            if (!result->ir)
                return TIRO_REPORT(err, TIRO_ERROR_BAD_STATE);
            *string = copy_to_cstr(*result->ir);
            break;
        case TIRO_ATTACHMENT_BYTECODE:
            if (!result->bytecode)
                return TIRO_REPORT(err, TIRO_ERROR_BAD_STATE);
            *string = copy_to_cstr(*result->bytecode);
            break;
        default:
            return TIRO_REPORT(err, TIRO_ERROR_BAD_ARG);
        }
    });
}

void tiro_module_free(tiro_module_t module) {
    delete module;
}
