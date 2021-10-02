#ifndef TIROPP_COMPILER_HPP_INCLUDED
#define TIROPP_COMPILER_HPP_INCLUDED

#include "tiropp/def.hpp"
#include "tiropp/detail/resource_holder.hpp"
#include "tiropp/error.hpp"
#include "tiropp/fwd.hpp"

#include "tiro/compiler.h"

#include <functional>
#include <memory>

namespace tiro {

enum class severity : int {
    warning = TIRO_SEVERITY_WARNING,
    error = TIRO_SEVERITY_ERROR,
};

inline const char* to_string(severity s) {
    return tiro_severity_str(static_cast<tiro_severity>(s));
}

struct compiler_message {
    tiro::severity severity = severity::error;
    std::string_view file;
    uint32_t line = 0;
    uint32_t column = 0;
    std::string_view text;
};

struct compiler_settings {
    using message_callback_type = std::function<void(const compiler_message& message)>;

    bool enable_dump_cst = false;
    bool enable_dump_ast = false;
    bool enable_dump_ir = false;
    bool enable_dump_bytecode = false;

    // TODO: MUST NOT throw an exception
    message_callback_type message_callback;
};

class compiled_module final {
public:
    explicit compiled_module(tiro_module_t raw_module)
        : raw_module_(raw_module) {
        TIRO_ASSERT(raw_module);
    }

    tiro_module_t raw_module() const { return raw_module_; }

private:
    detail::resource_holder<tiro_module_t, tiro_module_free> raw_module_;
};

// TODO docs
class compiler final {
public:
    compiler(std::string_view module_name)
        : raw_compiler_(construct_compiler(module_name, nullptr)) {}

    explicit compiler(std::string_view module_name, compiler_settings settings)
        : settings_(std::make_unique<compiler_settings>(std::move(settings)))
        , raw_compiler_(construct_compiler(module_name, settings_.get())) {}

    explicit compiler(tiro_compiler_t raw_compiler)
        : raw_compiler_(raw_compiler) {
        TIRO_ASSERT(raw_compiler);
    }

    void add_file(std::string_view file_name, std::string_view file_content) {
        tiro_compiler_add_file(raw_compiler_, {file_name.data(), file_name.size()},
            {file_content.data(), file_content.size()}, error_adapter());
    }

    void run() { tiro_compiler_run(raw_compiler_, error_adapter()); }

    bool has_module() const { return tiro_compiler_has_module(raw_compiler_); }

    compiled_module take_module() {
        tiro_module_t result = nullptr;
        tiro_compiler_take_module(raw_compiler_, &result, error_adapter());
        return compiled_module(result);
    }

    std::string dump_cst() const {
        detail::resource_holder<char*, std::free> result;
        tiro_compiler_dump_cst(raw_compiler_, result.out(), error_adapter());
        return std::string(result.get());
    }

    std::string dump_ast() const {
        detail::resource_holder<char*, std::free> result;
        tiro_compiler_dump_ast(raw_compiler_, result.out(), error_adapter());
        return std::string(result.get());
    }

    std::string dump_ir() const {
        detail::resource_holder<char*, std::free> result;
        tiro_compiler_dump_ir(raw_compiler_, result.out(), error_adapter());
        return std::string(result.get());
    }

    std::string dump_bytecode() const {
        detail::resource_holder<char*, std::free> result;
        tiro_compiler_dump_bytecode(raw_compiler_, result.out(), error_adapter());
        return std::string(result.get());
    }

    const compiler_settings& settings() const { return *settings_; }

    tiro_compiler_t raw_compiler() const { return raw_compiler_; }

private:
    static tiro_compiler_t
    construct_compiler(std::string_view module_name, compiler_settings* settings) {
        tiro_compiler_settings raw_settings;
        tiro_compiler_settings_init(&raw_settings);

        if (settings) {
            raw_settings.enable_dump_cst = settings->enable_dump_cst;
            raw_settings.enable_dump_ast = settings->enable_dump_ast;
            raw_settings.enable_dump_ir = settings->enable_dump_ir;
            raw_settings.enable_dump_bytecode = settings->enable_dump_bytecode;
            if (settings->message_callback) {
                raw_settings.message_callback_data = &settings->message_callback;
                raw_settings.message_callback = [](const tiro_compiler_message_t* m,
                                                    void* userdata) {
                    using callback_type = compiler_settings::message_callback_type;
                    try {
                        TIRO_ASSERT(userdata);

                        compiler_message message;
                        message.severity = static_cast<severity>(m->severity);
                        message.file = std::string_view(m->file.data, m->file.length);
                        message.line = m->line;
                        message.column = m->column;
                        message.text = std::string_view(m->text.data, m->text.length);

                        auto& func = *static_cast<callback_type*>(userdata);
                        func(message);
                    } catch (...) {
                        // TODO: No way to signal error to tiro (if needed at all!)
                        std::terminate();
                    }
                };
            }
        }

        tiro_compiler_t compiler = tiro_compiler_new(
            {module_name.data(), module_name.size()}, &raw_settings, error_adapter());
        TIRO_ASSERT(compiler); // tiro_compiler_new returns error
        return compiler;
    }

private:
    // unique_ptr: reference must stay stable (used in message callback)
    std::unique_ptr<compiler_settings> settings_;
    detail::resource_holder<tiro_compiler_t, tiro_compiler_free> raw_compiler_;
};

} // namespace tiro

#endif // TIROPP_COMPILER_HPP_INCLUDED
