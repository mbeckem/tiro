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

size_t ArrayLiteral::entry_count() const {
    return entries_.size();
}

Expr* ArrayLiteral::get_entry(size_t index) const {
    HAMMER_ASSERT(index < entries_.size(), "Index out of bounds.");
    return entries_[index];
}

void ArrayLiteral::add_entry(std::unique_ptr<Expr> entry) {
    HAMMER_ASSERT(entry, "Invalid entry.");
    entries_.push_back(add_child(std::move(entry)));
}

void ArrayLiteral::dump_impl(NodeFormatter& fmt) const {
    Literal::dump_impl(fmt);

    size_t index = 0;
    for (const auto& n : entries_) {
        std::string name = fmt::format("entry_{}", index);
        fmt.property(name, n);
        ++index;
    }
}

size_t TupleLiteral::entry_count() const {
    return entries_.size();
}

Expr* TupleLiteral::get_entry(size_t index) const {
    HAMMER_ASSERT(index < entries_.size(), "Index out of bounds.");
    return entries_[index];
}

void TupleLiteral::add_entry(std::unique_ptr<Expr> entry) {
    HAMMER_ASSERT(entry, "Invalid entry.");
    entries_.push_back(add_child(std::move(entry)));
}

void TupleLiteral::dump_impl(NodeFormatter& fmt) const {
    Literal::dump_impl(fmt);

    size_t index = 0;
    for (const auto& n : entries_) {
        std::string name = fmt::format("entry_{}", index);
        fmt.property(name, n);
        ++index;
    }
}

size_t MapLiteral::entry_count() const {
    return entries_.size();
}

std::pair<Expr*, Expr*> MapLiteral::get_entry(size_t index) const {
    HAMMER_ASSERT(index < entries_.size(), "Index out of bounds.");
    return entries_[index];
}

void MapLiteral::add_entry(
    std::unique_ptr<Expr> key, std::unique_ptr<Expr> value) {
    HAMMER_ASSERT_NOT_NULL(key);
    HAMMER_ASSERT_NOT_NULL(value);
    entries_.emplace_back(
        add_child(std::move(key)), add_child(std::move(value)));
}

void MapLiteral::dump_impl(NodeFormatter& fmt) const {
    Literal::dump_impl(fmt);

    for (const auto& [k, v] : entries_) {
        fmt.properties("key", k, "value", v);
    }
}

void SetLiteral::dump_impl(NodeFormatter& fmt) const {
    Literal::dump_impl(fmt);
}

void FuncLiteral::func(std::unique_ptr<FuncDecl> func) {
    remove_child(func_);
    func_ = add_child(std::move(func));
}

void FuncLiteral::dump_impl(NodeFormatter& fmt) const {
    Literal::dump_impl(fmt);
    fmt.property("func", func());
}

} // namespace hammer::ast
