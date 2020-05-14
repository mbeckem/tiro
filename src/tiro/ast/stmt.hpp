#ifndef TIRO_AST_STMT_HPP
#define TIRO_AST_STMT_HPP

#include "tiro/ast/node.hpp"

namespace tiro {

/* [[[cog
    from codegen.ast import NODE_TYPES, define, walk_types
    
    node_types = list(walk_types(NODE_TYPES.get("Stmt")))
    define(*node_types)
]]] */
/// Represents a statement.
class AstStmt : public AstNode {
protected:
    AstStmt(AstNodeType type);

public:
    ~AstStmt();
};

/// Represents an assert statement with an optional message.
class AstAssertStmt final : public AstStmt {
public:
    AstAssertStmt();

    ~AstAssertStmt();

    AstExpr* cond() const;
    void cond(AstPtr<AstExpr> new_cond);

    AstExpr* message() const;
    void message(AstPtr<AstExpr> new_message);

private:
    AstPtr<AstExpr> cond_;
    AstPtr<AstExpr> message_;
};

/// Represents an empty statement.
class AstEmptyStmt final : public AstStmt {
public:
    AstEmptyStmt();

    ~AstEmptyStmt();
};

/// Represents an expression in a statement context.
class AstExprStmt final : public AstStmt {
public:
    AstExprStmt();

    ~AstExprStmt();

    AstExpr* expr() const;
    void expr(AstPtr<AstExpr> new_expr);

private:
    AstPtr<AstExpr> expr_;
};

/// Represents a for loop.
class AstForStmt final : public AstStmt {
public:
    AstForStmt();

    ~AstForStmt();

    AstVarDecl* decl() const;
    void decl(AstPtr<AstVarDecl> new_decl);

    AstExpr* cond() const;
    void cond(AstPtr<AstExpr> new_cond);

    AstExpr* step() const;
    void step(AstPtr<AstExpr> new_step);

    AstExpr* body() const;
    void body(AstPtr<AstExpr> new_body);

private:
    AstPtr<AstVarDecl> decl_;
    AstPtr<AstExpr> cond_;
    AstPtr<AstExpr> step_;
    AstPtr<AstExpr> body_;
};

/// Represents an item in a statement context.
class AstItemStmt final : public AstStmt {
public:
    AstItemStmt();

    ~AstItemStmt();

    AstItem* item() const;
    void item(AstPtr<AstItem> new_item);

private:
    AstPtr<AstItem> item_;
};

/// Represents a while loop.
class AstWhileStmt final : public AstStmt {
public:
    AstWhileStmt();

    ~AstWhileStmt();

    AstExpr* cond() const;
    void cond(AstPtr<AstExpr> new_cond);

    AstExpr* body() const;
    void body(AstPtr<AstExpr> new_body);

private:
    AstPtr<AstExpr> cond_;
    AstPtr<AstExpr> body_;
};
// [[[end]]]

} // namespace tiro

#endif // TIRO_AST_STMT_HPP
