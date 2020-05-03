#ifndef TIRO_AST_TRAVERSAL_HPP
#define TIRO_AST_TRAVERSAL_HPP

#include "tiro/ast/ast.hpp"

namespace tiro {

struct AstVisitor {
    virtual ~AstVisitor();

    virtual void visit_decl(const AstDecl& decl);
    virtual void visit_stmt(const AstStmt& stmt);
    virtual void visit_expr(const AstExpr& expr);
};

void walk_decl(AstDecl& decl, AstVisitor& visitor);
void walk_stmt(AstStmt& stmt, AstVisitor& visitor);
void walk_expr(AstExpr& expr, AstVisitor& visitor);

} // namespace tiro

#endif // TIRO_AST_TRAVERSAL_HPP
