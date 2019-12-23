#ifndef HAMMER_AST_EXPR_HPP
#define HAMMER_AST_EXPR_HPP

#include "hammer/ast/node.hpp"
#include "hammer/ast/scope.hpp"
#include "hammer/core/casting.hpp"

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
    void expr_type(ExprType type) { type_ = type; }
    ExprType expr_type() const { return type_; }

    bool can_use_as_value() const {
        return type_ == ExprType::Value || type_ == ExprType::Never;
    }

protected:
    void dump_impl(NodeFormatter& fmt) const;

    template<typename Visitor>
    void visit_children(Visitor&& v) {
        Node::visit_children(v);
    };

private:
    ExprType type_ = ExprType::None;
};

/**
 * A block expression is a sequence of statements. Block expressions can return a value
 * if their last statement is an expression.
 */
class BlockExpr final : public Expr, public Scope {
public:
    BlockExpr();
    ~BlockExpr();

    Stmt* get_stmt(size_t index) const;
    size_t stmt_count() const { return stmts_.size(); }
    void add_stmt(std::unique_ptr<ast::Stmt> item);

    template<typename Visitor>
    void visit_children(Visitor&& v) {
        Expr::visit_children(v);
        stmts_.for_each(v);
    }

    void dump_impl(NodeFormatter& fmt) const;

private:
    NodeVector<Stmt> stmts_;
};

/**
 * The operator used in a unary operation.
 */
enum class UnaryOperator : int {
    // Arithmetic
    Plus,
    Minus,

    // Binary
    BitwiseNot,

    // Boolean
    LogicalNot
};

const char* to_string(UnaryOperator op);

/**
 * A unary operator applied to another expression.
 */
class UnaryExpr final : public Expr {
public:
    explicit UnaryExpr(UnaryOperator op)
        : Expr(NodeKind::UnaryExpr)
        , op_(op) {}

    UnaryOperator operation() const { return op_; }

    Expr* inner() const { return inner_.get(); }
    void inner(std::unique_ptr<Expr> inner) {
        inner->parent(this);
        inner_ = std::move(inner);
    }

    template<typename Visitor>
    void visit_children(Visitor&& v) {
        Expr::visit_children(v);
        v(inner());
    }

    void dump_impl(NodeFormatter& fmt) const;

private:
    UnaryOperator op_;
    std::unique_ptr<Expr> inner_;
};

/**
 * The operator used in a binary operation.
 */
enum class BinaryOperator : int {
    // Arithmetic
    Plus,
    Minus,
    Multiply,
    Divide,
    Modulus,
    Power,

    // Binary
    LeftShift,
    RightShift,
    BitwiseAnd,
    BitwiseOr,
    BitwiseXor,

    // Boolean
    Less,
    LessEquals,
    Greater,
    GreaterEquals,
    Equals,
    NotEquals,
    LogicalAnd,
    LogicalOr,

    // Assigments
    Assign,
};

const char* to_string(BinaryOperator op);

/**
 * A binary operator applied to two other expressions.
 */
class BinaryExpr final : public Expr {
public:
    BinaryExpr(BinaryOperator op)
        : Expr(NodeKind::BinaryExpr)
        , op_(op) {}

    BinaryOperator operation() const { return op_; }

    Expr* left_child() const { return left_.get(); }
    void left_child(std::unique_ptr<Expr> left) {
        left->parent(this);
        left_ = std::move(left);
    }

    Expr* right_child() const { return right_.get(); }
    void right_child(std::unique_ptr<Expr> right) {
        right->parent(this);
        right_ = std::move(right);
    }

    template<typename Visitor>
    void visit_children(Visitor&& v) {
        Expr::visit_children(v);
        v(left_child());
        v(right_child());
    }

    void dump_impl(NodeFormatter& fmt) const;

private:
    BinaryOperator op_;
    std::unique_ptr<Expr> left_ = nullptr;
    std::unique_ptr<Expr> right_ = nullptr;
};

/**
 * References a symbol (variable, function, class) by name.
 */
class VarExpr final : public Expr {
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

