#include "api/private.hpp"

#include <exception>
#include <memory>

using namespace tiro;

namespace tiro::api {

struct StaticError final : tiro_error {
    constexpr StaticError(tiro_errc errc_, const SourceLocation& source_)
        : tiro_error(ErrorKind::Static, errc_, source_) {}
};

struct DynamicError final : tiro_error {
    const std::string details;

    DynamicError(tiro_errc errc_, const SourceLocation& source_, std::string details_)
        : tiro_error(ErrorKind::Dynamic, errc_, source_)
        , details(std::move(details_)) {}
};

constexpr StaticError static_internal_error(TIRO_ERROR_INTERNAL, TIRO_SOURCE_LOCATION());

constexpr StaticError static_alloc_error(TIRO_ERROR_ALLOC, TIRO_SOURCE_LOCATION());

tiro_errc report_static_error(tiro_error** err, const StaticError& static_error) {
    if (err && !*err) {
        // Casting away the const-ness is safe because the public interface is immutable.
        *err = static_cast<tiro_error*>(const_cast<StaticError*>(&static_error));
    }
    return static_error.errc;
}

tiro_errc report_error(tiro_error** err, const SourceLocation& source, tiro_errc errc,
    FunctionRef<std::string()> produce_details) {
    if (!err || *err) // Do not overwrite existing errors.
        return errc;

    std::string details;
    if (produce_details) {
        try {
            details = produce_details();
        } catch (...) {
            return report_static_error(err, static_internal_error);
        }
    }

    try {
        *err = new DynamicError(errc, source, std::move(details));
        return errc;
    } catch (...) {
        return report_static_error(err, static_alloc_error);
    }
}

tiro_errc report_exception(tiro_error** err) {
    auto ptr = std::current_exception();
    TIRO_DEBUG_ASSERT(ptr, "There must be an active exception.");

    try {
        std::rethrow_exception(ptr);
    } catch (const Error& ex) {
        // TODO: tiro exceptions should have file/line in debug mode
        return report_error(err, {}, TIRO_ERROR_INTERNAL, [&]() { return ex.what(); });
    } catch (const std::bad_alloc& ex) {
        return report_static_error(err, static_alloc_error);
    } catch (const std::exception& ex) {
        return report_error(err, {}, TIRO_ERROR_INTERNAL, [&]() { return ex.what(); });
    } catch (...) {
        return report_error(
            err, {}, TIRO_ERROR_INTERNAL, [&]() { return "Exception of unknown type."; });
    }
    return TIRO_OK;
}

} // namespace tiro::api

using namespace tiro;
using namespace tiro::api;

const char* tiro_errc_name(tiro_errc e) {
    switch (e) {
#define TIRO_ERRC_NAME(X) \
    case TIRO_##X:        \
        return #X;

        TIRO_ERRC_NAME(OK)
        TIRO_ERRC_NAME(ERROR_BAD_STATE)
        TIRO_ERRC_NAME(ERROR_BAD_ARG)
        TIRO_ERRC_NAME(ERROR_BAD_SOURCE)
        TIRO_ERRC_NAME(ERROR_MODULE_EXISTS)
        TIRO_ERRC_NAME(ERROR_MODULE_NOT_FOUND)
        TIRO_ERRC_NAME(ERROR_FUNCTION_NOT_FOUND)
        TIRO_ERRC_NAME(ERROR_ALLOC)
        TIRO_ERRC_NAME(ERROR_INTERNAL)

#undef TIRO_ERRC_NAME
    }
    return "<INVALID ERROR CODE>";
}

const char* tiro_errc_message(tiro_errc e) {
    switch (e) {
#define TIRO_ERRC_MESSAGE(X, str) \
    case TIRO_##X:                \
        return str;

        TIRO_ERRC_MESSAGE(OK, "No error.")
        TIRO_ERRC_MESSAGE(
            ERROR_BAD_STATE, "The instance is not in a valid state for this operation.");
        TIRO_ERRC_MESSAGE(ERROR_BAD_ARG, "Invalid argument.")
        TIRO_ERRC_MESSAGE(ERROR_BAD_SOURCE, "The source code contains errors.")
        TIRO_ERRC_MESSAGE(ERROR_MODULE_EXISTS, "A module with that name already exists.")
        TIRO_ERRC_MESSAGE(ERROR_MODULE_NOT_FOUND, "The requested module is unknown to the vm.")
        TIRO_ERRC_MESSAGE(ERROR_FUNCTION_NOT_FOUND, "The requested function is unknown to the vm.")
        TIRO_ERRC_MESSAGE(ERROR_ALLOC, "Object allocation failed.")
        TIRO_ERRC_MESSAGE(ERROR_INTERNAL, "An internal error occurred.")
    }
    return "<INVALID ERROR CODE>";
}

void tiro_error_free(tiro_error* err) {
    if (!err)
        return;

    switch (err->kind) {
    case ErrorKind::Dynamic:
        delete static_cast<DynamicError*>(err);
        break;
    case ErrorKind::Static:
        break;
    }
}

tiro_errc tiro_error_errc(const tiro_error* err) {
    return err ? err->errc : TIRO_OK;
}

const char* tiro_error_name(const tiro_error* err) {
    return tiro_errc_name(tiro_error_errc(err));
}

const char* tiro_error_message(const tiro_error* err) {
    return tiro_errc_message(tiro_error_errc(err));
}

const char* tiro_error_details(const tiro_error* err) {
    if (!err)
        return "";

    switch (err->kind) {
    case ErrorKind::Dynamic:
        return static_cast<const DynamicError*>(err)->details.c_str();
    case ErrorKind::Static:
        return "";
    }

    TIRO_DEBUG_ASSERT(false, "Invalid error kind.");
    return "";
}

const char* tiro_error_file(const tiro_error* err) {
    if (!err || !err->source.file)
        return "";
    return err->source.file;
}

int tiro_error_line(const tiro_error* err) {
    return err ? err->source.line : 0;
}

const char* tiro_error_func(const tiro_error* err) {
    if (!err || !err->source.function)
        return "";
    return err->source.function;
}
