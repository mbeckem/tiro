#include "hammer/ast/stmt.hpp"

#include "hammer/ast/decl.hpp"
#include "hammer/ast/expr.hpp"
#include "hammer/ast/literal.hpp"
#include "hammer/ast/node_formatter.hpp"

#include <fmt/format.h>

namespace hammer::ast {

void EmptyStmt::dump_impl(NodeFormatter& fmt) const {
    Stmt::dump_impl(fmt);
}

void AssertStmt::condition(std::unique_ptr<Expr> condition) {
    remove_child(condition_);
    condition_ = add_child(std::move(condition));
}

void AssertStmt::message(std::unique_ptr<StringLiteral> message) {
    remove_child(message_);
    message_ = add_child(std::move(message));
}

void AssertStmt::dump_impl(NodeFormatter& fmt) const {
    Stmt::dump_impl(fmt);
    fmt.properties("condition", condition(), "message", message());
}

Expr* WhileStmt::condition() const {
    return condition_;
}

void WhileStmt::condition(std::unique_ptr<Expr> condition) {
    remove_child(condition_);
    condition_ = add_child(std::move(condition));
}

BlockExpr* WhileStmt::body() const {
    return body_;
}

void WhileStmt::body(std::unique_ptr<BlockExpr> body) {
    remove_child(body_);
    body_ = add_child(std::move(body));
}

void WhileStmt::dump_impl(NodeFormatter& fmt) const {
    Stmt::dump_impl(fmt);
    fmt.properties("condition", condition(), "body", body());
}

DeclStmt* ForStmt::decl() const {
    return decl_;
}

void ForStmt::decl(std::unique_ptr<DeclStmt> decl) {
    remove_child(decl_);
    decl_ = add_child(std::move(decl));
}

Expr* ForStmt::condition() const {
    return condition_;
}

void ForStmt::condition(std::unique_ptr<Expr> condition) {
    remove_child(condition_);
    condition_ = add_child(std::move(condition));
}

Expr* ForStmt::step() const {
    return step_;
}

void ForStmt::step(std::unique_ptr<Expr> step) {
    remove_child(step_);
    step_ = add_child(std::move(step));
}

BlockExpr* ForStmt::body() const {
    return body_;
}

void ForStmt::body(std::unique_ptr<BlockExpr> body) {
    remove_child(body_);
    body_ = add_child(std::move(body));
}

void ForStmt::dump_impl(NodeFormatter& fmt) const {
    Stmt::dump_impl(fmt);
    fmt.properties("decl", decl(), "condition", condition(), "step", step(),
        "body", body());
}

DeclStmt::DeclStmt()
    : Stmt(NodeKind::DeclStmt) {}

DeclStmt::~DeclStmt() {}

VarDecl* DeclStmt::declaration() const {
    return declaration_;
}

void DeclStmt::declaration(std::unique_ptr<VarDecl> decl) {
    remove_child(declaration_);
    declaration_ = add_child(std::move(decl));
}

void DeclStmt::dump_impl(NodeFormatter& fmt) const {
    Stmt::dump_impl(fmt);
    fmt.properties("declaration", declaration());
}

Expr* ExprStmt::expression() const {
    return expr_;
}

void ExprStmt::expression(std::unique_ptr<Expr> e) {
    remove_child(expr_);
    expr_ = add_child(std::move(e));
}

void ExprStmt::dump_impl(NodeFormatter& fmt) const {
    Stmt::dump_impl(fmt);
    fmt.properties("used", used(), "expression", expression());
}

} // namespace hammer::ast
