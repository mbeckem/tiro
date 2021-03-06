#ifndef TIRO_COMMON_ADT_FUNCTION_REF_HPP
#define TIRO_COMMON_ADT_FUNCTION_REF_HPP

#include "common/assert.hpp"
#include "common/defs.hpp"
#include "common/type_traits.hpp"

#include <type_traits>

namespace tiro {

template<typename Ret, typename... Args>
class FunctionRef;

namespace detail {

template<typename T>
struct is_function_ref : std::false_type {};

template<typename Ret, typename... Args>
struct is_function_ref<FunctionRef<Ret(Args...)>> : std::true_type {};

// Disable accidental override of move / copy constructor because of perfect forwarding.
template<typename T>
using disable_if_function_ref = std::enable_if_t<!is_function_ref<remove_cvref_t<T>>::value>;

} // namespace detail

/// A lightweight function reference.
/// The function object passed in the constructor is captured by reference, so it must stay
/// valid for as long as the function reference is being used.
/// This class is typically used for callbacks in function argument lists.
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

    template<typename FunctionObject, detail::disable_if_function_ref<FunctionObject>* = nullptr>
    FunctionRef(FunctionObject&& object)
        : userdata_((void*) std::addressof(object))
        , func_(&wrapper<std::remove_reference_t<FunctionObject>>) {}

    FunctionRef(Ret (*func)(Args..., void*), void* userdata)
        : userdata_(userdata)
        , func_(func) {}

    Ret operator()(Args... args) const {
        TIRO_DEBUG_ASSERT(func_, "Invalid function reference.");
        return func_(args..., userdata_);
    }

    explicit operator bool() const noexcept { return func_; }

private:
    void* userdata_ = nullptr;
    FunctionPtr func_ = nullptr;
};

} // namespace tiro

#endif // TIRO_COMMON_ADT_FUNCTION_REF_HPP
