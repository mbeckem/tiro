#ifndef HAMMER_AST_ROOT_HPP
#define HAMMER_AST_ROOT_HPP

#include "hammer/ast/node.hpp"
#include "hammer/ast/scope.hpp"

namespace hammer::ast {

/**
 * The root node of the AST.
 */
class Root : public Node, public Scope {
public:
    Root();
    ~Root();

    File* child() const;
    void child(std::unique_ptr<File> child);

    void dump_impl(NodeFormatter& fmt) const;

private:
    File* child_ = nullptr;
};

} // namespace hammer::ast

#endif // HAMMER_AST_ROOT_HPP