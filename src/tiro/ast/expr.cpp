#include "tiro/ast/expr.hpp"

namespace tiro {

/* [[[cog
    from codegen.ast import NODE_TYPES, implement, walk_types
    
    node_types = list(walk_types(NODE_TYPES.get("Expr")))
    implement(*node_types, NODE_TYPES.get("MapItem"))
]]] */
AstExpr::AstExpr(AstNodeType type)
    : AstNode(type) {
    TIRO_DEBUG_ASSERT(
        type >= AstNodeType::FirstExpr && type <= AstNodeType::LastExpr,
        "Derived type is invalid for this base class.");
}

AstExpr::~AstExpr() = default;

AstBinaryExpr::AstBinaryExpr()
    : AstExpr(AstNodeType::BinaryExpr) {}

AstBinaryExpr::~AstBinaryExpr() = default;

BinaryOperator AstBinaryExpr::operation() const {
    return operation_;
}

void AstBinaryExpr::operation(BinaryOperator new_operation) {
    operation_ = std::move(new_operation);
}

AstExpr* AstBinaryExpr::left() const {
    return left_.get();
}

void AstBinaryExpr::left(AstPtr<AstExpr> new_left) {
    left_ = std::move(new_left);
}

AstExpr* AstBinaryExpr::right() const {
    return right_.get();
}

void AstBinaryExpr::right(AstPtr<AstExpr> new_right) {
    right_ = std::move(new_right);
}

AstBlockExpr::AstBlockExpr()
    : AstExpr(AstNodeType::BlockExpr) {}

AstBlockExpr::~AstBlockExpr() = default;

const AstNodeList<AstStmt>& AstBlockExpr::stmts() const {
    return stmts_;
}

void AstBlockExpr::stmts(AstNodeList<AstStmt> new_stmts) {
    stmts_ = std::move(new_stmts);
}

AstBreakExpr::AstBreakExpr()
    : AstExpr(AstNodeType::BreakExpr) {}

AstBreakExpr::~AstBreakExpr() = default;

AstCallExpr::AstCallExpr()
    : AstExpr(AstNodeType::CallExpr) {}

AstCallExpr::~AstCallExpr() = default;

AccessType AstCallExpr::access_type() const {
    return access_type_;
}

void AstCallExpr::access_type(AccessType new_access_type) {
    access_type_ = std::move(new_access_type);
}

AstExpr* AstCallExpr::func() const {
    return func_.get();
}

void AstCallExpr::func(AstPtr<AstExpr> new_func) {
    func_ = std::move(new_func);
}

const AstNodeList<AstExpr>& AstCallExpr::args() const {
    return args_;
}

void AstCallExpr::args(AstNodeList<AstExpr> new_args) {
    args_ = std::move(new_args);
}

AstContinueExpr::AstContinueExpr()
    : AstExpr(AstNodeType::ContinueExpr) {}

AstContinueExpr::~AstContinueExpr() = default;

AstElementExpr::AstElementExpr()
    : AstExpr(AstNodeType::ElementExpr) {}

AstElementExpr::~AstElementExpr() = default;

AccessType AstElementExpr::access_type() const {
    return access_type_;
}

void AstElementExpr::access_type(AccessType new_access_type) {
    access_type_ = std::move(new_access_type);
}

AstExpr* AstElementExpr::instance() const {
    return instance_.get();
}

void AstElementExpr::instance(AstPtr<AstExpr> new_instance) {
    instance_ = std::move(new_instance);
}

AstExpr* AstElementExpr::element() const {
    return element_.get();
}

void AstElementExpr::element(AstPtr<AstExpr> new_element) {
    element_ = std::move(new_element);
}

AstFuncExpr::AstFuncExpr()
    : AstExpr(AstNodeType::FuncExpr) {}

AstFuncExpr::~AstFuncExpr() = default;

AstFuncDecl* AstFuncExpr::decl() const {
    return decl_.get();
}

void AstFuncExpr::decl(AstPtr<AstFuncDecl> new_decl) {
    decl_ = std::move(new_decl);
}

AstIfExpr::AstIfExpr()
    : AstExpr(AstNodeType::IfExpr) {}

AstIfExpr::~AstIfExpr() = default;

AstExpr* AstIfExpr::cond() const {
    return cond_.get();
}

void AstIfExpr::cond(AstPtr<AstExpr> new_cond) {
    cond_ = std::move(new_cond);
}

AstExpr* AstIfExpr::then_branch() const {
    return then_branch_.get();
}

void AstIfExpr::then_branch(AstPtr<AstExpr> new_then_branch) {
    then_branch_ = std::move(new_then_branch);
}

AstExpr* AstIfExpr::else_branch() const {
    return else_branch_.get();
}

void AstIfExpr::else_branch(AstPtr<AstExpr> new_else_branch) {
    else_branch_ = std::move(new_else_branch);
}

AstLiteral::AstLiteral(AstNodeType type)
    : AstExpr(type) {
    TIRO_DEBUG_ASSERT(
        type >= AstNodeType::FirstLiteral && type <= AstNodeType::LastLiteral,
        "Derived type is invalid for this base class.");
}

AstLiteral::~AstLiteral() = default;

AstArrayLiteral::AstArrayLiteral()
    : AstLiteral(AstNodeType::ArrayLiteral) {}

AstArrayLiteral::~AstArrayLiteral() = default;

const AstNodeList<AstExpr>& AstArrayLiteral::items() const {
    return items_;
}

void AstArrayLiteral::items(AstNodeList<AstExpr> new_items) {
    items_ = std::move(new_items);
}

AstBooleanLiteral::AstBooleanLiteral()
    : AstLiteral(AstNodeType::BooleanLiteral) {}

AstBooleanLiteral::~AstBooleanLiteral() = default;

bool AstBooleanLiteral::value() const {
    return value_;
}

void AstBooleanLiteral::value(bool new_value) {
    value_ = std::move(new_value);
}

AstFloatLiteral::AstFloatLiteral()
    : AstLiteral(AstNodeType::FloatLiteral) {}

AstFloatLiteral::~AstFloatLiteral() = default;

f64 AstFloatLiteral::value() const {
    return value_;
}

void AstFloatLiteral::value(f64 new_value) {
    value_ = std::move(new_value);
}

AstIntegerLiteral::AstIntegerLiteral()
    : AstLiteral(AstNodeType::IntegerLiteral) {}

AstIntegerLiteral::~AstIntegerLiteral() = default;

i64 AstIntegerLiteral::value() const {
    return value_;
}

void AstIntegerLiteral::value(i64 new_value) {
    value_ = std::move(new_value);
}

AstMapLiteral::AstMapLiteral()
    : AstLiteral(AstNodeType::MapLiteral) {}

AstMapLiteral::~AstMapLiteral() = default;

const AstNodeList<AstMapItem>& AstMapLiteral::items() const {
    return items_;
}

void AstMapLiteral::items(AstNodeList<AstMapItem> new_items) {
    items_ = std::move(new_items);
}

AstNullLiteral::AstNullLiteral()
    : AstLiteral(AstNodeType::NullLiteral) {}

AstNullLiteral::~AstNullLiteral() = default;

AstSetLiteral::AstSetLiteral()
    : AstLiteral(AstNodeType::SetLiteral) {}

AstSetLiteral::~AstSetLiteral() = default;

const AstNodeList<AstExpr>& AstSetLiteral::items() const {
    return items_;
}

void AstSetLiteral::items(AstNodeList<AstExpr> new_items) {
    items_ = std::move(new_items);
}

AstStringLiteral::AstStringLiteral()
    : AstLiteral(AstNodeType::StringLiteral) {}

AstStringLiteral::~AstStringLiteral() = default;

InternedString AstStringLiteral::value() const {
    return value_;
}

void AstStringLiteral::value(InternedString new_value) {
    value_ = std::move(new_value);
}

AstSymbolLiteral::AstSymbolLiteral()
    : AstLiteral(AstNodeType::SymbolLiteral) {}

AstSymbolLiteral::~AstSymbolLiteral() = default;

InternedString AstSymbolLiteral::value() const {
    return value_;
}

void AstSymbolLiteral::value(InternedString new_value) {
    value_ = std::move(new_value);
}

AstTupleLiteral::AstTupleLiteral()
    : AstLiteral(AstNodeType::TupleLiteral) {}

AstTupleLiteral::~AstTupleLiteral() = default;

const AstNodeList<AstExpr>& AstTupleLiteral::items() const {
    return items_;
}

void AstTupleLiteral::items(AstNodeList<AstExpr> new_items) {
    items_ = std::move(new_items);
}

AstPropertyExpr::AstPropertyExpr()
    : AstExpr(AstNodeType::PropertyExpr) {}

AstPropertyExpr::~AstPropertyExpr() = default;

AccessType AstPropertyExpr::access_type() const {
    return access_type_;
}

void AstPropertyExpr::access_type(AccessType new_access_type) {
    access_type_ = std::move(new_access_type);
}

AstExpr* AstPropertyExpr::instance() const {
    return instance_.get();
}

void AstPropertyExpr::instance(AstPtr<AstExpr> new_instance) {
    instance_ = std::move(new_instance);
}

const AstProperty& AstPropertyExpr::property() const {
    return property_;
}

void AstPropertyExpr::property(AstProperty new_property) {
    property_ = std::move(new_property);
}

AstReturnExpr::AstReturnExpr()
    : AstExpr(AstNodeType::ReturnExpr) {}

AstReturnExpr::~AstReturnExpr() = default;

AstExpr* AstReturnExpr::value() const {
    return value_.get();
}

void AstReturnExpr::value(AstPtr<AstExpr> new_value) {
    value_ = std::move(new_value);
}

AstStringExpr::AstStringExpr()
    : AstExpr(AstNodeType::StringExpr) {}

AstStringExpr::~AstStringExpr() = default;

const AstNodeList<AstExpr>& AstStringExpr::items() const {
    return items_;
}

void AstStringExpr::items(AstNodeList<AstExpr> new_items) {
    items_ = std::move(new_items);
}

AstUnaryExpr::AstUnaryExpr()
    : AstExpr(AstNodeType::UnaryExpr) {}

AstUnaryExpr::~AstUnaryExpr() = default;

UnaryOperator AstUnaryExpr::operation() const {
    return operation_;
}

void AstUnaryExpr::operation(UnaryOperator new_operation) {
    operation_ = std::move(new_operation);
}

AstExpr* AstUnaryExpr::inner() const {
    return inner_.get();
}

void AstUnaryExpr::inner(AstPtr<AstExpr> new_inner) {
    inner_ = std::move(new_inner);
}

AstVarExpr::AstVarExpr()
    : AstExpr(AstNodeType::VarExpr) {}

AstVarExpr::~AstVarExpr() = default;

InternedString AstVarExpr::name() const {
    return name_;
}

void AstVarExpr::name(InternedString new_name) {
    name_ = std::move(new_name);
}

AstMapItem::AstMapItem()
    : AstNode(AstNodeType::MapItem) {}

AstMapItem::~AstMapItem() = default;

AstExpr* AstMapItem::key() const {
    return key_.get();
}

void AstMapItem::key(AstPtr<AstExpr> new_key) {
    key_ = std::move(new_key);
}

AstExpr* AstMapItem::value() const {
    return value_.get();
}

void AstMapItem::value(AstPtr<AstExpr> new_value) {
    value_ = std::move(new_value);
}

// [[[end]]]

} // namespace tiro