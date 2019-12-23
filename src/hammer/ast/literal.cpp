#include "hammer/ast/literal.hpp"

#include "hammer/ast/decl.hpp"
#include "hammer/ast/node_formatter.hpp"
#include "hammer/core/defs.hpp"

namespace hammer::ast {

void Literal::dump_impl(NodeFormatter& fmt) const {
    Expr::dump_impl(fmt);
}

void NullLiteral::dump_impl(NodeFormatter& fmt) const {
    Literal::dump_impl(fmt);
}

void BooleanLiteral::dump_impl(NodeFormatter& fmt) const {
    Literal::dump_impl(fmt);
    fmt.property("value", value());
}

void IntegerLiteral::dump_impl(NodeFormatter& fmt) const {
    Literal::dump_impl(fmt);
    fmt.property("value", value());
}

void FloatLiteral::dump_impl(NodeFormatter& fmt) const {
    Literal::dump_impl(fmt);
    fmt.property("value", value());
}

void StringLiteral::dump_impl(NodeFormatter& fmt) const {
    Literal::dump_impl(fmt);
    fmt.property("value", value());
}

void SymbolLiteral::dump_impl(NodeFormatter& fmt) const {
    Literal::dump_impl(fmt);
    fmt.property("value", value());
}

void ArrayLiteral::dump_impl(NodeFormatter& fmt) const {
    Literal::dump_impl(fmt);

    fmt.property("entry_count", entry_count());

    for (size_t i = 0; i < entries_.size(); ++i) {
        std::string name = fmt::format("entry_{}", i);
        fmt.property(name, entries_.get(i));
    }
}

void TupleLiteral::dump_impl(NodeFormatter& fmt) const {
    Literal::dump_impl(fmt);

    fmt.property("entry_count", entry_count());

    for (size_t i = 0; i < entries_.size(); ++i) {
        std::string name = fmt::format("entry_{}", i);
        fmt.property(name, entries_.get(i));
    }
}

void MapEntryLiteral::dump_impl(NodeFormatter& fmt) const {
    Literal::dump_impl(fmt);

    fmt.properties("key", key(), "value", value());
}

void MapLiteral::dump_impl(NodeFormatter& fmt) const {
    Literal::dump_impl(fmt);

    fmt.property("entry_count", entry_count());

    for (size_t i = 0; i < entries_.size(); ++i) {
        std::string name = fmt::format("entry_{}", i);
        fmt.properties(name, entries_.get(i));
    }
}

void SetLiteral::dump_impl(NodeFormatter& fmt) const {
    Literal::dump_impl(fmt);

    fmt.property("entry_count", entry_count());

    for (size_t i = 0; i < values_.size(); ++i) {
        std::string name = fmt::format("value_{}", i);
        fmt.property(name, values_.get(i));
    }
}

FuncLiteral::FuncLiteral()
    : Literal(NodeKind::FuncLiteral) {}

FuncLiteral::~FuncLiteral() {}

FuncDecl* FuncLiteral::func() const {
    return func_.get();
}

void FuncLiteral::func(std::unique_ptr<FuncDecl> func) {
    func->parent(this);
    func_ = std::move(func);
}

void FuncLiteral::dump_impl(NodeFormatter& fmt) const {
    Literal::dump_impl(fmt);
    fmt.property("func", func());
}

} // namespace hammer::ast
