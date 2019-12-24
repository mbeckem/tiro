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
    return initializer_.get();
}

void VarDecl::initializer(std::unique_ptr<Expr> init) {
    init->parent(this);
    initializer_ = std::move(init);
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
    param->parent(this);
    params_.push_back(std::move(param));
}

ParamDecl* FuncDecl::get_param(size_t index) const {
    return params_.get(index);
}

BlockExpr* FuncDecl::body() const {
    return body_.get();
}

void FuncDecl::body(std::unique_ptr<BlockExpr> body) {
    body->parent(this);
    body_ = std::move(body);
}

void FuncDecl::dump_impl(NodeFormatter& fmt) const {
    Decl::dump_impl(fmt);

    for (size_t i = 0; i < params_.size(); ++i) {
        std::string name = fmt::format("param_{}", i);
        fmt.property(name, get_param(i));
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

    for (size_t i = 0; i < path_.size(); ++i) {
        std::string name = fmt::format("path_element_{}", i);
        fmt.property(name, get_path_element(i));
    }
}

} // namespace hammer::ast
