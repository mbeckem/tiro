#include "compiler/ast/ast.hpp"

namespace tiro {

/* [[[cog
    from codegen.ast import NODE_TYPES, implement, walk_types

    roots = [NODE_TYPES.get(name) for name in ["File", "Stmt"]]
    node_types = list(walk_types(*roots))
    implement(*node_types)
]]] */
AstFile::AstFile()
    : AstNode(AstNodeType::File)
    , items_() {}

AstFile::~AstFile() = default;

AstNodeList<AstStmt>& AstFile::items() {
    return items_;
}

const AstNodeList<AstStmt>& AstFile::items() const {
    return items_;
}

void AstFile::items(AstNodeList<AstStmt> new_items) {
    items_ = std::move(new_items);
}

void AstFile::do_traverse_children(FunctionRef<void(AstNode*)> callback) {
    AstNode::do_traverse_children(callback);
    traverse_list(items_, callback);
}

void AstFile::do_mutate_children(MutableAstVisitor& visitor) {
    AstNode::do_mutate_children(visitor);
    visitor.visit_stmt_list(items_);
}

AstStmt::AstStmt(AstNodeType type)
    : AstNode(type) {
    TIRO_DEBUG_ASSERT(type >= AstNodeType::FirstStmt && type <= AstNodeType::LastStmt,
        "Derived type is invalid for this base class.");
}

AstStmt::~AstStmt() = default;

void AstStmt::do_traverse_children(FunctionRef<void(AstNode*)> callback) {
    AstNode::do_traverse_children(callback);
}

void AstStmt::do_mutate_children(MutableAstVisitor& visitor) {
    AstNode::do_mutate_children(visitor);
}

AstAssertStmt::AstAssertStmt()
    : AstStmt(AstNodeType::AssertStmt)
    , cond_()
    , message_() {}

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

void AstAssertStmt::do_traverse_children(FunctionRef<void(AstNode*)> callback) {
    AstStmt::do_traverse_children(callback);
    callback(cond_.get());
    callback(message_.get());
}

void AstAssertStmt::do_mutate_children(MutableAstVisitor& visitor) {
    AstStmt::do_mutate_children(visitor);
    visitor.visit_expr(cond_);
    visitor.visit_expr(message_);
}

AstDeclStmt::AstDeclStmt()
    : AstStmt(AstNodeType::DeclStmt)
    , decl_() {}

AstDeclStmt::~AstDeclStmt() = default;

AstDecl* AstDeclStmt::decl() const {
    return decl_.get();
}

void AstDeclStmt::decl(AstPtr<AstDecl> new_decl) {
    decl_ = std::move(new_decl);
}

void AstDeclStmt::do_traverse_children(FunctionRef<void(AstNode*)> callback) {
    AstStmt::do_traverse_children(callback);
    callback(decl_.get());
}

void AstDeclStmt::do_mutate_children(MutableAstVisitor& visitor) {
    AstStmt::do_mutate_children(visitor);
    visitor.visit_decl(decl_);
}

AstEmptyStmt::AstEmptyStmt()
    : AstStmt(AstNodeType::EmptyStmt) {}

AstEmptyStmt::~AstEmptyStmt() = default;

void AstEmptyStmt::do_traverse_children(FunctionRef<void(AstNode*)> callback) {
    AstStmt::do_traverse_children(callback);
}

void AstEmptyStmt::do_mutate_children(MutableAstVisitor& visitor) {
    AstStmt::do_mutate_children(visitor);
}

AstExprStmt::AstExprStmt()
    : AstStmt(AstNodeType::ExprStmt)
    , expr_() {}

AstExprStmt::~AstExprStmt() = default;

AstExpr* AstExprStmt::expr() const {
    return expr_.get();
}

void AstExprStmt::expr(AstPtr<AstExpr> new_expr) {
    expr_ = std::move(new_expr);
}

void AstExprStmt::do_traverse_children(FunctionRef<void(AstNode*)> callback) {
    AstStmt::do_traverse_children(callback);
    callback(expr_.get());
}

void AstExprStmt::do_mutate_children(MutableAstVisitor& visitor) {
    AstStmt::do_mutate_children(visitor);
    visitor.visit_expr(expr_);
}

AstForStmt::AstForStmt()
    : AstStmt(AstNodeType::ForStmt)
    , decl_()
    , cond_()
    , step_()
    , body_() {}

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

void AstForStmt::do_traverse_children(FunctionRef<void(AstNode*)> callback) {
    AstStmt::do_traverse_children(callback);
    callback(decl_.get());
    callback(cond_.get());
    callback(step_.get());
    callback(body_.get());
}

void AstForStmt::do_mutate_children(MutableAstVisitor& visitor) {
    AstStmt::do_mutate_children(visitor);
    visitor.visit_var_decl(decl_);
    visitor.visit_expr(cond_);
    visitor.visit_expr(step_);
    visitor.visit_expr(body_);
}

AstWhileStmt::AstWhileStmt()
    : AstStmt(AstNodeType::WhileStmt)
    , cond_()
    , body_() {}

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

void AstWhileStmt::do_traverse_children(FunctionRef<void(AstNode*)> callback) {
    AstStmt::do_traverse_children(callback);
    callback(cond_.get());
    callback(body_.get());
}

void AstWhileStmt::do_mutate_children(MutableAstVisitor& visitor) {
    AstStmt::do_mutate_children(visitor);
    visitor.visit_expr(cond_);
    visitor.visit_expr(body_);
}
// [[[end]]]

} // namespace tiro
