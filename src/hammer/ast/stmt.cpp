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

AssertStmt::AssertStmt()
    : Stmt(NodeKind::AssertStmt) {}

AssertStmt::~AssertStmt() {}

Expr* AssertStmt::condition() const {
    return condition_.get();
}

void AssertStmt::condition(std::unique_ptr<Expr> condition) {
    condition->parent(this);
    condition_ = std::move(condition);
}

StringLiteral* AssertStmt::message() const {
    return message_.get();
}

void AssertStmt::message(std::unique_ptr<StringLiteral> message) {
    message->parent(this);
    message_ = std::move(message);
}

void AssertStmt::dump_impl(NodeFormatter& fmt) const {
    Stmt::dump_impl(fmt);
    fmt.properties("condition", condition(), "message", message());
}

WhileStmt::WhileStmt()
    : Stmt(NodeKind::WhileStmt) {}

WhileStmt::~WhileStmt() {}

Expr* WhileStmt::condition() const {
    return condition_.get();
}

void WhileStmt::condition(std::unique_ptr<Expr> condition) {
    condition->parent(this);
    condition_ = std::move(condition);
}

BlockExpr* WhileStmt::body() const {
    return body_.get();
}

void WhileStmt::body(std::unique_ptr<BlockExpr> body) {
    body->parent(this);
    body_ = std::move(body);
}

void WhileStmt::dump_impl(NodeFormatter& fmt) const {
    Stmt::dump_impl(fmt);
    fmt.properties("condition", condition(), "body", body());
}

ForStmt::ForStmt()
    : Stmt(NodeKind::ForStmt)
    , Scope(ScopeKind::ForStmtScope) {}

ForStmt::~ForStmt() {}

DeclStmt* ForStmt::decl() const {
    return decl_.get();
}

void ForStmt::decl(std::unique_ptr<DeclStmt> decl) {
    decl->parent(this);
    decl_ = std::move(decl);
}

Expr* ForStmt::condition() const {
    return condition_.get();
}

void ForStmt::condition(std::unique_ptr<Expr> condition) {
    condition->parent(this);
    condition_ = std::move(condition);
}

Expr* ForStmt::step() const {
    return step_.get();
}

void ForStmt::step(std::unique_ptr<Expr> step) {
    step->parent(this);
    step_ = std::move(step);
}

BlockExpr* ForStmt::body() const {
    return body_.get();
}

void ForStmt::body(std::unique_ptr<BlockExpr> body) {
    body->parent(this);
    body_ = std::move(body);
}

void ForStmt::dump_impl(NodeFormatter& fmt) const {
    Stmt::dump_impl(fmt);
    fmt.properties("decl", decl(), "condition", condition(), "step", step(),
        "body", body());
}

DeclStmt::DeclStmt()
    : Stmt(NodeKind::DeclStmt) {}

DeclStmt::~DeclStmt() {}

VarDecl* DeclStmt::decl() const {
    return declaration_.get();
}

void DeclStmt::decl(std::unique_ptr<VarDecl> decl) {
    decl->parent(this);
    declaration_ = std::move(decl);
}

void DeclStmt::dump_impl(NodeFormatter& fmt) const {
    Stmt::dump_impl(fmt);
    fmt.properties("decl", decl());
}

ExprStmt::ExprStmt()
    : Stmt(NodeKind::ExprStmt) {}

ExprStmt::~ExprStmt() {}

Expr* ExprStmt::expr() const {
    return expr_.get();
}

void ExprStmt::expr(std::unique_ptr<Expr> expr) {
    expr->parent(this);
    expr_ = std::move(expr);
}

void ExprStmt::dump_impl(NodeFormatter& fmt) const {
    Stmt::dump_impl(fmt);
    fmt.properties("used", used(), "expr", expr());
}

} // namespace hammer::ast
