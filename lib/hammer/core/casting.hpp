#ifndef HAMEMR_COMMON_CASTING_HPP
#define HAMEMR_COMMON_CASTING_HPP

#include "hammer/core/defs.hpp"
#include "hammer/core/type_traits.hpp"

#include <memory>

namespace hammer {

namespace detail {

template<typename To, typename From>
struct cast_result; // Not defined for non pointer types.

template<typename To, typename From>
struct cast_result<To*, From*> {
    using type = To*;
};

template<typename To, typename From>
struct cast_result<To*, const From*> {
    using type = const To*;
};

template<typename To, typename From>
using cast_result_t = typename cast_result<To, From>::type;

} // namespace detail

/*
 * LLVM-Style RTTI (enums/integers as class tags).
 *
 * Read https://llvm.org/docs/HowToSetUpLLVMStyleRTTI.html as an introduction, this system
 * is inspired by that article.
 *
 * Classes in this project that support RTTI (and our own flair of dynamic casts) must implement
 * (specialize) the test_instance class.
 */

/*
 * This class template must be specialized for every class that should
 * be able to support the type casting defined in this file.
 */
template<typename Target, typename Enabled = void>
struct InstanceTestTraits {
    static_assert(
        always_false<Target>,
        "Type casting is not supported for this target class because the instance_test_traits "
        "template was not specialized.");

    // Implement a function with the following signature:
    // static [constexpr] bool is_instance(const MyBaseClass* obj);
    // the function should return true if "obj" can be cast to the Target type.
    //
    // Example:
    //  static bool is_instance(const node* n) { return n->kind() == Target::Kind; }
    static constexpr bool is_instance(const void*) { return false; }
};

/*
 * Returns true if the parameter to this function of the requested type.
 * The object pointer must not be null.
 */
template<typename To, typename From>
constexpr bool isa(const From* obj) {
    HAMMER_ASSERT(obj != nullptr, "isa: null object.");
    unused(obj);

    using normalized_to = std::remove_cv_t<To>;
    using normalized_from = std::remove_cv_t<From>;

    if constexpr (std::is_base_of_v<normalized_to, normalized_from>) {
        // From is derived from To -> always castable.
        return true;
    } else if constexpr (std::is_base_of_v<normalized_from, normalized_to>) {
        // From is a base class of To -> possibly castable.
        using tester = InstanceTestTraits<normalized_to>;
        return tester::is_instance(obj);
    } else {
        // Neither is the case -> never castable.
        return false;
    }
}

/*
 * Casts the parameter to the requested type. Assets that the object is of the correct type.
 * The object pointer must not be null.
 */
template<typename To, typename From>
detail::cast_result_t<To*, From*> must_cast(From* obj) {
    static_assert(!std::is_volatile_v<From>, "Does not support volatile objects.");

    HAMMER_ASSERT(obj != nullptr, "must_cast<To>: attempt to cast a null pointer.");
    HAMMER_ASSERT(isa<To>(obj), "must_cast<To>: attempt to cast to an incompatible type.");
    return static_cast<detail::cast_result_t<To*, From*>>(obj);
}

/*
 * Attempts to cast the parameter to the requested type. Returns nullptr if the cast fails.
 */
template<typename To, typename From>
detail::cast_result_t<To*, From*> try_cast(From* obj) {
    static_assert(!std::is_volatile_v<From>, "Does not support volatile objects.");

    if (obj && isa<To>(obj)) {
        return static_cast<detail::cast_result_t<To*, From*>>(obj);
    }
    return nullptr;
}

} // namespace hammer

#endif // HAMEMR_COMMON_CASTING_HPP
