#ifndef TIRO_API_INTERNAL_HPP
#define TIRO_API_INTERNAL_HPP

#include "tiro/api.h"

#include "common/adt/function_ref.hpp"
#include "common/defs.hpp"
#include "compiler/compiler.hpp"
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

/// Reports a static error. This is usually a last resort (e.g. if an allocation failed
/// or if error reporting itself failed).
/// Note: existing errors in `err` will not be overwritten.
void report_static_error(tiro_error_t* err, const StaticError& static_err);

/// Reports an error as an API error code. Optionally stores detailed information
/// in `*err` if (err is not null). The optional `produce_details` will be called
/// if `err` is present in order to obtain detailed error messages.
/// Note: existing errors in `err` will not be overwritten.
void report_error(tiro_error_t* err, const SourceLocation& source, tiro_errc_t errc,
    FunctionRef<std::string()> produce_details = {});

/// Transforms the current exception into an API error (if err is not null).
/// Must be called from a catch block.
/// Note: existing errors in `err` will not be overwritten.
void report_caught_exception(tiro_error_t* err);

/// Convenience macro that automatically calls report_error with the
/// appropriate caller source location.
///
/// Example:
///
///     if (failure) {
///         return TIRO_REPORT(err, TIRO_ERROR_BAD_STATE);
///     }
///
/// Note: existing errors in `err` will not be overwritten.
#define TIRO_REPORT(err, ...) ::tiro::api::report_error((err), TIRO_SOURCE_LOCATION(), __VA_ARGS__)

/// Copies `str` into a zero-terminated, malloc'd string.
char* copy_to_cstr(std::string_view str);

/// Eat all exceptions and transform them into error codes.
/// Entry points of the public C api should use this function to wrap
/// C++ code that might throw.
///
/// `err` may be null and will be used for error reporting, if present.
template<typename ApiFunc>
void entry_point(tiro_error_t* err, ApiFunc&& fn) noexcept {
    try {
        fn();
    } catch (...) {
        report_caught_exception(err);
    }
}

/// Like the function above, but returns a default value on failure.
template<typename ApiFunc, typename DefaultVal>
auto entry_point(tiro_error_t* err, DefaultVal default_value, ApiFunc&& fn) {
    using return_type = decltype(fn());
    try {
        return fn();
    } catch (...) {
        report_caught_exception(err);
        return static_cast<return_type>(default_value);
    }
}

} // namespace tiro::api

struct tiro_error {
    const tiro::api::ErrorKind kind;
    const tiro_errc_t errc;

    constexpr tiro_error(tiro::api::ErrorKind kind_, tiro_errc_t errc_)
        : kind(kind_)
        , errc(errc_) {}
};

struct tiro_vm {
    void* external_userdata;
    tiro::vm::Context ctx;

    explicit tiro_vm(void* external_userdata_, tiro::vm::ContextSettings settings)
        : external_userdata(external_userdata_)
        , ctx(std::move(settings)) {
        ctx.userdata(this);
    }

    tiro_vm(const tiro_vm&) = delete;
    tiro_vm& operator=(const tiro_vm&) = delete;
};

inline tiro_vm* vm_from_context(tiro::vm::Context& ctx) {
    void* userdata = ctx.userdata();
    TIRO_DEBUG_ASSERT(userdata != nullptr, "Invalid userdata on context, expected the vm pointer.");
    return static_cast<tiro_vm*>(userdata);
}

// Never actually defined or used. Handles have public type `tiro_value*` and will
// be cast to their real type, which is `vm::Value*`.
struct tiro_value;

inline tiro_handle_t to_external(tiro::vm::MutHandle<tiro::vm::Value> h) {
    return reinterpret_cast<tiro_handle_t>(tiro::vm::get_valid_slot(h));
}

inline tiro::vm::MutHandle<tiro::vm::Value> to_internal(tiro_handle_t h) {
    return tiro::vm::MutHandle<tiro::vm::Value>::from_raw_slot(
        reinterpret_cast<tiro::vm::Value*>(h));
}

inline tiro::vm::MaybeMutHandle<tiro::vm::Value> to_internal_maybe(tiro_handle_t h) {
    if (h) {
        return to_internal(h);
    }
    return {};
}

// Never actually defined, pointer to this point to the actual frame instead.
// This is fine in this case because the lifetime is stack based for sync function calls.
struct tiro_sync_frame;

inline tiro_sync_frame_t to_external(tiro::vm::NativeFunctionFrame* frame) {
    return reinterpret_cast<tiro_sync_frame_t>(frame);
}

inline tiro::vm::NativeFunctionFrame* to_internal(tiro_sync_frame_t frame) {
    return reinterpret_cast<tiro::vm::NativeFunctionFrame*>(frame);
}

// Never actually defined. Async frames have public type `tiro_async_frame` but will be
// cast to their real type, which is `vm::NativeAsyncFunctionFrame`.
// This is fine in this case because the lifetime is stack based for sync function calls.
struct tiro_async_frame;

inline tiro_async_frame_t to_external(tiro::vm::NativeAsyncFunctionFrame* frame) {
    return reinterpret_cast<tiro_async_frame_t>(frame);
}

inline tiro::vm::NativeAsyncFunctionFrame* to_internal(tiro_async_frame_t frame) {
    return reinterpret_cast<tiro::vm::NativeAsyncFunctionFrame*>(frame);
}

struct tiro_compiler {
    tiro_compiler_settings_t settings;
    std::optional<tiro::Compiler> compiler;
    std::optional<tiro::CompilerResult> result;

    explicit tiro_compiler(const tiro_compiler_settings_t& settings_)
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

#endif // TIRO_API_INTERNAL_HPP
