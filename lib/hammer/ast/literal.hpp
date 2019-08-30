#ifndef HAMMER_AST_LITERAL_HPP
#define HAMMER_AST_LITERAL_HPP

#include "hammer/ast/expr.hpp"

namespace hammer::ast {

/**
 * Literal expressions produce immediate values.
 */
class Literal : public Expr {
protected:
    explicit Literal(NodeKind kind)
        : Expr(kind) {
        HAMMER_ASSERT(kind >= NodeKind::FirstLiteral && kind <= NodeKind::LastLiteral,
                      "Invalid literal kind.");
    }

    void dump_impl(NodeFormatter& fmt) const;
};

/**
 * Literal "null"
 */
class NullLiteral : public Literal {
public:
    NullLiteral()
        : Literal(NodeKind::NullLiteral) {}

    void dump_impl(NodeFormatter& fmt) const;
};

/**
 * True or false literal.
 */
class BooleanLiteral : public Literal {
public:
    BooleanLiteral(bool value)
        : Literal(NodeKind::BooleanLiteral)
        , value_(value) {}

    bool value() const { return value_; }

    void dump_impl(NodeFormatter& fmt) const;

private:
    bool value_;
};

/**
 * A 64 bit integer literal value.
 */
class IntegerLiteral : public Literal {
public:
    explicit IntegerLiteral(i64 value)
        : Literal(NodeKind::IntegerLiteral)
        , value_(value) {}

    i64 value() const { return value_; }

    void dump_impl(NodeFormatter& fmt) const;

private:
    i64 value_;
};

/**
 * A 64 bit floating point literal value.
 */
class FloatLiteral : public Literal {
public:
    FloatLiteral(double value)
        : Literal(NodeKind::FloatLiteral)
        , value_(value) {}

    double value() const { return value_; }

    void dump_impl(NodeFormatter& fmt) const;

private:
    double value_;
};

/**
 * A literal string value.
 */
class StringLiteral : public Literal {
public:
    StringLiteral(InternedString value)
        : Literal(NodeKind::StringLiteral)
        , value_(value) {}

    InternedString value() const { return value_; }

    void dump_impl(NodeFormatter& fmt) const;

private:
    InternedString value_;
};

/**
 * Represents a literal array in the source code.
 */
class ArrayLiteral : public Literal {
public:
    ArrayLiteral()
        : Literal(NodeKind::ArrayLiteral) {}

    void dump_impl(NodeFormatter& fmt) const;
};

/**
 * Represents a literal tuple in the source code.
 */
class TupleLiteral : public Literal {
public:
    TupleLiteral()
        : Literal(NodeKind::TupleLiteral) {}

    void dump_impl(NodeFormatter& fmt) const;
};

/**
 * Represents a literal map in the source code.
 */
class MapLiteral : public Literal {
public:
    MapLiteral()
        : Literal(NodeKind::MapLiteral) {}

    void dump_impl(NodeFormatter& fmt) const;
};

/**
 * Represents a literal set in the source code.
 */
class SetLiteral : public Literal {
public:
    SetLiteral()
        : Literal(NodeKind::SetLiteral) {}

    void dump_impl(NodeFormatter& fmt) const;
};

/**
 * Represents a literal function in the source code.
 */
class FuncLiteral : public Literal {
public:
    FuncLiteral()
        : Literal(NodeKind::FuncLiteral) {}

    FuncDecl* func() const { return func_; }
    void func(std::unique_ptr<FuncDecl> func);

    void dump_impl(NodeFormatter& fmt) const;

private:
    FuncDecl* func_ = nullptr;
};

} // namespace hammer::ast

#endif // HAMMER_AST_LITERAL_HPP
