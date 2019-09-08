#ifndef HAMMER_AST_EXPR_HPP
#define HAMMER_AST_EXPR_HPP

#include "hammer/ast/node.hpp"
#include "hammer/ast/operators.hpp"
#include "hammer/ast/scope.hpp"

namespace hammer::ast {

class Scope;

/**
 * Represents the kind of value produced by an expression.
 * Types are computed by the analyzer.
 */
enum class ExprType : u8 {
    None,  ///< Never produces a value.
    Never, ///< Never returns normally; convertible to Value.
    Value, ///< Produces a value.
};

std::string_view to_string(ExprType type);

/**
 * Base class for all expression types. All expressions may return a value.
 */
class Expr : public Node {
protected:
    explicit Expr(NodeKind kind)
        : Node(kind) {
        HAMMER_ASSERT(kind >= NodeKind::FirstExpr && kind <= NodeKind::LastExpr,
            "Invalid expression kind.");
    }

public:
    void type(ExprType type) { type_ = type; }
    ExprType type() const { return type_; }

    bool can_use_as_value() const {
        return type_ == ExprType::Value || type_ == ExprType::Never;
    }

protected:
    void dump_impl(NodeFormatter& fmt) const;

private:
    ExprType type_ = ExprType::None;
};

/**
 * A block expression is a sequence of statements. Block expressions can return a value
 * if their last statement is an expression.
 */
class BlockExpr : public Expr, public Scope {
public:
    BlockExpr();
    ~BlockExpr();

    Stmt* get_stmt(size_t index) const {
        HAMMER_ASSERT(index < nodes_.size(), "Index out of bounds.");
        return nodes_[index];
    }

    size_t stmt_count() const { return nodes_.size(); }

    void add_stmt(std::unique_ptr<ast::Stmt> item);

    void dump_impl(NodeFormatter& fmt) const;

private:
    std::vector<Stmt*> nodes_;
};

/**
 * A unary operator applied to another expression.
 */
class UnaryExpr : public Expr {
public:
    explicit UnaryExpr(UnaryOperator op)
        : Expr(NodeKind::UnaryExpr)
        , op_(op) {}

    UnaryOperator operation() const { return op_; }

    Expr* inner() const { return inner_; }
    void inner(std::unique_ptr<Expr> inner) {
        remove_child(inner_);
        inner_ = add_child(std::move(inner));
    }

    void dump_impl(NodeFormatter& fmt) const;

private:
    UnaryOperator op_;
    Expr* inner_ = nullptr;
};

/**
 * A binary operator applied to two other expressions.
 */
class BinaryExpr : public Expr {
public:
    BinaryExpr(BinaryOperator op)
        : Expr(NodeKind::BinaryExpr)
        , op_(op) {}

    BinaryOperator operation() const { return op_; }

    Expr* left_child() const { return left_; }
    void left_child(std::unique_ptr<Expr> left) {
        remove_child(left_);
        left_ = add_child(std::move(left));
    }

    Expr* right_child() const { return right_; }
    void right_child(std::unique_ptr<Expr> right) {
        remove_child(right_);
        right_ = add_child(std::move(right));
    }

    void dump_impl(NodeFormatter& fmt) const;

private:
    BinaryOperator op_;
    Expr* left_ = nullptr;
    Expr* right_ = nullptr;
};

/**
 * References a symbol (variable, function, class) by name.
 */
class VarExpr : public Expr {
public:
    explicit VarExpr(InternedString name)
        : Expr(NodeKind::VarExpr)
        , name_(name) {}

    InternedString name() const { return name_; }

    // The scope that contains this expression. Does not take ownership.
    Scope* surrounding_scope() const { return surrounding_scope_; }
    void surrounding_scope(Scope* sc) { surrounding_scope_ = sc; }

    // The decl referenced by this expression.
    // The expr does not take ownership of the symbol.
    Decl* decl() const { return decl_; }

    void decl(Decl* d) {
        HAMMER_ASSERT_NOT_NULL(d);
        decl_ = d;
    }

    void dump_impl(NodeFormatter& fmt) const;

private:
    InternedString name_;
    Decl* decl_ = nullptr;
    Scope* surrounding_scope_ = nullptr;
};

/**
 * Member access on another expression, e.g. "EXPR.member".
 */
class DotExpr : public Expr {
public:
    explicit DotExpr()
        : Expr(NodeKind::DotExpr) {}

