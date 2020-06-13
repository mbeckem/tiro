#ifndef TIRO_API_ERRORS_HPP
#define TIRO_API_ERRORS_HPP

#include "common/defs.hpp"
#include "common/function_ref.hpp"
#include "compiler/compiler.hpp"
#include "tiro/api.h"
#include "vm/context.hpp"

#include <memory>
#include <optional>
#include <string>

namespace tiro::api {

enum class ErrorKind { Static, Dynamic };

struct DynamicError;
struct StaticError;

extern const StaticError static_internal_error;
extern const StaticError static_alloc_error;

/// Reports an error as an API error code. Optionally stores detailed information
/// in `*errc` if (err is not null). The optional `produce_details` will be called
/// if `err` is present in order to obtain detailed error messages.
[[nodiscard]] tiro_errc report_error(tiro_error** err, const SourceLocation& source, tiro_errc errc,
    FunctionRef<std::string()> produce_details = {});

/// Transforms the current exception into an API error. Returns the error code
/// and optionally stores detailed information in `*err` (if err is not null).
/// Must be called from a catch block.
[[nodiscard]] tiro_errc report_exception(tiro_error** err);

/// Reports a static error. This is usually a last resort (e.g. if an allocation failed
/// or if error reporting itself failed).
[[nodiscard]] tiro_errc report_static_error(tiro_error** err, const StaticError& static_err);

/// Convenience function that automatically calls report_error with the
/// appropriate caller source location.
///
/// Example:
///
///     if (failure) {
///         return TIRO_REPORT(err, TIRO_ERROR_BAD_STATE);
///     }
///
#define TIRO_REPORT(err, ...) ::tiro::api::report_error((err), TIRO_SOURCE_LOCATION(), __VA_ARGS__)

/// Copies `str` into a zero-terminated, malloc'd string.
char* copy_to_cstr(const std::string_view str);

/// Eat all exceptions and transform them into error codes.
/// Entry points of the public C api should use this function to wrap
/// C++ code that might throw.
///
/// `err` may be null and will be used for additional error reporting, if present.
/// `fn` may either return void or an `tiro_errc` error code.
template<typename ApiFunc>
[[nodiscard]] static tiro_errc api_wrap(tiro_error** err, ApiFunc&& fn) noexcept {
    try {
        if constexpr (std::is_same_v<decltype(fn()), void>) {
            fn();
            return TIRO_OK;
        } else {
            return fn();
        }
    } catch (...) {
        return report_exception(err);
    }
}

} // namespace tiro::api

struct tiro_error {
    const tiro::api::ErrorKind kind;
    const tiro_errc errc;
    const tiro::SourceLocation source;

    constexpr tiro_error(
        tiro::api::ErrorKind kind_, tiro_errc errc_, const tiro::SourceLocation& source_)
        : kind(kind_)
        , errc(errc_)
        , source(source_) {}
};

struct tiro_vm {
    tiro::vm::Context ctx;
    tiro_vm_settings settings;

    explicit tiro_vm(const tiro_vm_settings& settings_)
        : settings(settings_) {}

    tiro_vm(const tiro_vm&) = delete;
    tiro_vm& operator=(const tiro_vm&) = delete;
};

struct tiro_compiler {
    tiro_compiler_settings settings;
    std::optional<tiro::Compiler> compiler;
    std::optional<tiro::CompilerResult> result;

    explicit tiro_compiler(const tiro_compiler_settings& settings_)
        : settings(settings_) {}

    tiro_compiler(const tiro_compiler&) = delete;
    tiro_compiler& operator=(const tiro_compiler&) = delete;
};

struct tiro_module {
    std::unique_ptr<tiro::BytecodeModule> mod;

    tiro_module(std::unique_ptr<tiro::BytecodeModule>&& mod_) noexcept
        : mod(std::move(mod_)) {}

    tiro_module(const tiro_module&) = delete;
    tiro_module& operator=(const tiro_module&) = delete;
};

#endif // TIRO_API_ERRORS_HPP
