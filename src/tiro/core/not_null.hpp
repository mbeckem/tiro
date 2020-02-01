#ifndef TIRO_CORE_NOT_NULL_HPP
#define TIRO_CORE_NOT_NULL_HPP

#include "tiro/core/defs.hpp"
#include "tiro/core/type_traits.hpp"

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

struct NullCheckDone {};

inline constexpr NullCheckDone null_check_done;

/// A wrapper around a pointer like class `T` that ensures that the wrapped
/// pointer is not null. It is typically used in function signatures.
///
/// Use TIRO_NN(ptr) for convenient construction with useful error reporting.
template<typename T>
class NotNull {
private:
    static_assert(std::is_constructible_v<T, std::nullptr_t>,
        "T must be constructible with a null pointer.");

public:
    NotNull() = delete;

    /// Construct from a non-null pointer that is convertible to T.
    /// Raises an assertion error if the pointer is null.
    // TODO: Should be explicit, use a conversion macro instead.
    template<typename U,
        std::enable_if_t<!detail::is_not_null_v<std::remove_cv_t<
                             U>> && std::is_convertible_v<U, T>>* = nullptr>
    explicit NotNull(NullCheckDone, U&& ptr)
        : ptr_(std::forward<U>(ptr)) {
        TIRO_ASSERT(ptr_ != nullptr, "NotNull: pointer is null.");
    }

    NotNull(NullCheckDone, std::nullptr_t) = delete;

    template<typename U,
        std::enable_if_t<std::is_convertible_v<U, T>>* = nullptr>
    NotNull(const NotNull<U>& other)
        : NotNull(null_check_done, other.get()) {}

    NotNull(const NotNull&) = default;
    NotNull& operator=(const NotNull&) = default;

    const T& get() const& {
        TIRO_ASSERT(ptr_ != nullptr, "NotNull: pointer is null.");
        return ptr_;
    }

    T&& get() && { return std::move(ptr_); }

    /// Explicitly disabled because NotNull pointers are always true.
    operator bool() = delete;

    const T& operator->() const { return get(); }
    decltype(auto) operator*() const { return *get(); }

    operator const T&() const& { return get(); }
    operator T &&() && { return std::move(ptr_); }

    template<typename U>
    friend class NotNull;

private:
    T ptr_;
};

template<typename T>
NotNull(NullCheckDone, T&& ptr)->NotNull<remove_cvref_t<T>>;

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

namespace detail {

template<typename T>
auto check_null(const SourceLocation& loc, T&& ptr) {
    if constexpr (is_not_null_v<remove_cvref_t<T>>) {
        TIRO_ASSERT(ptr != nullptr, "NotNull<T> pointer must not be null.");
        return std::forward<T>(ptr);
    } else {
        if (TIRO_UNLIKELY(ptr == nullptr)) {
            detail::assert_fail(loc, "ptr != nullptr",
                "Attempted to construct a NotNull<T> from a null pointer.");
        }
        return NotNull(null_check_done, std::forward<T>(ptr));
    }
}

} // namespace detail

#define TIRO_NN(ptr) (detail::check_null(TIRO_SOURCE_LOCATION(), (ptr)))

} // namespace tiro

namespace std {

template<typename T>
struct hash<tiro::NotNull<T>> {
    size_t operator()(const tiro::NotNull<T>& ptr) const noexcept {
        return std::hash<T>(ptr.get());
    }
};

} // namespace std

#endif // TIRO_CORE_NOT_NULL_HPP
