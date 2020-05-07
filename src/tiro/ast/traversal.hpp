#ifndef TIRO_AST_TRAVERSAL_HPP
#define TIRO_AST_TRAVERSAL_HPP

#include "tiro/ast/ast.hpp"

namespace tiro {

struct AstVisitor {
    virtual ~AstVisitor();

    virtual void visit_item(AstPtr<AstItem>& item);
    virtual void visit_expr(AstPtr<AstExpr>& expr);
    virtual void visit_stmt(AstPtr<AstStmt>& stmt);

    virtual void visit_binding(AstBinding& binding);
    virtual void visit_func(AstFuncDecl& func);
    virtual void visit_param(AstParamDecl& param);
    virtual void visit_property(AstProperty& prop);
};

void walk_item(AstItem& item, AstVisitor& visitor);
void walk_expr(AstExpr& expr, AstVisitor& visitor);
void walk_stmt(AstStmt& stmt, AstVisitor& visitor);

void walk_binding(AstBinding& binding, AstVisitor& visitor);
void walk_func(AstFuncDecl& func, AstVisitor& visitor);

} // namespace tiro

#endif // TIRO_AST_TRAVERSAL_HPP
