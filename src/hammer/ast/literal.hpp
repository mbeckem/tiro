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
        HAMMER_ASSERT(
            kind >= NodeKind::FirstLiteral && kind <= NodeKind::LastLiteral,
            "Invalid literal kind.");
    }

    void dump_impl(NodeFormatter& fmt) const;
};

/**
 * Literal "null"
 */
class NullLiteral final : public Literal {
public:
    NullLiteral()
        : Literal(NodeKind::NullLiteral) {}

    void dump_impl(NodeFormatter& fmt) const;
};

/**
 * True or false literal.
 */
class BooleanLiteral final : public Literal {
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
class IntegerLiteral final : public Literal {
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
class FloatLiteral final : public Literal {
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
class StringLiteral final : public Literal {
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
 * A literal symbol value, e.g. #red.
 */
class SymbolLiteral final : public Literal {
public:
    SymbolLiteral(InternedString value)
        : Literal(NodeKind::SymbolLiteral)
        , value_(value) {}

    // The name of the symbol, without the leading "#".
    InternedString value() const { return value_; }

    void dump_impl(NodeFormatter& fmt) const;

private:
    InternedString value_;
};

/**
 * Represents a literal array in the source code.
 */
class ArrayLiteral final : public Literal {
public:
    ArrayLiteral()
        : Literal(NodeKind::ArrayLiteral) {}

    auto entries() const { return IterRange(entries_.begin(), entries_.end()); }

    size_t entry_count() const;
    Expr* get_entry(size_t index) const;
    void add_entry(std::unique_ptr<Expr> entry);

    void dump_impl(NodeFormatter& fmt) const;

private:
    std::vector<Expr*> entries_;
};

/**
 * Represents a literal tuple in the source code.
 */
class TupleLiteral final : public Literal {
public:
    TupleLiteral()
        : Literal(NodeKind::TupleLiteral) {}

    auto entries() const { return IterRange(entries_.begin(), entries_.end()); }

    size_t entry_count() const;
    Expr* get_entry(size_t index) const;
    void add_entry(std::unique_ptr<Expr> entry);

    void dump_impl(NodeFormatter& fmt) const;

private:
    std::vector<Expr*> entries_;
};

/**
 * Represents a literal map in the source code.
 */
class MapLiteral final : public Literal {
public:
    MapLiteral()
        : Literal(NodeKind::MapLiteral) {}

    auto entries() const { return IterRange(entries_.begin(), entries_.end()); }

    size_t entry_count() const;
    std::pair<Expr*, Expr*> get_entry(size_t index) const;
    void add_entry(std::unique_ptr<Expr> key, std::unique_ptr<Expr> value);

    void dump_impl(NodeFormatter& fmt) const;

private:
    std::vector<std::pair<Expr*, Expr*>> entries_;
};

/**
 * Represents a literal set in the source code.
 */
class SetLiteral final : public Literal {
public:
    SetLiteral()
        : Literal(NodeKind::SetLiteral) {}

    auto entries() const { return IterRange(entries_.begin(), entries_.end()); }

    size_t entry_count() const;
    Expr* get_entry(size_t index) const;
    void add_entry(std::unique_ptr<Expr> value);

    void dump_impl(NodeFormatter& fmt) const;

private:
    std::vector<Expr*> entries_;
};

/**
 * Represents a literal function in the source code.
 */
class FuncLiteral final : public Literal {
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
