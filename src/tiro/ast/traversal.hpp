#ifndef TIRO_AST_TRAVERSAL_HPP
#define TIRO_AST_TRAVERSAL_HPP

#include "tiro/ast/ast.hpp"

namespace tiro {

struct AstVisitor {
    virtual ~AstVisitor();

    virtual void visit_expr(AstPtr<AstExpr>& expr);
    virtual void visit_stmt(AstPtr<AstStmt>& stmt);
    virtual void visit_decl(AstPtr<AstDecl>& decl);
};

void walk_expr(AstExpr& expr, AstVisitor& visitor);
void walk_stmt(AstStmt& stmt, AstVisitor& visitor);
void walk_decl(AstDecl& decl, AstVisitor& visitor);

} // namespace tiro

#endif // TIRO_AST_TRAVERSAL_HPP
