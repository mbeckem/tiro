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

/// Defines the possible attachments that can be emitted by the compiler.
enum class attachment : int {
    cst = TIRO_ATTACHMENT_CST,
    ast = TIRO_ATTACHMENT_AST,
    ir = TIRO_ATTACHMENT_IR,
    bytecode = TIRO_ATTACHMENT_BYTECODE,
};

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
        : state_(std::make_unique<state_t>())
        , raw_compiler_(construct_compiler(module_name)) {}

    /// Add a source file to the compiler's source set.
    /// Can only be called before compilation started.
    ///
    /// `file_name` should be unique in the current source set.
    void add_file(std::string_view file_name, std::string_view file_content) {
        tiro_compiler_add_file(raw_compiler_, detail::to_raw(file_name),
            detail::to_raw(file_content), error_adapter());
    }

    /// Sets the callback function that will be invoked for every diagnostic message emitted by the compiler.
    /// The callback will only be invoked from `run()`.
    ///
    /// Data referenced by the callback must remain valid for the lifetime of the compiler instance.
    ///
    /// The default message callback prints messages to stdout.
    void set_message_callback(std::function<void(const compiler_message& message)> callback) {
        if (!callback) {
            state_->message_callback = {};
            tiro_compiler_set_message_callback(raw_compiler_, nullptr, nullptr, error_adapter());
            return;
        }

        state_->message_callback = std::move(callback);
        tiro_message_callback_t native_callback = [](const tiro_compiler_message_t* m,
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
                cb_state->message_callback(message);
            } catch (...) {
                cb_state->stored_exception = std::current_exception();
                return false;
            }
            return true;
        };
        tiro_compiler_set_message_callback(
            raw_compiler_, native_callback, state_.get(), error_adapter());
    }

    /// Requests generation of the given attachment when the compiler runs.
    /// After `run()` has finished execution, the attachments may be retrieved
    /// by calling `get_attachment()`.
    ///
    /// Note that some attachments may not be available if the compilation process failed.
    void request_attachment(attachment a) {
        tiro_compiler_request_attachment(
            raw_compiler_, static_cast<tiro_attachment_t>(a), error_adapter());
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

    /// Returns the given attachment from the compiler.
    /// Requires that `run()` has finished execution.
    std::string get_attachment(attachment a) const {
        detail::resource_holder<char*, std::free> result;
        tiro_compiler_get_attachment(
            raw_compiler_, static_cast<tiro_attachment_t>(a), result.out(), error_adapter());
        return std::string(result.get());
    }

    tiro_compiler_t raw_compiler() const { return raw_compiler_; }

private:
    struct state_t {
        std::function<void(const compiler_message& message)> message_callback{};
        std::exception_ptr stored_exception{};
    };

    static tiro_compiler_t construct_compiler(std::string_view module_name) {
        tiro_compiler_t compiler = tiro_compiler_new(detail::to_raw(module_name), error_adapter());
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
