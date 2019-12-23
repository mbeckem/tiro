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

    template<typename Visitor>
    void visit_children(Visitor&& v) {
        Node::visit_children(v);
    }
};

/**
 * A statement that does nothing.
 */
class EmptyStmt final : public Stmt {
public:
    EmptyStmt()
        : Stmt(NodeKind::EmptyStmt) {}

    template<typename Visitor>
    void visit_children(Visitor&& v) {
        Stmt::visit_children(v);
    }

    void dump_impl(NodeFormatter& fmt) const;
};

/**
 * A statement that tests a condition and aborts if that condition fails.
 */
class AssertStmt final : public Stmt {
public:
    AssertStmt();
    ~AssertStmt();

    Expr* condition() const;
    void condition(std::unique_ptr<Expr> condition);

    // The message is optional.
    StringLiteral* message() const;
    void message(std::unique_ptr<StringLiteral> message);

    template<typename Visitor>
    void visit_children(Visitor&& v) {
        Stmt::visit_children(v);
        v(condition());
        v(message());
    }

    void dump_impl(NodeFormatter& fmt) const;

private:
    std::unique_ptr<Expr> condition_;
    std::unique_ptr<StringLiteral> message_; // Optional
};

/**
 * Evaluates condition and runs the body until the condition evaluates to false.
 */
class WhileStmt final : public Stmt {
public:
    WhileStmt();
    ~WhileStmt();

    Expr* condition() const;
    void condition(std::unique_ptr<Expr> condition);

    BlockExpr* body() const;
    void body(std::unique_ptr<BlockExpr> body);

    template<typename Visitor>
    void visit_children(Visitor&& v) {
        Stmt::visit_children(v);
        v(condition());
        v(body());
    }

    void dump_impl(NodeFormatter& fmt) const;

private:
    std::unique_ptr<Expr> condition_;
    std::unique_ptr<BlockExpr> body_;
};

/**
 * The classic for loop. The declaration, condition and step nodes are optional; the body is not.
 */
class ForStmt final : public Stmt, public Scope {
public:
    ForStmt();
    ~ForStmt();

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

    template<typename Visitor>
    void visit_children(Visitor&& v) {
        Stmt::visit_children(v);
        v(decl());
        v(condition());
        v(step());
        v(body());
    }

    void dump_impl(NodeFormatter& fmt) const;

private:
    std::unique_ptr<DeclStmt> decl_;
    std::unique_ptr<Expr> condition_;
    std::unique_ptr<Expr> step_;
    std::unique_ptr<BlockExpr> body_;
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
    VarDecl* decl() const;
    void decl(std::unique_ptr<VarDecl> decl);

    template<typename Visitor>
    void visit_children(Visitor&& v) {
        Stmt::visit_children(v);
        v(decl());
    }

    void dump_impl(NodeFormatter& fmt) const;

private:
    std::unique_ptr<VarDecl> declaration_;
};

/**
 * Evaluates an expression. The value of that expression is usually discarded, but 
 * may be used if it is (for example) the last statement in its surrounding block.
 */
class ExprStmt final : public Stmt {
public:
    ExprStmt();
    ~ExprStmt();

    Expr* expr() const;
    void expr(std::unique_ptr<Expr> expr);

    // True if the result of evaluating the expression is used in the program (e.g. by
    // an expression block).
    bool used() const { return used_; }
    void used(bool used) { used_ = used; }

    template<typename Visitor>
    void visit_children(Visitor&& v) {
        Stmt::visit_children(v);
        v(expr());
    }

    void dump_impl(NodeFormatter& fmt) const;

private:
    std::unique_ptr<Expr> expr_;
    bool used_ = false;
};

} // namespace hammer::ast

#endif // HAMMER_AST_STMT_HPP
