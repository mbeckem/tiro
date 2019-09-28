#include "hammer/ast/expr.hpp"

#include "hammer/ast/node_formatter.hpp"
#include "hammer/ast/stmt.hpp"

#include <fmt/format.h>

namespace hammer::ast {

std::string_view to_string(ExprType type) {
    switch (type) {
    case ExprType::None:
        return "None";
    case ExprType::Never:
        return "Never";
    case ExprType::Value:
        return "Value";
    }

    HAMMER_UNREACHABLE("Invalid expression type.");
}

BlockExpr::BlockExpr()
    : Expr(NodeKind::BlockExpr)
    , Scope(ScopeKind::BlockScope) {}

BlockExpr::~BlockExpr() {}

void Expr::dump_impl(NodeFormatter& fmt) const {
    Node::dump_impl(fmt);
    fmt.properties("type", to_string(expr_type()));
}

void BlockExpr::add_stmt(std::unique_ptr<Stmt> child) {
    nodes_.push_back(add_child(std::move(child)));
}

void BlockExpr::dump_impl(NodeFormatter& fmt) const {
    Expr::dump_impl(fmt);

    size_t index = 0;
    for (const auto& n : nodes_) {
        std::string name = fmt::format("stmt_{}", index);
        fmt.property(name, n);
        ++index;
    }
}

void UnaryExpr::dump_impl(NodeFormatter& fmt) const {
    Expr::dump_impl(fmt);
    fmt.properties("op", to_string(operation()), "inner", inner());
}

void BinaryExpr::dump_impl(NodeFormatter& fmt) const {
    Expr::dump_impl(fmt);
    fmt.properties("op", to_string(operation()), "left_child", left_child(),
        "right_child", right_child());
}

void VarExpr::dump_impl(NodeFormatter& fmt) const {
    Expr::dump_impl(fmt);
    fmt.properties("name", name());
}

void DotExpr::dump_impl(NodeFormatter& fmt) const {
    Expr::dump_impl(fmt);
    fmt.properties("name", name(), "expression", inner());
}

void CallExpr::dump_impl(NodeFormatter& fmt) const {
    Expr::dump_impl(fmt);
    fmt.properties("func", func());

    size_t index = 0;
    for (const auto& a : args_) {
        std::string name = fmt::format("arg_{}", index);
        fmt.property(name, a);
        ++index;
    }
}

void IndexExpr::dump_impl(NodeFormatter& fmt) const {
    Expr::dump_impl(fmt);
    fmt.properties("inner", inner(), "index", index());
}

Expr* IfExpr::condition() const {
    return condition_;
}

void IfExpr::condition(std::unique_ptr<Expr> condition) {
    remove_child(condition_);
    condition_ = add_child(std::move(condition));
}

BlockExpr* IfExpr::then_branch() const {
    return then_branch_;
}

void IfExpr::then_branch(std::unique_ptr<BlockExpr> statement) {
    remove_child(then_branch_);
    then_branch_ = add_child(std::move(statement));
}

Expr* IfExpr::else_branch() const {
    return else_branch_;
}

void IfExpr::else_branch(std::unique_ptr<Expr> statement) {
    remove_child(else_branch_);
    else_branch_ = add_child(std::move(statement));
}

void IfExpr::dump_impl(NodeFormatter& fmt) const {
    Expr::dump_impl(fmt);
    fmt.properties("condition", condition(), "then_statement", then_branch(),
        "else_statement", else_branch());
}

void ReturnExpr::dump_impl(NodeFormatter& fmt) const {
    Expr::dump_impl(fmt);
    fmt.properties("inner", inner());
}

void ContinueExpr::dump_impl(NodeFormatter& fmt) const {
    Expr::dump_impl(fmt);
}

void BreakExpr::dump_impl(NodeFormatter& fmt) const {
    Expr::dump_impl(fmt);
}

const char* to_string(UnaryOperator op) {
#define HAMMER_CASE(op)     \
    case UnaryOperator::op: \
        return #op;

    switch (op) {
        HAMMER_CASE(Plus)
        HAMMER_CASE(Minus)
        HAMMER_CASE(BitwiseNot)
        HAMMER_CASE(LogicalNot)
    }

#undef HAMMER_CASE

    HAMMER_UNREACHABLE("Invalid unary operation.");
}

const char* to_string(BinaryOperator op) {
#define HAMMER_CASE(op)      \
    case BinaryOperator::op: \
        return #op;

    switch (op) {
        HAMMER_CASE(Plus)
        HAMMER_CASE(Minus)
        HAMMER_CASE(Multiply)
        HAMMER_CASE(Divide)
        HAMMER_CASE(Modulus)
        HAMMER_CASE(Power)
        HAMMER_CASE(LeftShift)
        HAMMER_CASE(RightShift)
        HAMMER_CASE(BitwiseOr)
        HAMMER_CASE(BitwiseXor)
        HAMMER_CASE(BitwiseAnd)

        HAMMER_CASE(Less)
        HAMMER_CASE(LessEquals)
        HAMMER_CASE(Greater)
        HAMMER_CASE(GreaterEquals)
        HAMMER_CASE(Equals)
        HAMMER_CASE(NotEquals)
        HAMMER_CASE(LogicalAnd)
        HAMMER_CASE(LogicalOr)

        HAMMER_CASE(Assign)
    }

#undef HAMMER_CASE

    HAMMER_UNREACHABLE("Invalid binary operation.");
}

int operator_precedence(BinaryOperator op) {
    switch (op) {
    case BinaryOperator::Assign:
        return 0;

    case BinaryOperator::LogicalOr:
        return 1;

    case BinaryOperator::LogicalAnd:
        return 2;

    case BinaryOperator::BitwiseOr:
        return 3;

    case BinaryOperator::BitwiseXor:
        return 4;

    case BinaryOperator::BitwiseAnd:
        return 5;

    // TODO Reconsider precendence of equality: should it be lower than Bitwise xor/or/and?
    case BinaryOperator::Equals:
    case BinaryOperator::NotEquals:
        return 6;

    case BinaryOperator::Less:
    case BinaryOperator::LessEquals:
    case BinaryOperator::Greater:
    case BinaryOperator::GreaterEquals:
        return 7;

    case BinaryOperator::LeftShift:
    case BinaryOperator::RightShift:
        return 8;

    case BinaryOperator::Plus:
    case BinaryOperator::Minus:
        return 9;

    case BinaryOperator::Multiply:
    case BinaryOperator::Divide:
    case BinaryOperator::Modulus:
        return 10;

    case BinaryOperator::Power:
        return 11;
    }

    HAMMER_UNREACHABLE("Invalid binary operation type.");
}

bool operator_is_right_associative(BinaryOperator op) {
    switch (op) {
    case BinaryOperator::Assign:
    case BinaryOperator::Power:
        return true;
    default:
        return false;
    }
}

} // namespace hammer::ast
