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

struct compiler_settings {
    using message_callback_type =
        std::function<void(severity sev, uint32_t line, uint32_t column, const char* message)>;

    bool enable_dump_ast = false;
    bool enable_dump_ir = false;
    bool enable_dump_bytecode = false;

    // MUST NOT throw an exception
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
    compiler()
        : raw_compiler_(construct_compiler(nullptr)) {}

    explicit compiler(compiler_settings settings)
        : settings_(std::make_unique<compiler_settings>(std::move(settings)))
        , raw_compiler_(construct_compiler(settings_.get())) {}

    explicit compiler(tiro_compiler_t raw_compiler)
        : raw_compiler_(raw_compiler) {
        TIRO_ASSERT(raw_compiler);
    }

    void add_file(const char* file_name, const char* file_content) {
        tiro_compiler_add_file(raw_compiler_, file_name, file_content, error_adapter());
    }

    void run() { tiro_compiler_run(raw_compiler_, error_adapter()); }

    bool has_module() const { return tiro_compiler_has_module(raw_compiler_); }

    compiled_module take_module() {
        tiro_module_t result = nullptr;
        tiro_compiler_take_module(raw_compiler_, &result, error_adapter());
        return compiled_module(result);
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
    static tiro_compiler_t construct_compiler(compiler_settings* settings) {
        tiro_compiler_settings raw_settings;
        tiro_compiler_settings_init(&raw_settings);

        if (settings) {
            raw_settings.enable_dump_ast = settings->enable_dump_ast;
            raw_settings.enable_dump_ir = settings->enable_dump_ir;
            raw_settings.enable_dump_bytecode = settings->enable_dump_bytecode;
            if (settings->message_callback) {
                raw_settings.message_callback_data = &settings->message_callback;
                raw_settings.message_callback = [](tiro_severity s, uint32_t line, uint32_t column,
                                                    const char* message, void* userdata) {
                    try {
                        TIRO_ASSERT(userdata);
                        auto& func = *static_cast<compiler_settings::message_callback_type*>(
                            userdata);
                        func(static_cast<severity>(s), line, column, message);
                    } catch (...) {
                        // TODO: No way to signal error to tiro (if needed at all!)
                        std::terminate();
                    }
                };
            }
        }

        tiro_compiler_t compiler = tiro_compiler_new(&raw_settings, error_adapter());
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
