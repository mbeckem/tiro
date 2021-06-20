#ifndef TIRO_VM_OBJECTS_EXCEPTION_HPP
#define TIRO_VM_OBJECTS_EXCEPTION_HPP

#include "vm/object_support/layout.hpp"
#include "vm/object_support/type_desc.hpp"
#include "vm/objects/value.hpp"

#include "fmt/format.h"

#include <variant>

namespace tiro::vm {

/// Represents unexpected errors.
/// Exceptions are thrown either by the vm or by the programmer
/// by invoking `std.panic()`.
///
/// TODO: Expose more internals to tiro code (stack trace, secondary exceptions etc).
class Exception final : public HeapValue {
private:
    enum Slots {
        MessageSlot,
        SecondarySlot,
        SlotCount_,
    };

public:
    using Layout = StaticLayout<StaticSlotsPiece<SlotCount_>>;

    static Exception make(Context& ctx, Handle<String> message);

    explicit Exception(Value v)
        : HeapValue(v, DebugCheck<Exception>()) {}

    String message();

    /// Returns an array of secondary exceptions. Might be null or empty.
    Nullable<Array> secondary();

    /// Adds a secondary exception to this exception.
    ///
    /// Secondary exceptions are exceptions that occur while a primary exception
    /// (the original error) is already being handled.
    ///
    /// For example, if a deferred cleanup function `close()` is called because of a panic,
    /// and `close()` itself panics, then that exception is added as a secondary exception to
    /// the original one.
    void add_secondary(Context& ctx, Handle<Exception> sec);

    Layout* layout() const { return access_heap<Layout>(); }

private:
    void secondary(Nullable<Array> secondary);
};

Exception vformat_exception_impl(Context& ctx, std::string_view format, fmt::format_args args);

template<typename... Args>
[[nodiscard]] Exception
format_exception_impl(Context& ctx, std::string_view format, const Args&... args) {
    return vformat_exception_impl(ctx, format, fmt::make_format_args(args...));
}

/// Constructs a new exception from the given format string and arguments.
///
/// NOTE: This function allocates, all inputs must be rooted.
#define TIRO_FORMAT_EXCEPTION(ctx, ...) (::tiro::vm::format_exception_impl(ctx, __VA_ARGS__))

/// Represents a value that is either a `T` or an exception object.
/// Objects of these type are returned by functions that can fail.
/// The content is not rooted, so it should be stored in a handle or returned ASAP.
///
/// Note that Fallible<T> is not convertible to Value by design, so it must
/// always be checked before using it as a plain value.
template<typename T>
class [[nodiscard]] Fallible final {
    static_assert(!std::is_convertible_v<T, Exception>, "Fallible<Exception> does not make sense.");

public:
    using ValueType = T;

    /// Constructs a fallible that contains an exception.
    Fallible(Exception ex)
        : value_(std::move(ex)) {}

    /// Constructs a fallible that contains a valid value.
    Fallible(T value)
        : value_(std::move(value)) {}

    bool has_value() const { return std::holds_alternative<T>(value_); }
    bool has_exception() const { return std::holds_alternative<Exception>(value_); }
    explicit operator bool() const { return has_value(); }

    const T& value() const {
        TIRO_DEBUG_ASSERT(has_value(), "Fallible<T> does not contain a value.");
        return std::get<T>(value_);
    }

    Exception exception() const {
        TIRO_DEBUG_ASSERT(has_exception(), "Fallible<T> does not contain an exception.");
        return std::get<Exception>(value_);
    }

private:
    // TODO: Use something else than variant?
    std::variant<T, Exception> value_;
};

template<>
class Fallible<void> final {
public:
    using ValueType = void;

    /// Constructs a fallible that contains an exception.
    Fallible(Exception ex)
        : ex_(ex) {}

    /// Constructs a fallible that signals success.
    Fallible()
        : ex_() {}

    bool has_value() const { return !has_exception(); }
    bool has_exception() const { return ex_.has_value(); }
    explicit operator bool() const { return has_value(); }

    Exception exception() const {
        TIRO_DEBUG_ASSERT(has_exception(), "Fallible<T> does not contain an exception.");
        return ex_.value();
    }

private:
    Nullable<Exception> ex_;
};

extern const TypeDesc exception_type_desc;

} // namespace tiro::vm

#endif // TIRO_VM_OBJECTS_EXCEPTION_HPP
