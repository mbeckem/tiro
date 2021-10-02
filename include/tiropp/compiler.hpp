#ifndef TIROPP_COMPILER_HPP_INCLUDED
#define TIROPP_COMPILER_HPP_INCLUDED

#include "tiropp/def.hpp"
#include "tiropp/detail/resource_holder.hpp"
#include "tiropp/detail/translate.hpp"
#include "tiropp/error.hpp"
#include "tiropp/fwd.hpp"

#include "tiro/compiler.h"

#include <functional>
#include <memory>

namespace tiro {

/// Defines the possible values for the severity of diagnostic compiler messages.
enum class severity : int {
    warning = TIRO_SEVERITY_WARNING,
    error = TIRO_SEVERITY_ERROR,
};

/// Returns the string representation of the given severity value.
/// The returned string is allocated in static storage.
inline const char* to_string(severity s) {
    return tiro_severity_str(static_cast<tiro_severity>(s));
}

/// Represents a diagnostic message emitted by the compiler.
/// All fields are only valid for the duration of the `message_callback` function call.
struct compiler_message {
    /// The severity of this message.
    tiro::severity severity = severity::error;

    /// The relevant source file. May be empty if there is no source file associated with this message.
    std::string_view file;

    /// Source line (1 based). Zero if unavailable.
    uint32_t line = 0;

    /// Source column (1 based). Zero if unavailable.
    uint32_t column = 0;

    /// The message text.
    std::string_view text;
};

/// An instance of this type can be passed to the compiler to configure it.
/// The default constructor fills an instance with default values.
struct compiler_settings {
    using message_callback_type = std::function<void(const compiler_message& message)>;

    /// Enables the compiler's dump_cst() method.
    bool enable_dump_cst = false;

    /// Enables the compiler's dump_ast() method.
    bool enable_dump_ast = false;

    /// Enables the compiler's dump_ir() method.
    bool enable_dump_ir = false;

    /// Enables the compiler's dump_bytecode() method.
    bool enable_dump_bytecode = false;

    /// Callback for diagnostic messages (may be empty).
    /// The compiler will print to the process output stream if this is not set.
    // TODO: MUST NOT throw an exception
    message_callback_type message_callback;
};

/// Represents a compiled bytecode module.
/// Modules are produced by the compiler.
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

/// Translates a set of source files into a module.
class compiler final {
public:
    /// Constructs a new compiler instance for a module with the given name.
    compiler(std::string_view module_name)
        : raw_compiler_(construct_compiler(module_name, nullptr)) {}

    /// Constructs a new compiler instance for a module with the given name and the given settings.
    explicit compiler(std::string_view module_name, compiler_settings settings)
        : settings_(std::make_unique<compiler_settings>(std::move(settings)))
        , raw_compiler_(construct_compiler(module_name, settings_.get())) {}

    explicit compiler(tiro_compiler_t raw_compiler)
        : raw_compiler_(raw_compiler) {
        TIRO_ASSERT(raw_compiler);
    }

    /// Add a source file to the compiler's source set.
    /// Can only be called before compilation started.
    ///
    /// `file_name` should be unique in the current source set.
    void add_file(std::string_view file_name, std::string_view file_content) {
        tiro_compiler_add_file(raw_compiler_, detail::to_raw(file_name),
            detail::to_raw(file_content), error_adapter());
    }

    /// Run the compiler on the set of source files provided via `add_file`.
    /// Requires at least one source file.
    /// This function can only be called once for every compiler instance.
    void run() { tiro_compiler_run(raw_compiler_, error_adapter()); }

    /// Returns true if the compiler has successfully compiled a bytecode module.
    bool has_module() const { return tiro_compiler_has_module(raw_compiler_); }

    /// Extracts the compiled module from the compiler and returns it.
    /// For this to work, run() must have completed successfully.
    compiled_module take_module() {
        tiro_module_t result = nullptr;
        tiro_compiler_take_module(raw_compiler_, &result, error_adapter());
        return compiled_module(result);
    }

    /// Returns a dump of the compiler's concrete syntax tree.
    /// Can only be called after a call to run(), and only if the `enable_dump_cst` option was set to true.
    std::string dump_cst() const {
        detail::resource_holder<char*, std::free> result;
        tiro_compiler_dump_cst(raw_compiler_, result.out(), error_adapter());
        return std::string(result.get());
    }

    /// Returns a dump of the compiler's abstract syntax tree.
    /// Can only be called after a call to run(), and only if the `enable_dump_ast` option was set to true.
    std::string dump_ast() const {
        detail::resource_holder<char*, std::free> result;
        tiro_compiler_dump_ast(raw_compiler_, result.out(), error_adapter());
        return std::string(result.get());
    }

    /// Returns a dump of the compiler's internal representation.
    /// Can only be called after a call to run(), and only if the `enable_dump_ir` option was set to true.
    std::string dump_ir() const {
        detail::resource_holder<char*, std::free> result;
        tiro_compiler_dump_ir(raw_compiler_, result.out(), error_adapter());
        return std::string(result.get());
    }

    /// Returns a dump of the disassembled bytecode.
    /// Can only be called after a call to run(), and only if the `enable_bytecode` option was set to true.
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
                        message.file = detail::from_raw(m->file);
                        message.line = m->line;
                        message.column = m->column;
                        message.text = detail::from_raw(m->text);

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
            detail::to_raw(module_name), &raw_settings, error_adapter());
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
