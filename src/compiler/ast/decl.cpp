#include "compiler/ast/ast.hpp"

namespace tiro {

/* [[[cog
    from codegen.ast import NODE_TYPES, implement, walk_types

    roots = map(lambda name: NODE_TYPES.get(name), ["Decl", "Binding", "BindingSpec", "Modifier"])
    types = walk_types(*roots)
    implement(*types)
]]] */
AstDecl::AstDecl(AstNodeType type)
    : AstNode(type) {
    TIRO_DEBUG_ASSERT(type >= AstNodeType::FirstDecl && type <= AstNodeType::LastDecl,
        "Derived type is invalid for this base class.");
}

AstDecl::~AstDecl() = default;

AstNodeList<AstModifier>& AstDecl::modifiers() {
    return modifiers_;
}

const AstNodeList<AstModifier>& AstDecl::modifiers() const {
    return modifiers_;
}

void AstDecl::modifiers(AstNodeList<AstModifier> new_modifiers) {
    modifiers_ = std::move(new_modifiers);
}

void AstDecl::do_traverse_children(FunctionRef<void(AstNode*)> callback) const {
    AstNode::do_traverse_children(callback);
    traverse_list(modifiers_, callback);
}

void AstDecl::do_mutate_children(MutableAstVisitor& visitor) {
    AstNode::do_mutate_children(visitor);
    visitor.visit_modifier_list(modifiers_);
}

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

void AstFuncDecl::do_traverse_children(FunctionRef<void(AstNode*)> callback) const {
    AstDecl::do_traverse_children(callback);
    traverse_list(params_, callback);
    callback(body_.get());
}

void AstFuncDecl::do_mutate_children(MutableAstVisitor& visitor) {
    AstDecl::do_mutate_children(visitor);
    visitor.visit_param_decl_list(params_);
    visitor.visit_expr(body_);
}

AstImportDecl::AstImportDecl()
    : AstDecl(AstNodeType::ImportDecl)
    , name_()
    , path_() {}

AstImportDecl::~AstImportDecl() = default;

InternedString AstImportDecl::name() const {
    return name_;
}

void AstImportDecl::name(InternedString new_name) {
    name_ = std::move(new_name);
}

std::vector<InternedString>& AstImportDecl::path() {
    return path_;
}

const std::vector<InternedString>& AstImportDecl::path() const {
    return path_;
}

void AstImportDecl::path(std::vector<InternedString> new_path) {
    path_ = std::move(new_path);
}

void AstImportDecl::do_traverse_children(FunctionRef<void(AstNode*)> callback) const {
    AstDecl::do_traverse_children(callback);
}

void AstImportDecl::do_mutate_children(MutableAstVisitor& visitor) {
    AstDecl::do_mutate_children(visitor);
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

void AstParamDecl::do_traverse_children(FunctionRef<void(AstNode*)> callback) const {
    AstDecl::do_traverse_children(callback);
}

void AstParamDecl::do_mutate_children(MutableAstVisitor& visitor) {
    AstDecl::do_mutate_children(visitor);
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

void AstVarDecl::do_traverse_children(FunctionRef<void(AstNode*)> callback) const {
    AstDecl::do_traverse_children(callback);
    traverse_list(bindings_, callback);
}

void AstVarDecl::do_mutate_children(MutableAstVisitor& visitor) {
    AstDecl::do_mutate_children(visitor);
    visitor.visit_binding_list(bindings_);
}

AstBinding::AstBinding(bool is_const)
    : AstNode(AstNodeType::Binding)
    , is_const_(std::move(is_const))
    , spec_()
    , init_() {}

AstBinding::~AstBinding() = default;

bool AstBinding::is_const() const {
    return is_const_;
}

void AstBinding::is_const(bool new_is_const) {
    is_const_ = std::move(new_is_const);
}

AstBindingSpec* AstBinding::spec() const {
    return spec_.get();
}

void AstBinding::spec(AstPtr<AstBindingSpec> new_spec) {
    spec_ = std::move(new_spec);
}

AstExpr* AstBinding::init() const {
    return init_.get();
}

void AstBinding::init(AstPtr<AstExpr> new_init) {
    init_ = std::move(new_init);
}

void AstBinding::do_traverse_children(FunctionRef<void(AstNode*)> callback) const {
    AstNode::do_traverse_children(callback);
    callback(spec_.get());
    callback(init_.get());
}

void AstBinding::do_mutate_children(MutableAstVisitor& visitor) {
    AstNode::do_mutate_children(visitor);
    visitor.visit_binding_spec(spec_);
    visitor.visit_expr(init_);
}

AstBindingSpec::AstBindingSpec(AstNodeType type)
    : AstNode(type) {
    TIRO_DEBUG_ASSERT(type >= AstNodeType::FirstBindingSpec && type <= AstNodeType::LastBindingSpec,
        "Derived type is invalid for this base class.");
}

AstBindingSpec::~AstBindingSpec() = default;

void AstBindingSpec::do_traverse_children(FunctionRef<void(AstNode*)> callback) const {
    AstNode::do_traverse_children(callback);
}

void AstBindingSpec::do_mutate_children(MutableAstVisitor& visitor) {
    AstNode::do_mutate_children(visitor);
}

AstTupleBindingSpec::AstTupleBindingSpec()
    : AstBindingSpec(AstNodeType::TupleBindingSpec)
    , names_() {}

AstTupleBindingSpec::~AstTupleBindingSpec() = default;

AstNodeList<AstStringIdentifier>& AstTupleBindingSpec::names() {
    return names_;
}

const AstNodeList<AstStringIdentifier>& AstTupleBindingSpec::names() const {
    return names_;
}

void AstTupleBindingSpec::names(AstNodeList<AstStringIdentifier> new_names) {
    names_ = std::move(new_names);
}

void AstTupleBindingSpec::do_traverse_children(FunctionRef<void(AstNode*)> callback) const {
    AstBindingSpec::do_traverse_children(callback);
    traverse_list(names_, callback);
}

void AstTupleBindingSpec::do_mutate_children(MutableAstVisitor& visitor) {
    AstBindingSpec::do_mutate_children(visitor);
    visitor.visit_string_identifier_list(names_);
}

AstVarBindingSpec::AstVarBindingSpec()
    : AstBindingSpec(AstNodeType::VarBindingSpec)
    , name_() {}

AstVarBindingSpec::~AstVarBindingSpec() = default;

AstStringIdentifier* AstVarBindingSpec::name() const {
    return name_.get();
}

void AstVarBindingSpec::name(AstPtr<AstStringIdentifier> new_name) {
    name_ = std::move(new_name);
}

void AstVarBindingSpec::do_traverse_children(FunctionRef<void(AstNode*)> callback) const {
    AstBindingSpec::do_traverse_children(callback);
    callback(name_.get());
}

void AstVarBindingSpec::do_mutate_children(MutableAstVisitor& visitor) {
    AstBindingSpec::do_mutate_children(visitor);
    visitor.visit_string_identifier(name_);
}

AstModifier::AstModifier(AstNodeType type)
    : AstNode(type) {
    TIRO_DEBUG_ASSERT(type >= AstNodeType::FirstModifier && type <= AstNodeType::LastModifier,
        "Derived type is invalid for this base class.");
}

AstModifier::~AstModifier() = default;

void AstModifier::do_traverse_children(FunctionRef<void(AstNode*)> callback) const {
    AstNode::do_traverse_children(callback);
}

void AstModifier::do_mutate_children(MutableAstVisitor& visitor) {
    AstNode::do_mutate_children(visitor);
}

AstExportModifier::AstExportModifier()
    : AstModifier(AstNodeType::ExportModifier) {}

AstExportModifier::~AstExportModifier() = default;

void AstExportModifier::do_traverse_children(FunctionRef<void(AstNode*)> callback) const {
    AstModifier::do_traverse_children(callback);
}

void AstExportModifier::do_mutate_children(MutableAstVisitor& visitor) {
    AstModifier::do_mutate_children(visitor);
}
// [[[end]]]

} // namespace tiro
