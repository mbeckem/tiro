#include "tiro/ast/stmt.hpp"

namespace tiro {

/* [[[cog
    from codegen.ast import NODE_TYPES, implement, walk_types
    
    node_types = list(walk_types(NODE_TYPES.get("Stmt")))
    implement(*node_types)
]]] */
AstStmt::AstStmt(AstNodeType type)
    : AstNode(type) {
    TIRO_DEBUG_ASSERT(
        type >= AstNodeType::FirstStmt && type <= AstNodeType::LastStmt,
        "Derived type is invalid for this base class.");
}

AstStmt::~AstStmt() = default;

AstAssertStmt::AstAssertStmt()
    : AstStmt(AstNodeType::AssertStmt) {}

AstAssertStmt::~AstAssertStmt() = default;

AstExpr* AstAssertStmt::cond() const {
    return cond_.get();
}

void AstAssertStmt::cond(AstPtr<AstExpr> new_cond) {
    cond_ = std::move(new_cond);
}

AstExpr* AstAssertStmt::message() const {
    return message_.get();
}

void AstAssertStmt::message(AstPtr<AstExpr> new_message) {
    message_ = std::move(new_message);
}

AstEmptyStmt::AstEmptyStmt()
    : AstStmt(AstNodeType::EmptyStmt) {}

AstEmptyStmt::~AstEmptyStmt() = default;

AstExprStmt::AstExprStmt()
    : AstStmt(AstNodeType::ExprStmt) {}

AstExprStmt::~AstExprStmt() = default;

AstExpr* AstExprStmt::expr() const {
    return expr_.get();
}

void AstExprStmt::expr(AstPtr<AstExpr> new_expr) {
    expr_ = std::move(new_expr);
}

AstForStmt::AstForStmt()
    : AstStmt(AstNodeType::ForStmt) {}

AstForStmt::~AstForStmt() = default;

AstVarDecl* AstForStmt::decl() const {
    return decl_.get();
}

void AstForStmt::decl(AstPtr<AstVarDecl> new_decl) {
    decl_ = std::move(new_decl);
}

AstExpr* AstForStmt::cond() const {
    return cond_.get();
}

void AstForStmt::cond(AstPtr<AstExpr> new_cond) {
    cond_ = std::move(new_cond);
}

AstExpr* AstForStmt::step() const {
    return step_.get();
}

void AstForStmt::step(AstPtr<AstExpr> new_step) {
    step_ = std::move(new_step);
}

AstExpr* AstForStmt::body() const {
    return body_.get();
}

void AstForStmt::body(AstPtr<AstExpr> new_body) {
    body_ = std::move(new_body);
}

AstItemStmt::AstItemStmt()
    : AstStmt(AstNodeType::ItemStmt) {}

AstItemStmt::~AstItemStmt() = default;

AstItem* AstItemStmt::item() const {
    return item_.get();
}

void AstItemStmt::item(AstPtr<AstItem> new_item) {
    item_ = std::move(new_item);
}

AstWhileStmt::AstWhileStmt()
    : AstStmt(AstNodeType::WhileStmt) {}

AstWhileStmt::~AstWhileStmt() = default;

AstExpr* AstWhileStmt::cond() const {
    return cond_.get();
}

void AstWhileStmt::cond(AstPtr<AstExpr> new_cond) {
    cond_ = std::move(new_cond);
}

AstExpr* AstWhileStmt::body() const {
    return body_.get();
}

void AstWhileStmt::body(AstPtr<AstExpr> new_body) {
    body_ = std::move(new_body);
}

// [[[end]]]

} // namespace tiro
