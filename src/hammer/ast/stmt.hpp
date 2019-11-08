#ifndef HAMMER_AST_STMT_HPP
#define HAMMER_AST_STMT_HPP

#include "hammer/ast/node.hpp"
#include "hammer/ast/scope.hpp"

namespace hammer::ast {

class VarDecl;

/**
 * Base class for statements, i.e. imperative commands.
 */
class Stmt : public Node {
protected:
    explicit Stmt(NodeKind kind)
        : Node(kind) {
        HAMMER_ASSERT(kind >= NodeKind::FirstStmt && kind <= NodeKind::LastStmt,
            "Invalid statement kind.");
    }
};

/**
 * A statement that does nothing.
 */
class EmptyStmt final : public Stmt {
public:
    EmptyStmt()
        : Stmt(NodeKind::EmptyStmt) {}

    void dump_impl(NodeFormatter& fmt) const;
};

/**
 * A statement that tests a condition and aborts if that condition fails.
 */
class AssertStmt final : public Stmt {
public:
    AssertStmt()
        : Stmt(NodeKind::AssertStmt) {}

    Expr* condition() const { return condition_; }
    void condition(std::unique_ptr<Expr> condition);

    // The message is optional.
    StringLiteral* message() const { return message_; }
    void message(std::unique_ptr<StringLiteral> message);

    void dump_impl(NodeFormatter& fmt) const;

private:
    Expr* condition_ = nullptr;
    StringLiteral* message_ = nullptr; // Optional
};

/**
 * Evaluates condition and runs the body until the condition evaluates to false.
 */
class WhileStmt final : public Stmt {
public:
    WhileStmt()
        : Stmt(NodeKind::WhileStmt) {}

    Expr* condition() const;
    void condition(std::unique_ptr<Expr> condition);

    BlockExpr* body() const;
    void body(std::unique_ptr<BlockExpr> body);

    void dump_impl(NodeFormatter& fmt) const;

private:
    Expr* condition_ = nullptr;
    BlockExpr* body_ = nullptr;
};

/**
 * The classic for loop. The declaration, condition and step nodes are optional; the body is not.
 */
class ForStmt final : public Stmt, public Scope {
public:
    ForStmt()
        : Stmt(NodeKind::ForStmt)
        , Scope(ScopeKind::ForStmtScope) {}

    // Optional
    DeclStmt* decl() const;
    void decl(std::unique_ptr<DeclStmt> decl);

    // Optional
    Expr* condition() const;
    void condition(std::unique_ptr<Expr> condition);

    // Optional
    Expr* step() const;
    void step(std::unique_ptr<Expr> step);

    BlockExpr* body() const;
    void body(std::unique_ptr<BlockExpr> body);

    void dump_impl(NodeFormatter& fmt) const;

private:
    DeclStmt* decl_ = nullptr;
    Expr* condition_ = nullptr;
    Expr* step_ = nullptr;
    BlockExpr* body_ = nullptr;
};

// TODO implement (lookahead)
class ForEachLoop;

/**
 * Node for variable declarations.
 * 
 * TODO: Multiple symbols in the same declaration.
 */
class DeclStmt final : public Stmt {
public:
    DeclStmt();
    ~DeclStmt();

    // TODO multiple declarations
    VarDecl* declaration() const;
    void declaration(std::unique_ptr<VarDecl> Decl);

    void dump_impl(NodeFormatter& fmt) const;

private:
    VarDecl* declaration_ = nullptr;
};

/**
 * Evaluates an expression. The value of that expression is usually discarded, but 
 * may be used if it is (for example) the last statement in its surrounding block.
 */
class ExprStmt final : public Stmt {
public:
    ExprStmt()
        : Stmt(NodeKind::ExprStmt) {}

    Expr* expression() const;
    void expression(std::unique_ptr<Expr> e);

    // True if the result of evaluating the expression is used in the program (e.g. by
    // an expression block).
    bool used() const { return used_; }
    void used(bool used) { used_ = used; }

    void dump_impl(NodeFormatter& fmt) const;

private:
    Expr* expr_ = nullptr;
    bool used_ = false;
};

} // namespace hammer::ast

#endif // HAMMER_AST_STMT_HPP
