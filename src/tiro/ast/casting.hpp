#ifndef TIRO_AST_CASTING_HPP
#define TIRO_AST_CASTING_HPP

#include "tiro/ast/node.hpp"
#include "tiro/ast/node_traits.hpp"
#include "tiro/core/type_traits.hpp"

namespace tiro {

/// Returns true if the given ast node is an instance of `T`.
template<typename T>
bool is_instance(const AstNode* node) {
    using Traits = AstNodeTraits<T>;

    if (!node)
        return false;

    auto type = node->type();
    if constexpr (Traits::is_base) {
        return type >= Traits::first_child_id && type <= Traits::last_child_id;
    } else {
        return type == Traits::type_id;
    }
}

/// Returns true if the given ast node is an instance of `T`.
template<typename T, typename U,
    std::enable_if_t<std::is_base_of_v<AstNode, U>>* = nullptr>
bool is_instance(const AstPtr<U>& ptr) {
    return is_instance<T>(ptr.get());
}

/// Attempts to cast the given node to an instance of type `To`. Returns nullptr on failure.
template<typename To, typename From,
    std::enable_if_t<std::is_base_of_v<AstNode, From>>* = nullptr>
preserve_const_t<To, From>* try_cast(From* from) {
    return is_instance<To>(from)
               ? static_cast<preserve_const_t<To, From>*>(from)
               : nullptr;
}

/// Attempts to cast the given node to an instance of type `To`.
/// If the cast was successful, the `from` argument will no longer own that node (`from` == nullptr).
/// Returns nullptr on failure and leaves `from` untouched.
template<typename To, typename From,
    std::enable_if_t<std::is_base_of_v<AstNode, From>>* = nullptr>
AstPtr<preserve_const_t<To, From>> try_cast(AstPtr<From>& from) {
    if (is_instance<To>(from.get())) {
        return AstPtr<preserve_const_t<To, From>>(
            static_cast<preserve_const_t<To, From>*>(from.release()));
    }
    return nullptr;
}

} // namespace tiro

#endif // TIRO_AST_CASTING_HPP
