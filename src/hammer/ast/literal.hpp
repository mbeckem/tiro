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

    template<typename Visitor>
    void visit_children(Visitor&& v) {
        Expr::visit_children(v);
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

    template<typename Visitor>
    void visit_children(Visitor&& v) {
        Literal::visit_children(v);
    }

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

    template<typename Visitor>
    void visit_children(Visitor&& v) {
        Literal::visit_children(v);
    }

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

    template<typename Visitor>
    void visit_children(Visitor&& v) {
        Literal::visit_children(v);
    }

    void dump_impl(NodeFormatter& fmt) const;

private:
    i64 value_;
};

/**
 * A 64 bit floating point literal value.
 */
class FloatLiteral final : public Literal {
public:
    FloatLiteral(f64 value)
        : Literal(NodeKind::FloatLiteral)
        , value_(value) {}

    f64 value() const { return value_; }

    template<typename Visitor>
    void visit_children(Visitor&& v) {
        Literal::visit_children(v);
    }

    void dump_impl(NodeFormatter& fmt) const;

private:
    f64 value_;
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

    template<typename Visitor>
    void visit_children(Visitor&& v) {
        Literal::visit_children(v);
    }

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

    template<typename Visitor>
    void visit_children(Visitor&& v) {
        Literal::visit_children(v);
    }

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

    auto entries() const { return entries_.entries(); }

    size_t entry_count() const { return entries_.size(); }
    Expr* get_entry(size_t index) const { return entries_.get(index); }
    void add_entry(std::unique_ptr<Expr> entry) {
        entry->parent(this);
        entries_.push_back(std::move(entry));
    }

    template<typename Visitor>
    void visit_children(Visitor&& v) {
        Literal::visit_children(v);
        entries_.for_each(v);
    }

    void dump_impl(NodeFormatter& fmt) const;

private:
    NodeVector<Expr> entries_;
};

/**
 * Represents a literal tuple in the source code.
 */
class TupleLiteral final : public Literal {
public:
    TupleLiteral()
        : Literal(NodeKind::TupleLiteral) {}

    auto entries() const { return entries_.entries(); }

    size_t entry_count() const { return entries_.size(); }
    Expr* get_entry(size_t index) const { return entries_.get(index); }
    void add_entry(std::unique_ptr<Expr> entry) {
        entry->parent(this);
        entries_.push_back(std::move(entry));
    }

    template<typename Visitor>
    void visit_children(Visitor&& v) {
        Literal::visit_children(v);
        entries_.for_each(v);
    }

    void dump_impl(NodeFormatter& fmt) const;

private:
    NodeVector<Expr> entries_;
};

/**
 * Represents an entry in a map literal.
 */
class MapEntryLiteral final : public Literal {
public:
    MapEntryLiteral()
        : Literal(NodeKind::MapEntryLiteral) {}

    Expr* key() const { return key_.get(); }
    void key(std::unique_ptr<Expr> key) {
        key->parent(this);
        key_ = std::move(key);
    }

    Expr* value() const { return value_.get(); }
    void value(std::unique_ptr<Expr> value) {
        value->parent(this);
        value_ = std::move(value);
    }

    template<typename Visitor>
    void visit_children(Visitor&& v) {
        Literal::visit_children(v);
        v(key());
        v(value());
    }

    void dump_impl(NodeFormatter& fmt) const;

private:
    std::unique_ptr<Expr> key_;
    std::unique_ptr<Expr> value_;
};

/**
 * Represents a literal map in the source code.
 */
class MapLiteral final : public Literal {
public:
    MapLiteral()
        : Literal(NodeKind::MapLiteral) {}

    auto entries() const { return entries_.entries(); }

    size_t entry_count() const { return entries_.size(); }

    MapEntryLiteral* get_entry(size_t index) const {
        return entries_.get(index);
    }

    void add_entry(std::unique_ptr<MapEntryLiteral> entry) {
        entry->parent(this);
        entries_.push_back(std::move(entry));
    }

    template<typename Visitor>
    void visit_children(Visitor&& v) {
        Literal::visit_children(v);
        entries_.for_each(v);
    }

    void dump_impl(NodeFormatter& fmt) const;

private:
    NodeVector<MapEntryLiteral> entries_;
};

/**
 * Represents a literal set in the source code.
 */
class SetLiteral final : public Literal {
public:
    SetLiteral()
        : Literal(NodeKind::SetLiteral) {}

    auto entries() const { return values_.entries(); }

    size_t entry_count() const { return values_.size(); }
    Expr* get_entry(size_t index) const { return values_.get(index); }
    void add_entry(std::unique_ptr<Expr> value) {
        value->parent(this);
        values_.push_back(std::move(value));
    }

    template<typename Visitor>
    void visit_children(Visitor&& v) {
        Literal::visit_children(v);
        values_.for_each(v);
    }

    void dump_impl(NodeFormatter& fmt) const;

private:
    NodeVector<Expr> values_;
};

/**
 * Represents a literal function in the source code.
 */
class FuncLiteral final : public Literal {
public:
    FuncLiteral();
    ~FuncLiteral();

    FuncDecl* func() const;
    void func(std::unique_ptr<FuncDecl> func);

    template<typename Visitor>
    void visit_children(Visitor&& v) {
        Literal::visit_children(v);
        v(func());
    }

    void dump_impl(NodeFormatter& fmt) const;

private:
    std::unique_ptr<FuncDecl> func_;
};

} // namespace hammer::ast

#endif // HAMMER_AST_LITERAL_HPP
