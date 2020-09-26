#include "compiler/ast/ast.hpp"

namespace tiro {

std::string_view to_string(UnaryOperator op) {
#define TIRO_CASE(op)       \
    case UnaryOperator::op: \
        return #op;

    switch (op) {
        TIRO_CASE(Plus)
        TIRO_CASE(Minus)
        TIRO_CASE(BitwiseNot)
        TIRO_CASE(LogicalNot)
    }

#undef TIRO_CASE

    TIRO_UNREACHABLE("Invalid unary operation.");
}

std::string_view to_string(BinaryOperator op) {
#define TIRO_CASE(op)        \
    case BinaryOperator::op: \
        return #op;

    switch (op) {
        TIRO_CASE(Plus)
        TIRO_CASE(Minus)
        TIRO_CASE(Multiply)
        TIRO_CASE(Divide)
        TIRO_CASE(Modulus)
        TIRO_CASE(Power)
        TIRO_CASE(LeftShift)
        TIRO_CASE(RightShift)
        TIRO_CASE(BitwiseOr)
        TIRO_CASE(BitwiseXor)
        TIRO_CASE(BitwiseAnd)

        TIRO_CASE(Less)
        TIRO_CASE(LessEquals)
        TIRO_CASE(Greater)
        TIRO_CASE(GreaterEquals)
        TIRO_CASE(Equals)
        TIRO_CASE(NotEquals)
        TIRO_CASE(LogicalAnd)
        TIRO_CASE(LogicalOr)

        TIRO_CASE(NullCoalesce)

        TIRO_CASE(Assign)
        TIRO_CASE(AssignPlus)
        TIRO_CASE(AssignMinus)
        TIRO_CASE(AssignMultiply)
        TIRO_CASE(AssignDivide)
        TIRO_CASE(AssignModulus)
        TIRO_CASE(AssignPower)
    }

#undef TIRO_CASE

    TIRO_UNREACHABLE("Invalid binary operation.");
}

/* [[[cog
    from codegen.ast import NODE_TYPES, implement, walk_types
    
    roots = [NODE_TYPES.get(root) for root in ["Expr", "Identifier", "MapItem", "RecordItem"]]
    node_types = list(walk_types(*roots))
    implement(*node_types)
]]] */
AstExpr::AstExpr(AstNodeType type)
    : AstNode(type) {
    TIRO_DEBUG_ASSERT(type >= AstNodeType::FirstExpr && type <= AstNodeType::LastExpr,
        "Derived type is invalid for this base class.");
}

AstExpr::~AstExpr() = default;

void AstExpr::do_traverse_children(FunctionRef<void(AstNode*)> callback) const {
    AstNode::do_traverse_children(callback);
}

void AstExpr::do_mutate_children(MutableAstVisitor& visitor) {
    AstNode::do_mutate_children(visitor);
}

AstBinaryExpr::AstBinaryExpr(BinaryOperator operation)
    : AstExpr(AstNodeType::BinaryExpr)
    , operation_(std::move(operation))
    , left_()
    , right_() {}

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

void AstBinaryExpr::do_traverse_children(FunctionRef<void(AstNode*)> callback) const {
    AstExpr::do_traverse_children(callback);
    callback(left_.get());
    callback(right_.get());
}

void AstBinaryExpr::do_mutate_children(MutableAstVisitor& visitor) {
    AstExpr::do_mutate_children(visitor);
    visitor.visit_expr(left_);
    visitor.visit_expr(right_);
}

AstBlockExpr::AstBlockExpr()
    : AstExpr(AstNodeType::BlockExpr)
    , stmts_() {}

AstBlockExpr::~AstBlockExpr() = default;

AstNodeList<AstStmt>& AstBlockExpr::stmts() {
    return stmts_;
}

const AstNodeList<AstStmt>& AstBlockExpr::stmts() const {
    return stmts_;
}

void AstBlockExpr::stmts(AstNodeList<AstStmt> new_stmts) {
    stmts_ = std::move(new_stmts);
}

void AstBlockExpr::do_traverse_children(FunctionRef<void(AstNode*)> callback) const {
    AstExpr::do_traverse_children(callback);
    traverse_list(stmts_, callback);
}

void AstBlockExpr::do_mutate_children(MutableAstVisitor& visitor) {
    AstExpr::do_mutate_children(visitor);
    visitor.visit_stmt_list(stmts_);
}

AstBreakExpr::AstBreakExpr()
    : AstExpr(AstNodeType::BreakExpr) {}

AstBreakExpr::~AstBreakExpr() = default;

void AstBreakExpr::do_traverse_children(FunctionRef<void(AstNode*)> callback) const {
    AstExpr::do_traverse_children(callback);
}

void AstBreakExpr::do_mutate_children(MutableAstVisitor& visitor) {
    AstExpr::do_mutate_children(visitor);
}

AstCallExpr::AstCallExpr(AccessType access_type)
    : AstExpr(AstNodeType::CallExpr)
    , access_type_(std::move(access_type))
    , func_()
    , args_() {}

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

AstNodeList<AstExpr>& AstCallExpr::args() {
    return args_;
}

const AstNodeList<AstExpr>& AstCallExpr::args() const {
    return args_;
}

void AstCallExpr::args(AstNodeList<AstExpr> new_args) {
    args_ = std::move(new_args);
}

void AstCallExpr::do_traverse_children(FunctionRef<void(AstNode*)> callback) const {
    AstExpr::do_traverse_children(callback);
    callback(func_.get());
    traverse_list(args_, callback);
}

void AstCallExpr::do_mutate_children(MutableAstVisitor& visitor) {
    AstExpr::do_mutate_children(visitor);
    visitor.visit_expr(func_);
    visitor.visit_expr_list(args_);
}

AstContinueExpr::AstContinueExpr()
    : AstExpr(AstNodeType::ContinueExpr) {}

AstContinueExpr::~AstContinueExpr() = default;

void AstContinueExpr::do_traverse_children(FunctionRef<void(AstNode*)> callback) const {
    AstExpr::do_traverse_children(callback);
}

void AstContinueExpr::do_mutate_children(MutableAstVisitor& visitor) {
    AstExpr::do_mutate_children(visitor);
}

AstElementExpr::AstElementExpr(AccessType access_type)
    : AstExpr(AstNodeType::ElementExpr)
    , access_type_(std::move(access_type))
    , instance_()
    , element_() {}

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

void AstElementExpr::do_traverse_children(FunctionRef<void(AstNode*)> callback) const {
    AstExpr::do_traverse_children(callback);
    callback(instance_.get());
    callback(element_.get());
}

void AstElementExpr::do_mutate_children(MutableAstVisitor& visitor) {
    AstExpr::do_mutate_children(visitor);
    visitor.visit_expr(instance_);
    visitor.visit_expr(element_);
}

AstFuncExpr::AstFuncExpr()
    : AstExpr(AstNodeType::FuncExpr)
    , decl_() {}

AstFuncExpr::~AstFuncExpr() = default;

AstFuncDecl* AstFuncExpr::decl() const {
    return decl_.get();
}

void AstFuncExpr::decl(AstPtr<AstFuncDecl> new_decl) {
    decl_ = std::move(new_decl);
}

void AstFuncExpr::do_traverse_children(FunctionRef<void(AstNode*)> callback) const {
    AstExpr::do_traverse_children(callback);
    callback(decl_.get());
}

void AstFuncExpr::do_mutate_children(MutableAstVisitor& visitor) {
    AstExpr::do_mutate_children(visitor);
    visitor.visit_func_decl(decl_);
}

AstIfExpr::AstIfExpr()
    : AstExpr(AstNodeType::IfExpr)
    , cond_()
    , then_branch_()
    , else_branch_() {}

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

void AstIfExpr::do_traverse_children(FunctionRef<void(AstNode*)> callback) const {
    AstExpr::do_traverse_children(callback);
    callback(cond_.get());
    callback(then_branch_.get());
    callback(else_branch_.get());
}

void AstIfExpr::do_mutate_children(MutableAstVisitor& visitor) {
    AstExpr::do_mutate_children(visitor);
    visitor.visit_expr(cond_);
    visitor.visit_expr(then_branch_);
    visitor.visit_expr(else_branch_);
}

AstLiteral::AstLiteral(AstNodeType type)
    : AstExpr(type) {
    TIRO_DEBUG_ASSERT(type >= AstNodeType::FirstLiteral && type <= AstNodeType::LastLiteral,
        "Derived type is invalid for this base class.");
}

AstLiteral::~AstLiteral() = default;

void AstLiteral::do_traverse_children(FunctionRef<void(AstNode*)> callback) const {
    AstExpr::do_traverse_children(callback);
}

void AstLiteral::do_mutate_children(MutableAstVisitor& visitor) {
    AstExpr::do_mutate_children(visitor);
}

AstArrayLiteral::AstArrayLiteral()
    : AstLiteral(AstNodeType::ArrayLiteral)
    , items_() {}

AstArrayLiteral::~AstArrayLiteral() = default;

AstNodeList<AstExpr>& AstArrayLiteral::items() {
    return items_;
}

const AstNodeList<AstExpr>& AstArrayLiteral::items() const {
    return items_;
}

void AstArrayLiteral::items(AstNodeList<AstExpr> new_items) {
    items_ = std::move(new_items);
}

void AstArrayLiteral::do_traverse_children(FunctionRef<void(AstNode*)> callback) const {
    AstLiteral::do_traverse_children(callback);
    traverse_list(items_, callback);
}

void AstArrayLiteral::do_mutate_children(MutableAstVisitor& visitor) {
    AstLiteral::do_mutate_children(visitor);
    visitor.visit_expr_list(items_);
}

AstBooleanLiteral::AstBooleanLiteral(bool value)
    : AstLiteral(AstNodeType::BooleanLiteral)
    , value_(std::move(value)) {}

AstBooleanLiteral::~AstBooleanLiteral() = default;

bool AstBooleanLiteral::value() const {
    return value_;
}

void AstBooleanLiteral::value(bool new_value) {
    value_ = std::move(new_value);
}

void AstBooleanLiteral::do_traverse_children(FunctionRef<void(AstNode*)> callback) const {
    AstLiteral::do_traverse_children(callback);
}

void AstBooleanLiteral::do_mutate_children(MutableAstVisitor& visitor) {
    AstLiteral::do_mutate_children(visitor);
}

AstFloatLiteral::AstFloatLiteral(f64 value)
    : AstLiteral(AstNodeType::FloatLiteral)
    , value_(std::move(value)) {}

AstFloatLiteral::~AstFloatLiteral() = default;

f64 AstFloatLiteral::value() const {
    return value_;
}

void AstFloatLiteral::value(f64 new_value) {
    value_ = std::move(new_value);
}

void AstFloatLiteral::do_traverse_children(FunctionRef<void(AstNode*)> callback) const {
    AstLiteral::do_traverse_children(callback);
}

void AstFloatLiteral::do_mutate_children(MutableAstVisitor& visitor) {
    AstLiteral::do_mutate_children(visitor);
}

AstIntegerLiteral::AstIntegerLiteral(i64 value)
    : AstLiteral(AstNodeType::IntegerLiteral)
    , value_(std::move(value)) {}

AstIntegerLiteral::~AstIntegerLiteral() = default;

i64 AstIntegerLiteral::value() const {
    return value_;
}

void AstIntegerLiteral::value(i64 new_value) {
    value_ = std::move(new_value);
}

void AstIntegerLiteral::do_traverse_children(FunctionRef<void(AstNode*)> callback) const {
    AstLiteral::do_traverse_children(callback);
}

void AstIntegerLiteral::do_mutate_children(MutableAstVisitor& visitor) {
    AstLiteral::do_mutate_children(visitor);
}

AstMapLiteral::AstMapLiteral()
    : AstLiteral(AstNodeType::MapLiteral)
    , items_() {}

AstMapLiteral::~AstMapLiteral() = default;

AstNodeList<AstMapItem>& AstMapLiteral::items() {
    return items_;
}

const AstNodeList<AstMapItem>& AstMapLiteral::items() const {
    return items_;
}

void AstMapLiteral::items(AstNodeList<AstMapItem> new_items) {
    items_ = std::move(new_items);
}

void AstMapLiteral::do_traverse_children(FunctionRef<void(AstNode*)> callback) const {
    AstLiteral::do_traverse_children(callback);
    traverse_list(items_, callback);
}

void AstMapLiteral::do_mutate_children(MutableAstVisitor& visitor) {
    AstLiteral::do_mutate_children(visitor);
    visitor.visit_map_item_list(items_);
}

AstNullLiteral::AstNullLiteral()
    : AstLiteral(AstNodeType::NullLiteral) {}

AstNullLiteral::~AstNullLiteral() = default;

void AstNullLiteral::do_traverse_children(FunctionRef<void(AstNode*)> callback) const {
    AstLiteral::do_traverse_children(callback);
}

void AstNullLiteral::do_mutate_children(MutableAstVisitor& visitor) {
    AstLiteral::do_mutate_children(visitor);
}

AstRecordLiteral::AstRecordLiteral()
    : AstLiteral(AstNodeType::RecordLiteral)
    , items_() {}

AstRecordLiteral::~AstRecordLiteral() = default;

AstNodeList<AstRecordItem>& AstRecordLiteral::items() {
    return items_;
}

const AstNodeList<AstRecordItem>& AstRecordLiteral::items() const {
    return items_;
}

void AstRecordLiteral::items(AstNodeList<AstRecordItem> new_items) {
    items_ = std::move(new_items);
}

void AstRecordLiteral::do_traverse_children(FunctionRef<void(AstNode*)> callback) const {
    AstLiteral::do_traverse_children(callback);
    traverse_list(items_, callback);
}

void AstRecordLiteral::do_mutate_children(MutableAstVisitor& visitor) {
    AstLiteral::do_mutate_children(visitor);
    visitor.visit_record_item_list(items_);
}

AstSetLiteral::AstSetLiteral()
    : AstLiteral(AstNodeType::SetLiteral)
    , items_() {}

AstSetLiteral::~AstSetLiteral() = default;

AstNodeList<AstExpr>& AstSetLiteral::items() {
    return items_;
}

const AstNodeList<AstExpr>& AstSetLiteral::items() const {
    return items_;
}

void AstSetLiteral::items(AstNodeList<AstExpr> new_items) {
    items_ = std::move(new_items);
}

void AstSetLiteral::do_traverse_children(FunctionRef<void(AstNode*)> callback) const {
    AstLiteral::do_traverse_children(callback);
    traverse_list(items_, callback);
}

void AstSetLiteral::do_mutate_children(MutableAstVisitor& visitor) {
    AstLiteral::do_mutate_children(visitor);
    visitor.visit_expr_list(items_);
}

AstStringLiteral::AstStringLiteral(InternedString value)
    : AstLiteral(AstNodeType::StringLiteral)
    , value_(std::move(value)) {}

AstStringLiteral::~AstStringLiteral() = default;

InternedString AstStringLiteral::value() const {
    return value_;
}

void AstStringLiteral::value(InternedString new_value) {
    value_ = std::move(new_value);
}

void AstStringLiteral::do_traverse_children(FunctionRef<void(AstNode*)> callback) const {
    AstLiteral::do_traverse_children(callback);
}

void AstStringLiteral::do_mutate_children(MutableAstVisitor& visitor) {
    AstLiteral::do_mutate_children(visitor);
}

AstSymbolLiteral::AstSymbolLiteral(InternedString value)
    : AstLiteral(AstNodeType::SymbolLiteral)
    , value_(std::move(value)) {}

AstSymbolLiteral::~AstSymbolLiteral() = default;

InternedString AstSymbolLiteral::value() const {
    return value_;
}

void AstSymbolLiteral::value(InternedString new_value) {
    value_ = std::move(new_value);
}

void AstSymbolLiteral::do_traverse_children(FunctionRef<void(AstNode*)> callback) const {
    AstLiteral::do_traverse_children(callback);
}

void AstSymbolLiteral::do_mutate_children(MutableAstVisitor& visitor) {
    AstLiteral::do_mutate_children(visitor);
}

AstTupleLiteral::AstTupleLiteral()
    : AstLiteral(AstNodeType::TupleLiteral)
    , items_() {}

AstTupleLiteral::~AstTupleLiteral() = default;

AstNodeList<AstExpr>& AstTupleLiteral::items() {
    return items_;
}

const AstNodeList<AstExpr>& AstTupleLiteral::items() const {
    return items_;
}

void AstTupleLiteral::items(AstNodeList<AstExpr> new_items) {
    items_ = std::move(new_items);
}

void AstTupleLiteral::do_traverse_children(FunctionRef<void(AstNode*)> callback) const {
    AstLiteral::do_traverse_children(callback);
    traverse_list(items_, callback);
}

void AstTupleLiteral::do_mutate_children(MutableAstVisitor& visitor) {
    AstLiteral::do_mutate_children(visitor);
    visitor.visit_expr_list(items_);
}

AstPropertyExpr::AstPropertyExpr(AccessType access_type)
    : AstExpr(AstNodeType::PropertyExpr)
    , access_type_(std::move(access_type))
    , instance_()
    , property_() {}

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

AstIdentifier* AstPropertyExpr::property() const {
    return property_.get();
}

void AstPropertyExpr::property(AstPtr<AstIdentifier> new_property) {
    property_ = std::move(new_property);
}

void AstPropertyExpr::do_traverse_children(FunctionRef<void(AstNode*)> callback) const {
    AstExpr::do_traverse_children(callback);
    callback(instance_.get());
    callback(property_.get());
}

void AstPropertyExpr::do_mutate_children(MutableAstVisitor& visitor) {
    AstExpr::do_mutate_children(visitor);
    visitor.visit_expr(instance_);
    visitor.visit_identifier(property_);
}

AstReturnExpr::AstReturnExpr()
    : AstExpr(AstNodeType::ReturnExpr)
    , value_() {}

AstReturnExpr::~AstReturnExpr() = default;

AstExpr* AstReturnExpr::value() const {
    return value_.get();
}

void AstReturnExpr::value(AstPtr<AstExpr> new_value) {
    value_ = std::move(new_value);
}

void AstReturnExpr::do_traverse_children(FunctionRef<void(AstNode*)> callback) const {
    AstExpr::do_traverse_children(callback);
    callback(value_.get());
}

void AstReturnExpr::do_mutate_children(MutableAstVisitor& visitor) {
    AstExpr::do_mutate_children(visitor);
    visitor.visit_expr(value_);
}

AstStringExpr::AstStringExpr()
    : AstExpr(AstNodeType::StringExpr)
    , items_() {}

AstStringExpr::~AstStringExpr() = default;

AstNodeList<AstExpr>& AstStringExpr::items() {
    return items_;
}

const AstNodeList<AstExpr>& AstStringExpr::items() const {
    return items_;
}

void AstStringExpr::items(AstNodeList<AstExpr> new_items) {
    items_ = std::move(new_items);
}

void AstStringExpr::do_traverse_children(FunctionRef<void(AstNode*)> callback) const {
    AstExpr::do_traverse_children(callback);
    traverse_list(items_, callback);
}

void AstStringExpr::do_mutate_children(MutableAstVisitor& visitor) {
    AstExpr::do_mutate_children(visitor);
    visitor.visit_expr_list(items_);
}

AstStringGroupExpr::AstStringGroupExpr()
    : AstExpr(AstNodeType::StringGroupExpr)
    , strings_() {}

AstStringGroupExpr::~AstStringGroupExpr() = default;

AstNodeList<AstStringExpr>& AstStringGroupExpr::strings() {
    return strings_;
}

const AstNodeList<AstStringExpr>& AstStringGroupExpr::strings() const {
    return strings_;
}

void AstStringGroupExpr::strings(AstNodeList<AstStringExpr> new_strings) {
    strings_ = std::move(new_strings);
}

void AstStringGroupExpr::do_traverse_children(FunctionRef<void(AstNode*)> callback) const {
    AstExpr::do_traverse_children(callback);
    traverse_list(strings_, callback);
}

void AstStringGroupExpr::do_mutate_children(MutableAstVisitor& visitor) {
    AstExpr::do_mutate_children(visitor);
    visitor.visit_string_expr_list(strings_);
}

AstUnaryExpr::AstUnaryExpr(UnaryOperator operation)
    : AstExpr(AstNodeType::UnaryExpr)
    , operation_(std::move(operation))
    , inner_() {}

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

void AstUnaryExpr::do_traverse_children(FunctionRef<void(AstNode*)> callback) const {
    AstExpr::do_traverse_children(callback);
    callback(inner_.get());
}

void AstUnaryExpr::do_mutate_children(MutableAstVisitor& visitor) {
    AstExpr::do_mutate_children(visitor);
    visitor.visit_expr(inner_);
}

AstVarExpr::AstVarExpr(InternedString name)
    : AstExpr(AstNodeType::VarExpr)
    , name_(std::move(name)) {}

AstVarExpr::~AstVarExpr() = default;

InternedString AstVarExpr::name() const {
    return name_;
}

void AstVarExpr::name(InternedString new_name) {
    name_ = std::move(new_name);
}

void AstVarExpr::do_traverse_children(FunctionRef<void(AstNode*)> callback) const {
    AstExpr::do_traverse_children(callback);
}

void AstVarExpr::do_mutate_children(MutableAstVisitor& visitor) {
    AstExpr::do_mutate_children(visitor);
}

AstIdentifier::AstIdentifier(AstNodeType type)
    : AstNode(type) {
    TIRO_DEBUG_ASSERT(type >= AstNodeType::FirstIdentifier && type <= AstNodeType::LastIdentifier,
        "Derived type is invalid for this base class.");
}

AstIdentifier::~AstIdentifier() = default;

void AstIdentifier::do_traverse_children(FunctionRef<void(AstNode*)> callback) const {
    AstNode::do_traverse_children(callback);
}

void AstIdentifier::do_mutate_children(MutableAstVisitor& visitor) {
    AstNode::do_mutate_children(visitor);
}

AstNumericIdentifier::AstNumericIdentifier(u32 value)
    : AstIdentifier(AstNodeType::NumericIdentifier)
    , value_(std::move(value)) {}

AstNumericIdentifier::~AstNumericIdentifier() = default;

u32 AstNumericIdentifier::value() const {
    return value_;
}

void AstNumericIdentifier::value(u32 new_value) {
    value_ = std::move(new_value);
}

void AstNumericIdentifier::do_traverse_children(FunctionRef<void(AstNode*)> callback) const {
    AstIdentifier::do_traverse_children(callback);
}

void AstNumericIdentifier::do_mutate_children(MutableAstVisitor& visitor) {
    AstIdentifier::do_mutate_children(visitor);
}

AstStringIdentifier::AstStringIdentifier(InternedString value)
    : AstIdentifier(AstNodeType::StringIdentifier)
    , value_(std::move(value)) {}

AstStringIdentifier::~AstStringIdentifier() = default;

InternedString AstStringIdentifier::value() const {
    return value_;
}

void AstStringIdentifier::value(InternedString new_value) {
    value_ = std::move(new_value);
}

void AstStringIdentifier::do_traverse_children(FunctionRef<void(AstNode*)> callback) const {
    AstIdentifier::do_traverse_children(callback);
}

void AstStringIdentifier::do_mutate_children(MutableAstVisitor& visitor) {
    AstIdentifier::do_mutate_children(visitor);
}

AstMapItem::AstMapItem()
    : AstNode(AstNodeType::MapItem)
    , key_()
    , value_() {}

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

void AstMapItem::do_traverse_children(FunctionRef<void(AstNode*)> callback) const {
    AstNode::do_traverse_children(callback);
    callback(key_.get());
    callback(value_.get());
}

void AstMapItem::do_mutate_children(MutableAstVisitor& visitor) {
    AstNode::do_mutate_children(visitor);
    visitor.visit_expr(key_);
    visitor.visit_expr(value_);
}

AstRecordItem::AstRecordItem()
    : AstNode(AstNodeType::RecordItem)
    , key_()
    , value_() {}

AstRecordItem::~AstRecordItem() = default;

AstStringIdentifier* AstRecordItem::key() const {
    return key_.get();
}

void AstRecordItem::key(AstPtr<AstStringIdentifier> new_key) {
    key_ = std::move(new_key);
}

AstExpr* AstRecordItem::value() const {
    return value_.get();
}

void AstRecordItem::value(AstPtr<AstExpr> new_value) {
    value_ = std::move(new_value);
}

void AstRecordItem::do_traverse_children(FunctionRef<void(AstNode*)> callback) const {
    AstNode::do_traverse_children(callback);
    callback(key_.get());
    callback(value_.get());
}

void AstRecordItem::do_mutate_children(MutableAstVisitor& visitor) {
    AstNode::do_mutate_children(visitor);
    visitor.visit_string_identifier(key_);
    visitor.visit_expr(value_);
}
// [[[end]]]

} // namespace tiro
