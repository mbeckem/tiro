#ifndef HAMMER_COMMON_TYPE_TRAITS_HPP
#define HAMMER_COMMON_TYPE_TRAITS_HPP

#include <type_traits>

namespace hammer {

// Strips const/volatile and references from T. From c++20.
template<class T>
struct remove_cvref {
    typedef std::remove_cv_t<std::remove_reference_t<T>> type;
};

template<typename T>
using remove_cvref_t = typename remove_cvref<T>::type;

// For static asserts
template<typename T>
struct always_false_t {
    static constexpr bool value = false;
};

template<typename T>
inline constexpr bool always_false = always_false_t<T>::value;

// Special type used by some trait classes to indicate that they have not been defined.
struct undefined_type {};

// A type is treated as undefined if it is equal to or derived from undefined_type.
template<typename T>
inline constexpr bool is_undefined = std::is_base_of_v<undefined_type, T>;

template<typename T>
const T* const_ptr(T* ptr) {
    return ptr;
}

} // namespace hammer

#endif // HAMMER_COMMON_TYPE_TRAITS_HPP
