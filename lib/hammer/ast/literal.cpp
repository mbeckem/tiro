#include "hammer/ast/literal.hpp"

#include "hammer/ast/decl.hpp"
#include "hammer/ast/node_formatter.hpp"

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

void ArrayLiteral::dump_impl(NodeFormatter& fmt) const {
    Literal::dump_impl(fmt);
}

void TupleLiteral::dump_impl(NodeFormatter& fmt) const {
    Literal::dump_impl(fmt);
}

void MapLiteral::dump_impl(NodeFormatter& fmt) const {
    Literal::dump_impl(fmt);
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
