#ifndef TIRO_COMPILER_RESET_VALUE_HPP
#define TIRO_COMPILER_RESET_VALUE_HPP

#include <utility>

namespace tiro {

/// Replaces a value with an old value in its destructor.
/// Useful for recursive algoriths in the tree visitors.
template<typename T>
struct [[nodiscard]] ResetValue {
    T* location_ = nullptr; // nullptr -> moved from
    T old_;

    ResetValue(T & location, T old)
        : location_(std::addressof(location))
        , old_(std::move(old)) {}

    ~ResetValue() {
        if (location_)
            *location_ = std::move(old_);
    }

    ResetValue(const ResetValue&) = delete;
    ResetValue& operator=(const ResetValue&) = delete;

    ResetValue(ResetValue && other) noexcept(std::is_nothrow_move_constructible_v<T>)
        : location_(std::exchange(other.location_, nullptr))
        , old_(std::move(other.old)) {}
};

template<typename T, typename U>
ResetValue<T> replace_value(T& location, U&& new_value) {
    return ResetValue<T>(location, std::exchange(location, std::forward<U>(new_value)));
}

} // namespace tiro

#endif // TIRO_COMPILER_RESET_VALUE_HPP
