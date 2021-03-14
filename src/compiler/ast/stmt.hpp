#ifndef TIRO_COMPILER_AST_STMT_HPP
#define TIRO_COMPILER_AST_STMT_HPP

#include "compiler/ast/node.hpp"

namespace tiro {

/* [[[cog
    from codegen.ast import NODE_TYPES, define, walk_types

    roots = [NODE_TYPES.get(name) for name in ["File", "Stmt"]]
    node_types = list(walk_types(*roots))
    define(*node_types)
]]] */
/// Represents the contents of a file.
class AstFile final : public AstNode {
public:
    AstFile();

    ~AstFile();

    AstNodeList<AstStmt>& items();
    const AstNodeList<AstStmt>& items() const;
    void items(AstNodeList<AstStmt> new_items);

protected:
    void do_traverse_children(FunctionRef<void(AstNode*)> callback) const override;
    void do_mutate_children(MutableAstVisitor& visitor) override;

private:
    AstNodeList<AstStmt> items_;
};

/// Represents a statement.
class AstStmt : public AstNode {
protected:
    AstStmt(AstNodeType type);

public:
    ~AstStmt();

protected:
    void do_traverse_children(FunctionRef<void(AstNode*)> callback) const override;
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
    void do_traverse_children(FunctionRef<void(AstNode*)> callback) const override;
    void do_mutate_children(MutableAstVisitor& visitor) override;

private:
    AstPtr<AstExpr> cond_;
    AstPtr<AstExpr> message_;
};

/// Represents a declaration in a statement context.
class AstDeclStmt final : public AstStmt {
public:
    AstDeclStmt();

    ~AstDeclStmt();

    AstDecl* decl() const;
    void decl(AstPtr<AstDecl> new_decl);

protected:
    void do_traverse_children(FunctionRef<void(AstNode*)> callback) const override;
    void do_mutate_children(MutableAstVisitor& visitor) override;

private:
    AstPtr<AstDecl> decl_;
};

/// Represents an expression that will be evaluated on scope exit.
class AstDeferStmt final : public AstStmt {
public:
    AstDeferStmt();

    ~AstDeferStmt();

    AstExpr* expr() const;
    void expr(AstPtr<AstExpr> new_expr);

protected:
    void do_traverse_children(FunctionRef<void(AstNode*)> callback) const override;
    void do_mutate_children(MutableAstVisitor& visitor) override;

private:
    AstPtr<AstExpr> expr_;
};

/// Represents an empty statement.
class AstEmptyStmt final : public AstStmt {
public:
    AstEmptyStmt();

    ~AstEmptyStmt();

protected:
    void do_traverse_children(FunctionRef<void(AstNode*)> callback) const override;
    void do_mutate_children(MutableAstVisitor& visitor) override;
};

/// Represents an error at statement level.
class AstErrorStmt final : public AstStmt {
public:
    AstErrorStmt();

    ~AstErrorStmt();

protected:
    void do_traverse_children(FunctionRef<void(AstNode*)> callback) const override;
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
    void do_traverse_children(FunctionRef<void(AstNode*)> callback) const override;
    void do_mutate_children(MutableAstVisitor& visitor) override;

private:
    AstPtr<AstExpr> expr_;
};

/// Represents a for each loop.
class AstForEachStmt final : public AstStmt {
public:
    AstForEachStmt();

    ~AstForEachStmt();

    AstBindingSpec* spec() const;
    void spec(AstPtr<AstBindingSpec> new_spec);

    AstExpr* expr() const;
    void expr(AstPtr<AstExpr> new_expr);

    AstExpr* body() const;
    void body(AstPtr<AstExpr> new_body);

protected:
    void do_traverse_children(FunctionRef<void(AstNode*)> callback) const override;
    void do_mutate_children(MutableAstVisitor& visitor) override;

private:
    AstPtr<AstBindingSpec> spec_;
    AstPtr<AstExpr> expr_;
    AstPtr<AstExpr> body_;
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
    void do_traverse_children(FunctionRef<void(AstNode*)> callback) const override;
    void do_mutate_children(MutableAstVisitor& visitor) override;

private:
    AstPtr<AstVarDecl> decl_;
    AstPtr<AstExpr> cond_;
    AstPtr<AstExpr> step_;
    AstPtr<AstExpr> body_;
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
    void do_traverse_children(FunctionRef<void(AstNode*)> callback) const override;
    void do_mutate_children(MutableAstVisitor& visitor) override;

private:
    AstPtr<AstExpr> cond_;
    AstPtr<AstExpr> body_;
};
// [[[end]]]

} // namespace tiro

#endif // TIRO_COMPILER_AST_STMT_HPP
