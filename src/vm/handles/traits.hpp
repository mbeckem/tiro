#ifndef TIRO_VM_HANDLES_TRAITS_HPP
#define TIRO_VM_HANDLES_TRAITS_HPP

#include "common/type_traits.hpp"
#include "vm/handles/fwd.hpp"

#include <type_traits>

namespace tiro::vm {

namespace detail {

struct NoWrapper {};

template<typename CleanT>
inline constexpr bool is_wrapper_impl =
    !std::is_same_v<typename WrapperTraits<CleanT>::WrappedType, detail::NoWrapper>;

template<typename CleanT>
using WrappedTypeImpl = std::conditional_t<is_wrapper_impl<CleanT>,
    typename WrapperTraits<CleanT>::WrappedType, CleanT>;

} // namespace detail

/// Type trait to access the inner value from a value wrapper (such as
/// a handle or a local).
template<typename T, typename Enable>
struct WrapperTraits {
    using WrappedType = detail::NoWrapper;

    // using WrappedType
    // static WrappedType get(const T& wrapper)
};

/// True if `T` is a wrapper type.
template<typename T>
inline constexpr bool is_wrapper = detail::is_wrapper_impl<remove_cvref_t<T>>;

/// Resolves to the wrapped type of `T` specializes WrapperTraits. Otherwise resolves to `T` directly.
template<typename T>
using WrappedType = detail::WrappedTypeImpl<remove_cvref_t<T>>;

/// Unwraps the value from the given instance. If `instance` does not specialize WrapperTraits, then
/// `ìnstance` is returned directly.
template<typename T>
WrappedType<T> unwrap_value(const T& instance) {
    if constexpr (is_wrapper<T>) {
        return WrapperTraits<T>::get(instance);
    } else {
        return instance;
    }
}

} // namespace tiro::vm

#endif // TIRO_VM_HANDLES_TRAITS_HPP
