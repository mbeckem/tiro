#include "hammer/ast/expr.hpp"

#include "hammer/ast/literal.hpp"
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

void Expr::dump_impl(NodeFormatter& fmt) const {
    Node::dump_impl(fmt);
    fmt.properties("type", to_string(expr_type()));
}

BlockExpr::BlockExpr()
    : Expr(NodeKind::BlockExpr)
    , Scope(ScopeKind::BlockScope) {}

BlockExpr::~BlockExpr() {}

Stmt* BlockExpr::get_stmt(size_t index) const {
    return stmts_.get(index);
}

void BlockExpr::add_stmt(std::unique_ptr<Stmt> child) {
    child->parent(this);
    stmts_.push_back(std::move(child));
}

void BlockExpr::dump_impl(NodeFormatter& fmt) const {
    Expr::dump_impl(fmt);

    for (size_t i = 0; i < stmts_.size(); ++i) {
        std::string name = fmt::format("stmt_{}", i);
        fmt.property(name, stmts_.get(i));
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

    for (size_t i = 0; i < args_.size(); ++i) {
        std::string name = fmt::format("arg_{}", i);
        fmt.property(name, args_.get(i));
    }
}

void IndexExpr::dump_impl(NodeFormatter& fmt) const {
    Expr::dump_impl(fmt);
    fmt.properties("inner", inner(), "index", index());
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

StringLiteralList::StringLiteralList()
    : Expr(NodeKind::StringLiteralList) {}

StringLiteralList::~StringLiteralList() {}

StringLiteral* StringLiteralList::get_string(size_t index) const {
    return strings_.get(index);
}

void StringLiteralList::add_string(std::unique_ptr<StringLiteral> child) {
    child->parent(this);
    strings_.push_back(std::move(child));
}

void StringLiteralList::dump_impl(NodeFormatter& fmt) const {
    Expr::dump_impl(fmt);

    for (size_t i = 0; i < strings_.size(); ++i) {
        std::string name = fmt::format("string_{}", i);
        fmt.property(name, strings_.get(i));
    }
}

} // namespace hammer::ast