    Expr* inner() const { return inner_; }
    void inner(std::unique_ptr<Expr> inner) {
        remove_child(inner_);
        inner_ = add_child(std::move(inner));
    }

    InternedString name() const { return name_; }
    void name(InternedString n) { name_ = n; }

    void dump_impl(NodeFormatter& fmt) const;

private:
    Expr* inner_ = nullptr;
    InternedString name_;
};

/**
 * Calls an expression as a function.
 */
class CallExpr : public Expr {
public:
    CallExpr()
        : Expr(NodeKind::CallExpr) {}

    // The expression to call as a function
    Expr* func() const { return func_; }
    void func(std::unique_ptr<Expr> func) {
        remove_child(func_);
        func_ = add_child(std::move(func));
    }

    Expr* get_arg(size_t index) const {
        HAMMER_ASSERT(index < args_.size(), "Index out of bounds.");
        return args_[index];
    }

    size_t arg_count() const { return args_.size(); }

    void add_arg(std::unique_ptr<Expr> arg) {
        Expr* raw_arg = add_child(std::move(arg));
        args_.push_back(raw_arg);
    }

    void dump_impl(NodeFormatter& fmt) const;

private:
    Expr* func_ = nullptr;
    std::vector<Expr*> args_;
};

/**
 * Indexes into another expression, e.g. array[INDEX]
 */
class IndexExpr : public Expr {
public:
    IndexExpr()
        : Expr(NodeKind::IndexExpr) {}

    Expr* inner() const { return inner_; }
    void inner(std::unique_ptr<Expr> inner) {
        remove_child(inner_);
        inner_ = add_child(std::move(inner));
    }

    // We could allow multiple index expressions here, i.e. array[index1, index2]
    Expr* index() const { return index_; }
    void index(std::unique_ptr<Expr> index) {
        remove_child(index_);
        index_ = add_child(std::move(index));
    }

    void dump_impl(NodeFormatter& fmt) const;

private:
    Expr* inner_ = nullptr;
    Expr* index_ = nullptr;
};

/**
 * Evaluates an expression as boolean condition and then either
 * executes the "then" branch or the "else" branch. The else branch is optional.
 * An if expression can return a value if it has both branches and both return a value.
 */
class IfExpr : public Expr {
public:
    IfExpr()
        : Expr(NodeKind::IfExpr) {}

    Expr* condition() const;
    void condition(std::unique_ptr<Expr> condition);

    BlockExpr* then_branch() const;
    void then_branch(std::unique_ptr<BlockExpr> statement);

    // Must be either another if_expr or a block_expr
    Expr* else_branch() const;
    void else_branch(std::unique_ptr<Expr> statement);

    void dump_impl(NodeFormatter& fmt) const;

private:
    Expr* condition_ = nullptr;
    BlockExpr* then_branch_ = nullptr;
    Expr* else_branch_ = nullptr;
};

/**
 * Returns the value of an expression from the surrounding function.
 * The expression is optional.
 */
class ReturnExpr : public Expr {
public:
    ReturnExpr()
        : Expr(NodeKind::ReturnExpr) {}

    // Optional
    Expr* inner() const { return inner_; }
    void inner(std::unique_ptr<Expr> inner) {
        remove_child(inner_);
        inner_ = add_child(std::move(inner));
    }

    void dump_impl(NodeFormatter& fmt) const;

private:
    Expr* inner_ = nullptr;
};

/**
 * Jumps to the next iteration of the surrounding loop. 
 * 
 * TODO: labeled loops
 */
class ContinueExpr : public Expr {
public:
    ContinueExpr()
        : Expr(NodeKind::ContinueExpr) {}

    void dump_impl(NodeFormatter& fmt) const;
};

/**
 * Stops the execution of the surrounding loop.
 *
 * TODO: labeled loops
 */
class BreakExpr : public Expr {
public:
    BreakExpr()
        : Expr(NodeKind::BreakExpr) {}

    void dump_impl(NodeFormatter& fmt) const;
};

} // namespace hammer::ast

#endif // HAMMER_AST_EXPR_HPP
