#ifndef TIRO_COMMON_TYPE_TRAITS_HPP
#define TIRO_COMMON_TYPE_TRAITS_HPP

#include <type_traits>

namespace tiro {

/// Strips const/volatile and references from T. From c++20.
template<typename T>
struct remove_cvref {
    using type = std::remove_cv_t<std::remove_reference_t<T>>;
};

template<typename T>
using remove_cvref_t = typename remove_cvref<T>::type;

/// Adds or remove `const` to or from T, depending on whether U is const or not.
template<typename T, typename U>
struct preserve_const
    : std::conditional<std::is_const_v<U>, std::add_const_t<T>, std::remove_const_t<T>> {};

template<typename T, typename U>
using preserve_const_t = typename preserve_const<T, U>::type;

/// Helper type whose value always evaluates to false. For static assertions.
template<typename T>
struct always_false_t {
    static constexpr bool value = false;
};

template<typename T>
inline constexpr bool always_false = always_false_t<T>::value;

/// Special type used by some trait classes to indicate that they have not been defined.
struct undefined_type {};

/// A type is treated as undefined if it is equal to or derived from undefined_type.
template<typename T>
inline constexpr bool is_undefined = std::is_base_of_v<undefined_type, T>;

/// Casts a pointer to a const pointer. Useful to make sure that a const member
/// function is being called.
template<typename T>
const T* const_ptr(T* ptr) {
    return ptr;
}

} // namespace tiro

#endif // TIRO_COMMON_TYPE_TRAITS_HPP
