#include "hammer/ast/root.hpp"

#include "hammer/ast/file.hpp"
#include "hammer/ast/node_formatter.hpp"
#include "hammer/ast/scope.hpp"

namespace hammer::ast {

Root::Root()
    : Node(NodeKind::Root)
    , Scope(ScopeKind::GlobalScope) {}

Root::~Root() {}

File* Root::child() const {
    return child_;
}

void Root::child(std::unique_ptr<File> child) {
    remove_child(child_);
    child_ = add_child(std::move(child));
}

void Root::dump_impl(NodeFormatter& fmt) const {
    Node::dump_impl(fmt);
    fmt.properties("child", child());
}

} // namespace hammer::ast
