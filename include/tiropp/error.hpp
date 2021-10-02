#ifndef TIROPP_ERROR_HPP_INCLUDED
#define TIROPP_ERROR_HPP_INCLUDED

#include "tiropp/def.hpp"
#include "tiropp/detail/resource_holder.hpp"
#include "tiropp/fwd.hpp"

#include "tiro/error.h"

#include <cassert>
#include <exception>
#include <string>
#include <utility>

namespace tiro {

/// Defines all possible error codes.
enum class api_errc : int {
    /// Success
    ok = TIRO_OK,

    /// Instance is not in the correct state
    bad_state = TIRO_ERROR_BAD_STATE,

    /// Invalid argument
    bad_arg = TIRO_ERROR_BAD_ARG,

    /// Invalid source code
    bad_source = TIRO_ERROR_BAD_SOURCE,

    /// Operation not supported on type
    bad_type = TIRO_ERROR_BAD_TYPE,

    /// Key does not exist on object
    bad_key = TIRO_ERROR_BAD_KEY,

    /// Module name defined more than once
    module_exists = TIRO_ERROR_MODULE_EXISTS,

    /// Requested module does not exist
    module_not_found = TIRO_ERROR_MODULE_NOT_FOUND,

    /// Requested export does not exist
    export_not_found = TIRO_ERROR_EXPORT_NOT_FOUND,

    /// Argument was out of bounds
    out_of_bounds = TIRO_ERROR_OUT_OF_BOUNDS,

    /// Allocation failure
    alloc = TIRO_ERROR_ALLOC,

    /// Internal error
    internal = TIRO_ERROR_INTERNAL,
};

/// Returns the name of the given error code.
/// The returned string is allocated in static storage.
inline const char* name(api_errc e) {
    return tiro_errc_name(static_cast<tiro_errc_t>(e));
}

/// Returns the human readable message associated with the error code.
/// The returned string is allocated in static storage.
inline const char* message(api_errc e) {
    return tiro_errc_message(static_cast<tiro_errc_t>(e));
}

/// Base class for all errors thrown by this library.
class error : public std::exception {
public:
    /// A simple message line that describes the error condition. Never null.
    virtual const char* message() const noexcept = 0;

    /// Optional detailed error information. Never null, but may be empty.
    virtual const char* details() const noexcept = 0;
};

/// Generic error with a simple message.
class generic_error : public error {
public:
    explicit generic_error(std::string message)
        : message_(std::move(message)) {}

    virtual const char* message() const noexcept override { return message_.c_str(); }
    virtual const char* details() const noexcept override { return ""; }
    virtual const char* what() const noexcept override { return message(); }

private:
    std::string message_;
};

/// Thrown when a debug mode handle check failed.
class bad_handle_check final : public generic_error {
public:
    explicit bad_handle_check(std::string message)
        : generic_error(std::move(message)) {}

private:
    std::string message_;
};

/// Represents an error thrown by the tiro c library.
class api_error final : public error {
public:
    /// Constructs a new error from the given raw error.
    /// The error takes ownership of the raw error.
    explicit api_error(tiro_error_t raw_error)
        : raw_error_(raw_error)
        , what_message_(format_message(raw_error)) {
        TIRO_ASSERT(raw_error);
    }

    /// Returns the error code represented by this error.
    api_errc code() const noexcept { return static_cast<api_errc>(tiro_error_errc(raw_error_)); }

    /// Returns the wrapped tiro_error_t instance.
    tiro_error_t raw_error() const { return raw_error_; }

    /// Returns the name of the error (never null).
    const char* name() const noexcept { return tiro_error_name(raw_error_); }

    /// Returns the human readable error message (never null).
    const char* message() const noexcept override { return tiro_error_message(raw_error_); }

    /// Returns detailed error information (may be the empty string, but never null).
    const char* details() const noexcept override { return tiro_error_details(raw_error_); }

    const char* what() const noexcept override { return what_message_.c_str(); }

private:
    static std::string format_message(tiro_error_t raw_error) {
        assert(raw_error);

        std::string message;
        message += "tiro::error: ";
        message += tiro_error_name(raw_error);
        message += "\n    ";
        message += tiro_error_message(raw_error);

        const char* details = tiro_error_details(raw_error);
        if (*details != 0) {
            message += "\n    ";
            message += details;
        }
        return message;
    }

private:
    detail::resource_holder<tiro_error_t, tiro_error_free> raw_error_;
    std::string what_message_;
};

/// Error adapter class for the tiro_error_t* argument expected by most tiro_* functions.
/// This class implicitly converts to a tiro_error_t* and will throw any error from its destructor.
///
/// Thanks to Stefanus Du Toit (https://www.slideshare.net/StefanusDuToit/cpp-con-2014-hourglass-interfaces-for-c-apis)
/// for showing this technique.
class error_adapter final {
public:
    error_adapter() = default;

    ~error_adapter() noexcept(false) {
        if (raw_error_)
            throw api_error(raw_error_); // takes ownership
    }

    error_adapter(error_adapter&&) noexcept = delete;
    error_adapter& operator=(error_adapter&&) noexcept = delete;

    operator tiro_error_t*() { return &raw_error_; }

private:
    tiro_error_t raw_error_ = nullptr;
};

} // namespace tiro

#endif // TIROPP_ERROR_HPP_INCLUDED
