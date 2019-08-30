#include "hammer/ast/decl.hpp"

#include "hammer/ast/expr.hpp"
#include "hammer/ast/node_formatter.hpp"

#include <fmt/format.h>

namespace hammer::ast {

Decl::Decl(NodeKind kind)
    : Node(kind) {
    HAMMER_ASSERT(kind >= NodeKind::FirstDecl && kind <= NodeKind::LastDecl,
                  "Invalid node kind for symbol child class.");
}

Decl::~Decl() {}

void Decl::dump_impl(NodeFormatter& fmt) const {
    Node::dump_impl(fmt);
    fmt.properties("name", name(), "captured", captured());
}

VarDecl::VarDecl()
    : Decl(NodeKind::VarDecl) {}

VarDecl::~VarDecl() {}

Expr* VarDecl::initializer() const {
    return initializer_;
}

void VarDecl::initializer(std::unique_ptr<Expr> value) {
    remove_child(initializer_);
    initializer_ = add_child(std::move(value));
}

void VarDecl::dump_impl(NodeFormatter& fmt) const {
    Decl::dump_impl(fmt);
    fmt.properties("is_const", is_const(), "initializer", initializer());
}

FuncDecl::FuncDecl()
    : Decl(NodeKind::FuncDecl)
    , Scope(ScopeKind::ParameterScope) {}

FuncDecl::~FuncDecl() {}

void FuncDecl::add_param(std::unique_ptr<ParamDecl> param) {
    params_.push_back(add_child(std::move(param)));
}

BlockExpr* FuncDecl::body() const {
    return body_;
}

void FuncDecl::body(std::unique_ptr<BlockExpr> block) {
    remove_child(body_);
    body_ = add_child(std::move(block));
}

void FuncDecl::dump_impl(NodeFormatter& fmt) const {
    Decl::dump_impl(fmt);

    size_t index = 0;
    for (const auto& p : params_) {
        std::string name = fmt::format("param_{}", index);
        fmt.property(name, p); // TODO new class for param decl
        ++index;
    }

    fmt.properties("body", body());
}

ParamDecl::ParamDecl()
    : Decl(NodeKind::ParamDecl) {}

ParamDecl::~ParamDecl() {}

void ParamDecl::dump_impl(NodeFormatter& fmt) const {
    Decl::dump_impl(fmt);
}

ImportDecl::ImportDecl()
    : Decl(NodeKind::ImportDecl) {}

ImportDecl::~ImportDecl() {}

void ImportDecl::dump_impl(NodeFormatter& fmt) const {
    Decl::dump_impl(fmt);
}

} // namespace hammer::ast
