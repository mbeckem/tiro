#ifndef TIRO_COMMON_MEMORY_NOT_NULL_HPP
#define TIRO_COMMON_MEMORY_NOT_NULL_HPP

#include "common/assert.hpp"
#include "common/defs.hpp"
#include "common/type_traits.hpp"

#include <type_traits>
#include <utility>

namespace tiro {

template<typename T>
class NotNull;

namespace detail {

template<typename T>
struct is_not_null : std::false_type {};

template<typename T>
struct is_not_null<NotNull<T>> : std::true_type {};

template<typename T>
inline constexpr bool is_not_null_v = is_not_null<T>::value;

} // namespace detail

struct GuaranteedNotNull {};

/// Use this value to indicate that the argument is guaranteed to not be null,
/// i.e. a check was performed at the call site.
inline constexpr GuaranteedNotNull guaranteed_not_null;

/// A wrapper around a pointer like class `T` that ensures that the wrapped
/// pointer is not null. It is typically used in function signatures.
///
/// Use TIRO_NN(ptr) for convenient construction with useful error reporting.
template<typename T>
class NotNull {
public:
    NotNull() = delete;

    /// Construct from a non-null pointer that is convertible to T.
    /// Raises an assertion error if the pointer is null.
    template<typename U,
        std::enable_if_t<
            !detail::is_not_null_v<std::remove_cv_t<U>> && std::is_convertible_v<U, T>>* = nullptr>
    explicit NotNull(GuaranteedNotNull, U&& ptr)
        : ptr_(std::forward<U>(ptr)) {
        static_assert(std::is_constructible_v<T, std::nullptr_t>,
            "T must be constructible with a null pointer.");

        TIRO_DEBUG_ASSERT(ptr_ != nullptr, "NotNull: pointer is null.");
    }

    /// Constructs a non-null pointer from a reference.
    /// The input reference's address must be compatible with the pointer type T.
    template<typename U, std::enable_if_t<std::is_convertible_v<U*, T>>* = nullptr>
    explicit NotNull(U& ref)
        : NotNull(guaranteed_not_null, std::addressof(ref)) {}

    NotNull(GuaranteedNotNull, std::nullptr_t) = delete;

    template<typename U, std::enable_if_t<std::is_convertible_v<U, T>>* = nullptr>
    NotNull(const NotNull<U>& other)
        : ptr_(other.ptr_) {}

    template<typename U, std::enable_if_t<std::is_convertible_v<U, T>>* = nullptr>
    NotNull(NotNull<U>&& other)
        : ptr_(std::move(other.ptr_)) {}

    NotNull(const NotNull&) = default;
    NotNull& operator=(const NotNull&) = default;

    NotNull(NotNull&&) noexcept = default;
    NotNull& operator=(NotNull&&) noexcept = default;

    const T& get() const& {
        TIRO_DEBUG_ASSERT(ptr_ != nullptr, "NotNull: pointer is null.");
        return ptr_;
    }

    T&& get() && { return std::move(ptr_); }

    operator bool() = delete;

    const T& operator->() const { return get(); }
    decltype(auto) operator*() const { return *get(); }

    operator const T&() const& { return get(); }
    operator T() && { return std::move(ptr_); }

    template<typename U, std::enable_if_t<std::is_convertible_v<T, U>>* = nullptr>
    operator U() && {
        return std::move(ptr_);
    }

    template<typename U>
    friend class NotNull;

private:
    T ptr_;
};

template<typename T>
NotNull(GuaranteedNotNull, T&& ptr) -> NotNull<remove_cvref_t<T>>;

template<typename T, typename U>
bool operator==(const NotNull<T>& lhs, const NotNull<U>& rhs) {
    return lhs.get() == rhs.get();
}

template<typename T, typename U>
bool operator!=(const NotNull<T>& lhs, const NotNull<U>& rhs) {
    return lhs.get() != rhs.get();
}

template<typename T, typename U>
bool operator<(const NotNull<T>& lhs, const NotNull<U>& rhs) {
    return lhs.get() < rhs.get();
}

template<typename T, typename U>
bool operator>(const NotNull<T>& lhs, const NotNull<U>& rhs) {
    return lhs.get() > rhs.get();
}

template<typename T, typename U>
bool operator<=(const NotNull<T>& lhs, const NotNull<U>& rhs) {
    return lhs.get() <= rhs.get();
}

template<typename T, typename U>
bool operator>=(const NotNull<T>& lhs, const NotNull<U>& rhs) {
    return lhs.get() >= rhs.get();
}

template<typename T>
bool operator==(const NotNull<T>&, std::nullptr_t) = delete;

template<typename T>
bool operator==(std::nullptr_t, const NotNull<T>&) = delete;

template<typename To, typename From>
NotNull<To> static_not_null_cast(NotNull<From> from) {
    return NotNull(guaranteed_not_null, static_cast<To>(from.get()));
}

namespace detail {

template<typename T>
auto debug_check_null(
    [[maybe_unused]] const SourceLocation& loc, T&& value, [[maybe_unused]] const char* expr) {
    if constexpr (is_not_null_v<remove_cvref_t<T>>) {
        TIRO_DEBUG_ASSERT(value != nullptr, "NotNull<T> pointer must not be null.");
        return std::forward<T>(value);
    } else {
#ifdef TIRO_DEBUG
        if (TIRO_UNLIKELY(value == nullptr)) {
            detail::assert_fail(
                loc, expr, "Attempted to construct a NotNull<T> from a null pointer.");
        }
#endif
        return NotNull(guaranteed_not_null, std::forward<T>(value));
    }
}

} // namespace detail

/// Converts the given pointer value to a NotNull<T> instance.
/// In debug mode, the pointer value will be checked and an assertion failure will be triggered
/// if the pointer is null.
#define TIRO_NN(ptr) (::tiro::detail::debug_check_null(TIRO_SOURCE_LOCATION(), (ptr), #ptr))

} // namespace tiro

namespace std {

template<typename T>
struct hash<tiro::NotNull<T>> {
    size_t operator()(const tiro::NotNull<T>& ptr) const noexcept {
        return std::hash<T>()(ptr.get());
    }
};

} // namespace std

#endif // TIRO_COMMON_MEMORY_NOT_NULL_HPP
