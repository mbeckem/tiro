#include "api/internal.hpp"

#include "common/format.hpp"

#include <exception>
#include <memory>

using namespace tiro;
using namespace tiro::api;

namespace tiro::api {

struct StaticError final : tiro_error {
    constexpr StaticError(tiro_errc_t errc_)
        : tiro_error(ErrorKind::Static, errc_) {}
};

struct DynamicError final : tiro_error {
    const std::string details;

    DynamicError(tiro_errc_t errc_, std::string details_)
        : tiro_error(ErrorKind::Dynamic, errc_)
        , details(std::move(details_)) {}
};

constexpr StaticError static_internal_error(TIRO_ERROR_INTERNAL);

constexpr StaticError static_alloc_error(TIRO_ERROR_ALLOC);

void report_static_error(tiro_error_t* err, const StaticError& static_error) {
    if (!err || *err) // Do not overwrite existing errors.
        return;

    // Casting away the const-ness is safe because the public interface is immutable.
    *err = static_cast<tiro_error_t>(const_cast<StaticError*>(&static_error));
}

void report_error(tiro_error_t* err, const SourceLocation& source, tiro_errc_t errc,
    FunctionRef<std::string()> produce_details) {
    if (!err || *err) // Do not overwrite existing errors.
        return;

    std::string details = [&] {
        StringFormatStream stream;
        if (produce_details) {
            stream.format("{}", produce_details());
        }
        if (source) {
            if (!stream.str().empty())
                stream.format("\n");

            stream.format("in {}:{}", source.file, source.line, source.function);
        }
        return stream.take_str();
    }();

    *err = new DynamicError(errc, std::move(details));
}

void report_caught_exception(tiro_error_t* err) {
    if (!err || *err) // Do not overwrite existing errors.
        return;

    try {
        auto ptr = std::current_exception();
        TIRO_DEBUG_ASSERT(ptr, "there must be an active exception");

        try {
            std::rethrow_exception(ptr);
        } catch (const Error& ex) {
            return report_error(err, {}, ex.code(), [&]() { return ex.what(); });
        } catch (const std::bad_alloc& ex) {
            return report_static_error(err, static_alloc_error);
        } catch (const std::exception& ex) {
            return report_error(err, {}, TIRO_ERROR_INTERNAL, [&]() { return ex.what(); });
        } catch (...) {
            return report_error(err, {}, TIRO_ERROR_INTERNAL,
                [&]() { return "internal exception of unknown type"; });
        }
    } catch (...) {
        report_static_error(err, static_internal_error);
    }
}

} // namespace tiro::api

const char* tiro_errc_name(tiro_errc_t e) {
    switch (e) {
#define TIRO_ERRC_NAME(X) \
    case TIRO_##X:        \
        return #X;

        TIRO_ERRC_NAME(OK)
        TIRO_ERRC_NAME(ERROR_BAD_STATE)
        TIRO_ERRC_NAME(ERROR_BAD_ARG)
        TIRO_ERRC_NAME(ERROR_BAD_SOURCE)
        TIRO_ERRC_NAME(ERROR_BAD_TYPE)
        TIRO_ERRC_NAME(ERROR_BAD_KEY)
        TIRO_ERRC_NAME(ERROR_MODULE_EXISTS)
        TIRO_ERRC_NAME(ERROR_MODULE_NOT_FOUND)
        TIRO_ERRC_NAME(ERROR_EXPORT_NOT_FOUND)
        TIRO_ERRC_NAME(ERROR_OUT_OF_BOUNDS)
        TIRO_ERRC_NAME(ERROR_ALLOC)
        TIRO_ERRC_NAME(ERROR_INTERNAL)

#undef TIRO_ERRC_NAME
    }
    return "unknown error code";
}

const char* tiro_errc_message(tiro_errc_t e) {
    switch (e) {
#define TIRO_ERRC_MESSAGE(X, str) \
    case TIRO_##X:                \
        return str;

        TIRO_ERRC_MESSAGE(OK, "no error")
        TIRO_ERRC_MESSAGE(
            ERROR_BAD_STATE, "the instance is not in a valid state for this operation");
        TIRO_ERRC_MESSAGE(ERROR_BAD_ARG, "invalid argument")
        TIRO_ERRC_MESSAGE(ERROR_BAD_SOURCE, "the source code contains errors")
        TIRO_ERRC_MESSAGE(ERROR_BAD_KEY, "the key is not valid for that object")
        TIRO_ERRC_MESSAGE(
            ERROR_BAD_TYPE, "the operation is not supported on arguments of that type")
        TIRO_ERRC_MESSAGE(ERROR_MODULE_EXISTS, "a module with that name already exists")
        TIRO_ERRC_MESSAGE(ERROR_MODULE_NOT_FOUND, "the requested module is unknown to the vm")
        TIRO_ERRC_MESSAGE(ERROR_EXPORT_NOT_FOUND,
            "module does not contain an exported member with the requested name")
        TIRO_ERRC_MESSAGE(ERROR_OUT_OF_BOUNDS, "the argument is of bounds")
        TIRO_ERRC_MESSAGE(ERROR_ALLOC, "object allocation failed")
        TIRO_ERRC_MESSAGE(ERROR_INTERNAL, "an internal error occurred")
    }
    return "unknown error";
}

void tiro_error_free(tiro_error_t err) {
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

tiro_errc_t tiro_error_errc(const tiro_error_t err) {
    return err ? err->errc : TIRO_OK;
}

const char* tiro_error_name(const tiro_error_t err) {
    return tiro_errc_name(tiro_error_errc(err));
}

const char* tiro_error_message(const tiro_error_t err) {
    return tiro_errc_message(tiro_error_errc(err));
}

const char* tiro_error_details(const tiro_error_t err) {
    if (!err)
        return "";

    switch (err->kind) {
    case ErrorKind::Dynamic:
        return static_cast<const DynamicError*>(err)->details.c_str();
    case ErrorKind::Static:
        return "";
    }

    TIRO_DEBUG_ASSERT(false, "invalid error kind");
    return "";
}