    template<typename Visitor>
    void visit_children(Visitor&& v) {
        Expr::visit_children(v);
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
class DotExpr final : public Expr {
public:
    explicit DotExpr()
        : Expr(NodeKind::DotExpr) {}

    Expr* inner() const { return inner_.get(); }
    void inner(std::unique_ptr<Expr> inner) {
        inner->parent(this);
        inner_ = std::move(inner);
    }

    InternedString name() const { return name_; }
    void name(InternedString n) { name_ = n; }

    template<typename Visitor>
    void visit_children(Visitor&& v) {
        Expr::visit_children(v);
        v(inner());
    }

    void dump_impl(NodeFormatter& fmt) const;

private:
    std::unique_ptr<Expr> inner_ = nullptr;
    InternedString name_;
};

/**
 * Calls an expression as a function.
 */
class CallExpr final : public Expr {
public:
    CallExpr()
        : Expr(NodeKind::CallExpr) {}

    // The expression to call as a function
    Expr* func() const { return func_.get(); }
    void func(std::unique_ptr<Expr> func) {
        func->parent(this);
        func_ = std::move(func);
    }

    Expr* get_arg(size_t index) const { return args_.get(index); }

    size_t arg_count() const { return args_.size(); }

    void add_arg(std::unique_ptr<Expr> arg) {
        arg->parent(this);
        args_.push_back(std::move(arg));
    }

    template<typename Visitor>
    void visit_children(Visitor&& v) {
        Expr::visit_children(v);
        v(func());
        args_.for_each(v);
    }

    void dump_impl(NodeFormatter& fmt) const;

private:
    std::unique_ptr<Expr> func_ = nullptr;
    NodeVector<Expr> args_;
};

/**
 * Indexes into another expression, e.g. array[INDEX]
 */
class IndexExpr final : public Expr {
public:
    IndexExpr()
        : Expr(NodeKind::IndexExpr) {}

    Expr* inner() const { return inner_.get(); }
    void inner(std::unique_ptr<Expr> inner) {
        inner->parent(this);
        inner_ = std::move(inner);
    }

    // We could allow multiple index expressions here, i.e. array[index1, index2]
    Expr* index() const { return index_.get(); }
    void index(std::unique_ptr<Expr> index) {
        index->parent(this);
        index_ = std::move(index);
    }

    template<typename Visitor>
    void visit_children(Visitor&& v) {
        Expr::visit_children(v);
        v(inner());
        v(index());
    }

    void dump_impl(NodeFormatter& fmt) const;

private:
    std::unique_ptr<Expr> inner_ = nullptr;
    std::unique_ptr<Expr> index_ = nullptr;
};

/**
 * Evaluates an expression as boolean condition and then either
 * executes the "then" branch or the "else" branch. The else branch is optional.
 * An if expression can return a value if it has both branches and both return a value.
 */
class IfExpr final : public Expr {
public:
    IfExpr()
        : Expr(NodeKind::IfExpr) {}

    Expr* condition() const { return condition_.get(); }
    void condition(std::unique_ptr<Expr> condition) {
        condition->parent(this);
        condition_ = std::move(condition);
    }

    BlockExpr* then_branch() const { return then_branch_.get(); }
    void then_branch(std::unique_ptr<BlockExpr> then_branch) {
        then_branch->parent(this);
        then_branch_ = std::move(then_branch);
    }

    // Must be either another if_expr or a block_expr
    Expr* else_branch() const { return else_branch_.get(); }
    void else_branch(std::unique_ptr<Expr> else_branch) {
        HAMMER_ASSERT(
            isa<BlockExpr>(else_branch.get()) || isa<IfExpr>(else_branch.get()),
            "Else branch must be a block expr or a if expr.");
        else_branch->parent(this);
        else_branch_ = std::move(else_branch);
    }

    template<typename Visitor>
    void visit_children(Visitor&& v) {
        Expr::visit_children(v);
        v(condition());
        v(then_branch());
        v(else_branch());
    }

    void dump_impl(NodeFormatter& fmt) const;

private:
    std::unique_ptr<Expr> condition_ = nullptr;
    std::unique_ptr<BlockExpr> then_branch_ = nullptr;
    std::unique_ptr<Expr> else_branch_ = nullptr;
};

/**
 * Returns the value of an expression from the surrounding function.
 * The expression is optional.
 */
class ReturnExpr final : public Expr {
public:
    ReturnExpr()
        : Expr(NodeKind::ReturnExpr) {}

    // Optional
    Expr* inner() const { return inner_.get(); }
    void inner(std::unique_ptr<Expr> inner) {
        inner->parent(this);
        inner_ = std::move(inner);
    }

    template<typename Visitor>
    void visit_children(Visitor&& v) {
        Expr::visit_children(v);
        v(inner());
    }

    void dump_impl(NodeFormatter& fmt) const;

private:
    std::unique_ptr<Expr> inner_ = nullptr;
};

/**
 * Jumps to the next iteration of the surrounding loop. 
 * 
 * TODO: labeled loops
 */
class ContinueExpr final : public Expr {
public:
    ContinueExpr()
        : Expr(NodeKind::ContinueExpr) {}

    template<typename Visitor>
    void visit_children(Visitor&& v) {
        Expr::visit_children(v);
    }

    void dump_impl(NodeFormatter& fmt) const;
};

/**
 * Stops the execution of the surrounding loop.
 *
 * TODO: labeled loops
 */
class BreakExpr final : public Expr {
public:
    BreakExpr()
        : Expr(NodeKind::BreakExpr) {}

    template<typename Visitor>
    void visit_children(Visitor&& v) {
        Expr::visit_children(v);
    }

    void dump_impl(NodeFormatter& fmt) const;
};

/**
 * Represents a sequence of string literals.
 */
class StringLiteralList final : public Expr {
public:
    StringLiteralList();
    ~StringLiteralList();

    StringLiteral* get_string(size_t index) const;

    size_t string_count() const { return strings_.size(); }

    void add_string(std::unique_ptr<ast::StringLiteral> item);

    template<typename Visitor>
    void visit_children(Visitor&& v) {
        Expr::visit_children(v);
        strings_.for_each(v);
    }

    void dump_impl(NodeFormatter& fmt) const;

private:
    NodeVector<StringLiteral> strings_;
};

} // namespace hammer::ast

#endif // HAMMER_AST_EXPR_HPP
