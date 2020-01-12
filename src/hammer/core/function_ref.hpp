#ifndef HAMMER_CORE_FUNCTION_REF_HPP
#define HAMMER_CORE_FUNCTION_REF_HPP

#include "hammer/core/defs.hpp"
#include "hammer/core/type_traits.hpp"

#include <type_traits>

namespace hammer {

template<typename Ret, typename... Args>
class FunctionRef;

namespace detail {

template<typename T>
struct is_function_ref : std::false_type {};

template<typename Ret, typename... Args>
struct is_function_ref<FunctionRef<Ret(Args...)>> : std::true_type {};

// Disable accidental override of move / copy constructor because of perfect forwarding.
template<typename T>
using disable_if_function_ref =
    std::enable_if_t<!is_function_ref<remove_cvref_t<T>>::value>;

} // namespace detail

/// A lightweight function reference.
/// The function object passed in the constructor is captured by reference, so it must stay
/// valid for as long as the function reference is being used.
/// This class is typically used for callbacks in function argument lists.
///
/// TODO use this at more places to get rid of some templates.
template<typename Ret, typename... Args>
class FunctionRef<Ret(Args...)> final {
private:
    using FunctionPtr = Ret (*)(Args..., void*);

    template<typename FunctionObject>
    static Ret wrapper(Args... args, void* userdata) {
        return static_cast<FunctionObject*>(userdata)->operator()(args...);
    }

public:
    FunctionRef() = default;

    template<typename FunctionObject,
        detail::disable_if_function_ref<FunctionObject>* = nullptr>
    FunctionRef(FunctionObject&& object)
        : userdata_(std::addressof(object))
        , func_(&wrapper<std::remove_reference_t<FunctionObject>>) {}

    FunctionRef(Ret (*func)(Args..., void*), void* userdata)
        : userdata_(userdata)
        , func_(func) {}

    Ret operator()(Args... args) const {
        HAMMER_ASSERT(func_, "Invalid function reference.");
        return func_(args..., userdata_);
    }

private:
    void* userdata_ = nullptr;
    FunctionPtr func_ = nullptr;
};

} // namespace hammer

#endif // HAMMER_CORE_FUNCTION_REF_HPP
