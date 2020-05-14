#ifndef TIRO_AST_PTR_HPP
#define TIRO_AST_PTR_HPP

#include "tiro/ast/fwd.hpp"
#include "tiro/core/defs.hpp"

#include <utility>

namespace tiro {

template<typename T, typename... Args>
AstPtr<T> allocate_node(Args&&... args) {
    return AstPtr<T>(new T(std::forward<Args>(args)...));
}

template<typename T>
AstPtr<T> box_node(T node) {
    return AstPtr<T>(new T(std::move(node)));
}

} // namespace tiro

#endif // TIRO_AST_PTR_HPP
