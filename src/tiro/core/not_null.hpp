#include "tiro/core/defs.hpp"

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

/// A wrapper around a pointer like class `T` that ensures that the wrapped
/// pointer is not null.
template<typename T>
class NotNull {
private:
    static_assert(std::is_constructible_v<T, std::nullptr_t>,
        "T must be constructible with a null pointer.");

public:
    NotNull() = delete;

    template<typename U,
        std::enable_if_t<!detail::is_not_null_v<std::remove_cv_t<
                             U>> && std::is_convertible_v<U, T>>* = nullptr>
    NotNull(U&& obj)
        : obj_(std::forward<U>(obj)) {
        TIRO_ASSERT(obj_ != nullptr, "NotNull: pointer is null.");
    }

    template<typename U,
        std::enable_if_t<std::is_convertible_v<U, T>>* = nullptr>
    NotNull(const NotNull<U>& other)
        : NotNull(other.get()) {}

    NotNull(std::nullptr_t) = delete;
    NotNull& operator=(std::nullptr_t) = delete;

    NotNull(const NotNull&) = default;
    NotNull& operator=(const NotNull&) = default;

    const T& get() const {
        TIRO_ASSERT(obj_ != nullptr, "NotNull: pointer is null.");
        return obj_;
    }

    const T& operator->() const { return get(); }
    operator T() const { return get(); }
    decltype(auto) operator*() const { return *get(); }

    template<typename U>
    friend class NotNull;

private:
    T obj_;
};

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

} // namespace tiro

namespace std {

template<typename T>
struct hash<tiro::NotNull<T>> {
    size_t operator()(const tiro::NotNull<T>& ptr) const noexcept {
        return std::hash<T>(ptr.get());
    }
};

} // namespace std
