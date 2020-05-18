#include "tiro/ast/ast.hpp"

namespace tiro {

/* [[[cog
    from codegen.ast import NODE_TYPES, implement, walk_types
    
    decl_types = list(walk_types(NODE_TYPES.get("Decl")))
    binding_types = list(walk_types(NODE_TYPES.get("Binding")))
    implement(*decl_types, *binding_types)
]]] */
AstDecl::AstDecl(AstNodeType type)
    : AstNode(type) {
    TIRO_DEBUG_ASSERT(
        type >= AstNodeType::FirstDecl && type <= AstNodeType::LastDecl,
        "Derived type is invalid for this base class.");
}

AstDecl::~AstDecl() = default;

AstFuncDecl::AstFuncDecl()
    : AstDecl(AstNodeType::FuncDecl)
    , name_()
    , body_is_value_()
    , params_()
    , body_() {}

AstFuncDecl::~AstFuncDecl() = default;

InternedString AstFuncDecl::name() const {
    return name_;
}

void AstFuncDecl::name(InternedString new_name) {
    name_ = std::move(new_name);
}

bool AstFuncDecl::body_is_value() const {
    return body_is_value_;
}

void AstFuncDecl::body_is_value(bool new_body_is_value) {
    body_is_value_ = std::move(new_body_is_value);
}

AstNodeList<AstParamDecl>& AstFuncDecl::params() {
    return params_;
}

const AstNodeList<AstParamDecl>& AstFuncDecl::params() const {
    return params_;
}

void AstFuncDecl::params(AstNodeList<AstParamDecl> new_params) {
    params_ = std::move(new_params);
}

AstExpr* AstFuncDecl::body() const {
    return body_.get();
}

void AstFuncDecl::body(AstPtr<AstExpr> new_body) {
    body_ = std::move(new_body);
}

AstParamDecl::AstParamDecl()
    : AstDecl(AstNodeType::ParamDecl)
    , name_() {}

AstParamDecl::~AstParamDecl() = default;

InternedString AstParamDecl::name() const {
    return name_;
}

void AstParamDecl::name(InternedString new_name) {
    name_ = std::move(new_name);
}

AstVarDecl::AstVarDecl()
    : AstDecl(AstNodeType::VarDecl)
    , bindings_() {}

AstVarDecl::~AstVarDecl() = default;

AstNodeList<AstBinding>& AstVarDecl::bindings() {
    return bindings_;
}

const AstNodeList<AstBinding>& AstVarDecl::bindings() const {
    return bindings_;
}

void AstVarDecl::bindings(AstNodeList<AstBinding> new_bindings) {
    bindings_ = std::move(new_bindings);
}

AstBinding::AstBinding(AstNodeType type)
    : AstNode(type) {
    TIRO_DEBUG_ASSERT(
        type >= AstNodeType::FirstBinding && type <= AstNodeType::LastBinding,
        "Derived type is invalid for this base class.");
}

AstBinding::~AstBinding() = default;

bool AstBinding::is_const() const {
    return is_const_;
}

void AstBinding::is_const(bool new_is_const) {
    is_const_ = std::move(new_is_const);
}

AstExpr* AstBinding::init() const {
    return init_.get();
}

void AstBinding::init(AstPtr<AstExpr> new_init) {
    init_ = std::move(new_init);
}

AstTupleBinding::AstTupleBinding()
    : AstBinding(AstNodeType::TupleBinding)
    , names_() {}

AstTupleBinding::~AstTupleBinding() = default;

std::vector<InternedString>& AstTupleBinding::names() {
    return names_;
}

const std::vector<InternedString>& AstTupleBinding::names() const {
    return names_;
}

void AstTupleBinding::names(std::vector<InternedString> new_names) {
    names_ = std::move(new_names);
}

AstVarBinding::AstVarBinding()
    : AstBinding(AstNodeType::VarBinding)
    , name_() {}

AstVarBinding::~AstVarBinding() = default;

InternedString AstVarBinding::name() const {
    return name_;
}

void AstVarBinding::name(InternedString new_name) {
    name_ = std::move(new_name);
}

// [[[end]]]

} // namespace tiro
