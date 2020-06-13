#ifndef TIRO_COMPILER_AST_STMT_HPP
#define TIRO_COMPILER_AST_STMT_HPP

#include "compiler/ast/node.hpp"

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

protected:
    void do_traverse_children(FunctionRef<void(AstNode*)> callback) override;
    void do_mutate_children(MutableAstVisitor& visitor) override;
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

protected:
    void do_traverse_children(FunctionRef<void(AstNode*)> callback) override;
    void do_mutate_children(MutableAstVisitor& visitor) override;

private:
    AstPtr<AstExpr> cond_;
    AstPtr<AstExpr> message_;
};

/// Represents an empty statement.
class AstEmptyStmt final : public AstStmt {
public:
    AstEmptyStmt();

    ~AstEmptyStmt();

protected:
    void do_traverse_children(FunctionRef<void(AstNode*)> callback) override;
    void do_mutate_children(MutableAstVisitor& visitor) override;
};

/// Represents an expression in a statement context.
class AstExprStmt final : public AstStmt {
public:
    AstExprStmt();

    ~AstExprStmt();

    AstExpr* expr() const;
    void expr(AstPtr<AstExpr> new_expr);

protected:
    void do_traverse_children(FunctionRef<void(AstNode*)> callback) override;
    void do_mutate_children(MutableAstVisitor& visitor) override;

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

protected:
    void do_traverse_children(FunctionRef<void(AstNode*)> callback) override;
    void do_mutate_children(MutableAstVisitor& visitor) override;

private:
    AstPtr<AstVarDecl> decl_;
    AstPtr<AstExpr> cond_;
    AstPtr<AstExpr> step_;
    AstPtr<AstExpr> body_;
};

/// Represents a variable declaration in statement context
class AstVarStmt final : public AstStmt {
public:
    AstVarStmt();

    ~AstVarStmt();

    AstVarDecl* decl() const;
    void decl(AstPtr<AstVarDecl> new_decl);

protected:
    void do_traverse_children(FunctionRef<void(AstNode*)> callback) override;
    void do_mutate_children(MutableAstVisitor& visitor) override;

private:
    AstPtr<AstVarDecl> decl_;
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

protected:
    void do_traverse_children(FunctionRef<void(AstNode*)> callback) override;
    void do_mutate_children(MutableAstVisitor& visitor) override;

private:
    AstPtr<AstExpr> cond_;
    AstPtr<AstExpr> body_;
};
// [[[end]]]

} // namespace tiro

#endif // TIRO_COMPILER_AST_STMT_HPP
