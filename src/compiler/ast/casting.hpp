#ifndef TIRO_COMPILER_AST_CASTING_HPP
#define TIRO_COMPILER_AST_CASTING_HPP

#include "common/adt/not_null.hpp"
#include "common/type_traits.hpp"
#include "compiler/ast/node.hpp"
#include "compiler/ast/node_traits.hpp"

namespace tiro {

/// Returns true if the given ast node is an instance of `T`.
template<typename To, typename From, std::enable_if_t<std::is_base_of_v<AstNode, From>>* = nullptr>
bool is_instance(const From* node) {
    using Traits = AstNodeTraits<To>;

    if (!node)
        return false;

    if constexpr (std::is_base_of_v<To, From>) {
        // Trivial case: From is derived from To.
        return true;
    } else if constexpr (!std::is_base_of_v<From, To>) {
        // Trivial case: To is not derived from From, it can never be an instance of From.
        return false;
    } else {
        // Test dynamic type.
        auto type = node->type();
        if constexpr (Traits::is_base) {
            return type >= Traits::first_child_id && type <= Traits::last_child_id;
        } else {
            return type == Traits::type_id;
        }
    }
}

/// Returns true if the given ast node is an instance of `T`.
template<typename To, typename From, std::enable_if_t<std::is_base_of_v<AstNode, From>>* = nullptr>
bool is_instance(NotNull<From*> node) {
    return is_instance<To>(node.get());
}

/// Returns true if the given ast node is an instance of `T`.
template<typename To, typename From, std::enable_if_t<std::is_base_of_v<AstNode, From>>* = nullptr>
bool is_instance(const AstPtr<From>& ptr) {
    return is_instance<To>(ptr.get());
}

/// Attempts to cast the given node to an instance of type `To`. Returns nullptr on failure.
template<typename To, typename From, std::enable_if_t<std::is_base_of_v<AstNode, From>>* = nullptr>
preserve_const_t<To, From>* try_cast(From* from) {
    return is_instance<To>(from) ? static_cast<preserve_const_t<To, From>*>(from) : nullptr;
}

/// Attempts to cast the given node to an instance of type `To`. Returns nullptr on failure.
template<typename To, typename From, std::enable_if_t<std::is_base_of_v<AstNode, From>>* = nullptr>
preserve_const_t<To, From>* try_cast(NotNull<From*> from) {
    return is_instance<To>(from) ? static_cast<preserve_const_t<To, From>*>(from.get()) : nullptr;
}

/// Attempts to cast the given node to an instance of type `To`.
/// If the cast was successful, the `from` argument will no longer own that node (`from` == nullptr).
/// Returns nullptr on failure and leaves `from` untouched.
template<typename To, typename From, std::enable_if_t<std::is_base_of_v<AstNode, From>>* = nullptr>
AstPtr<preserve_const_t<To, From>> try_cast(AstPtr<From>& from) {
    if (is_instance<To>(from.get())) {
        return AstPtr<preserve_const_t<To, From>>(
            static_cast<preserve_const_t<To, From>*>(from.release()));
    }
    return nullptr;
}

/// Attempts to cast the given node to an instance of type `To`. Raises an assertion error on failure.
template<typename To, typename From, std::enable_if_t<std::is_base_of_v<AstNode, From>>* = nullptr>
NotNull<preserve_const_t<To, From>*> must_cast(From* from) {
    return TIRO_NN(try_cast<To>(from));
}

/// Attempts to cast the given node to an instance of type `To`. Raises an assertion error on failure.
template<typename To, typename From, std::enable_if_t<std::is_base_of_v<AstNode, From>>* = nullptr>
NotNull<preserve_const_t<To, From>*> must_cast(NotNull<From*> from) {
    return TIRO_NN(try_cast<To>(from));
}

/// Attempts to cast the given node to an instance of type `To`. Raises an assertion error on failure.
template<typename To, typename From, std::enable_if_t<std::is_base_of_v<AstNode, From>>* = nullptr>
NotNull<AstPtr<preserve_const_t<To, From>>> must_cast(AstPtr<From>& from) {
    return TIRO_NN(try_cast<To>(from));
}

} // namespace tiro

#endif // TIRO_COMPILER_AST_CASTING_HPP
