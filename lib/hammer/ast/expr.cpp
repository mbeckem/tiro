#include "hammer/ast/expr.hpp"

#include "hammer/ast/node_formatter.hpp"
#include "hammer/ast/stmt.hpp"

#include <fmt/format.h>

namespace hammer::ast {

BlockExpr::BlockExpr()
    : Expr(NodeKind::BlockExpr)
    , Scope(ScopeKind::BlockScope) {}

BlockExpr::~BlockExpr() {}

void Expr::dump_impl(NodeFormatter& fmt) const {
    Node::dump_impl(fmt);
    fmt.properties("has_value", has_value());
}

void BlockExpr::add_stmt(std::unique_ptr<Stmt> child) {
    nodes_.push_back(add_child(std::move(child)));
}

void BlockExpr::dump_impl(NodeFormatter& fmt) const {
    Expr::dump_impl(fmt);

    size_t index = 0;
    for (const auto& n : nodes_) {
        std::string name = fmt::format("stmt_{}", index);
        fmt.property(name, n);
        ++index;
    }
}

void UnaryExpr::dump_impl(NodeFormatter& fmt) const {
    Expr::dump_impl(fmt);
    fmt.properties("op", to_string(operation()), "inner", inner());
}

void BinaryExpr::dump_impl(NodeFormatter& fmt) const {
    Expr::dump_impl(fmt);
    fmt.properties("op", to_string(operation()), "left_child", left_child(), "right_child",
                   right_child());
}

void VarExpr::dump_impl(NodeFormatter& fmt) const {
    Expr::dump_impl(fmt);
    fmt.properties("name", name());
}

void DotExpr::dump_impl(NodeFormatter& fmt) const {
    Expr::dump_impl(fmt);
    fmt.properties("name", name(), "expression", inner());
}

void CallExpr::dump_impl(NodeFormatter& fmt) const {
    Expr::dump_impl(fmt);
    fmt.properties("func", func());

    size_t index = 0;
    for (const auto& a : args_) {
        std::string name = fmt::format("arg_{}", index);
        fmt.property(name, a);
        ++index;
    }
}

void IndexExpr::dump_impl(NodeFormatter& fmt) const {
    Expr::dump_impl(fmt);
    fmt.properties("inner", inner(), "index", index());
}

Expr* IfExpr::condition() const {
    return condition_;
}

void IfExpr::condition(std::unique_ptr<Expr> condition) {
    remove_child(condition_);
    condition_ = add_child(std::move(condition));
}

BlockExpr* IfExpr::then_branch() const {
    return then_branch_;
}

void IfExpr::then_branch(std::unique_ptr<BlockExpr> statement) {
    remove_child(then_branch_);
    then_branch_ = add_child(std::move(statement));
}

Expr* IfExpr::else_branch() const {
    return else_branch_;
}

void IfExpr::else_branch(std::unique_ptr<Expr> statement) {
    remove_child(else_branch_);
    else_branch_ = add_child(std::move(statement));
}

void IfExpr::dump_impl(NodeFormatter& fmt) const {
    Expr::dump_impl(fmt);
    fmt.properties("condition", condition(), "then_statement", then_branch(), "else_statement",
                   else_branch());
}

void ReturnExpr::dump_impl(NodeFormatter& fmt) const {
    Expr::dump_impl(fmt);
    fmt.properties("inner", inner());
}

void ContinueExpr::dump_impl(NodeFormatter& fmt) const {
    Expr::dump_impl(fmt);
}

void BreakExpr::dump_impl(NodeFormatter& fmt) const {
    Expr::dump_impl(fmt);
}

} // namespace hammer::ast
