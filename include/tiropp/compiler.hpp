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
#include <utility>

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
    explicit compiler(std::string_view module_name)
        : compiler(module_name, compiler_settings()) {}

    /// Constructs a new compiler instance for a module with the given name and the given settings.
    explicit compiler(std::string_view module_name, compiler_settings settings)
        : state_(std::make_unique<state_t>(std::move(settings)))
        , raw_compiler_(construct_compiler(module_name, state_.get())) {}

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
    void run() {
        struct exception_handler_t {
            state_t* state_;

            exception_handler_t(state_t* state)
                : state_(state) {
                TIRO_ASSERT(!state_->stored_exception);
            }

            ~exception_handler_t() {
                if (state_ && state_->stored_exception)
                    state_->stored_exception = std::exception_ptr();
            }

            void check() {
                if (state_ && state_->stored_exception) {
                    std::rethrow_exception(std::move(state_->stored_exception));
                }
            }
        } handler(state_.get());

        try {
            tiro_compiler_run(raw_compiler_, error_adapter());
        } catch (const tiro::api_error&) {
            handler.check(); // Rethrow stored exception from callback instead
            throw;
        }
    }

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

    tiro_compiler_t raw_compiler() const { return raw_compiler_; }

private:
    struct state_t {
        compiler_settings settings;
        std::exception_ptr stored_exception;

        state_t(compiler_settings&& settings_)
            : settings(std::move(settings_))
            , stored_exception() {}
    };

    static tiro_compiler_t construct_compiler(std::string_view module_name, state_t* state) {
        tiro_compiler_settings raw_settings;
        tiro_compiler_settings_init(&raw_settings);

        if (state) {
            auto& settings = state->settings;
            raw_settings.enable_dump_cst = settings.enable_dump_cst;
            raw_settings.enable_dump_ast = settings.enable_dump_ast;
            raw_settings.enable_dump_ir = settings.enable_dump_ir;
            raw_settings.enable_dump_bytecode = settings.enable_dump_bytecode;
            if (settings.message_callback) {
                raw_settings.message_callback_data = state;
                raw_settings.message_callback = [](const tiro_compiler_message_t* m,
                                                    void* userdata) {
                    state_t* cb_state = static_cast<state_t*>(userdata);
                    if (cb_state->stored_exception) {
                        return false;
                    }

                    compiler_message message;
                    message.severity = static_cast<severity>(m->severity);
                    message.file = detail::from_raw(m->file);
                    message.line = m->line;
                    message.column = m->column;
                    message.text = detail::from_raw(m->text);

                    try {
                        cb_state->settings.message_callback(message);
                    } catch (...) {
                        cb_state->stored_exception = std::current_exception();
                        return false;
                    }
                    return true;
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
    std::unique_ptr<state_t> state_;
    detail::resource_holder<tiro_compiler_t, tiro_compiler_free> raw_compiler_;
};

} // namespace tiro

#endif // TIROPP_COMPILER_HPP_INCLUDED
