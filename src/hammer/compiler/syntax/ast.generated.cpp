// Automatically generated by ./utils/generate-ast at 2020-01-03 21:27:53.662999.
// Do not include this file directly and do not edit by hand!

#include "hammer/compiler/syntax/ast.hpp"

#include <utility>

namespace hammer::compiler {

std::string_view to_string(NodeType type) {
    switch (type) {
    case NodeType::FuncDecl:
        return "FuncDecl";
    case NodeType::ImportDecl:
        return "ImportDecl";
    case NodeType::ParamDecl:
        return "ParamDecl";
    case NodeType::VarDecl:
        return "VarDecl";
    case NodeType::BinaryExpr:
        return "BinaryExpr";
    case NodeType::BlockExpr:
        return "BlockExpr";
    case NodeType::BreakExpr:
        return "BreakExpr";
    case NodeType::CallExpr:
        return "CallExpr";
    case NodeType::ContinueExpr:
        return "ContinueExpr";
    case NodeType::DotExpr:
        return "DotExpr";
    case NodeType::IfExpr:
        return "IfExpr";
    case NodeType::IndexExpr:
        return "IndexExpr";
    case NodeType::ArrayLiteral:
        return "ArrayLiteral";
    case NodeType::BooleanLiteral:
        return "BooleanLiteral";
    case NodeType::FloatLiteral:
        return "FloatLiteral";
    case NodeType::FuncLiteral:
        return "FuncLiteral";
    case NodeType::IntegerLiteral:
        return "IntegerLiteral";
    case NodeType::MapLiteral:
        return "MapLiteral";
    case NodeType::NullLiteral:
        return "NullLiteral";
    case NodeType::SetLiteral:
        return "SetLiteral";
    case NodeType::StringLiteral:
        return "StringLiteral";
    case NodeType::SymbolLiteral:
        return "SymbolLiteral";
    case NodeType::TupleLiteral:
        return "TupleLiteral";
    case NodeType::ReturnExpr:
        return "ReturnExpr";
    case NodeType::StringSequenceExpr:
        return "StringSequenceExpr";
    case NodeType::UnaryExpr:
        return "UnaryExpr";
    case NodeType::VarExpr:
        return "VarExpr";
    case NodeType::ExprList:
        return "ExprList";
    case NodeType::File:
        return "File";
    case NodeType::MapEntry:
        return "MapEntry";
    case NodeType::MapEntryList:
        return "MapEntryList";
    case NodeType::NodeList:
        return "NodeList";
    case NodeType::ParamList:
        return "ParamList";
    case NodeType::Root:
        return "Root";
    case NodeType::AssertStmt:
        return "AssertStmt";
    case NodeType::DeclStmt:
        return "DeclStmt";
    case NodeType::EmptyStmt:
        return "EmptyStmt";
    case NodeType::ExprStmt:
        return "ExprStmt";
    case NodeType::ForStmt:
        return "ForStmt";
    case NodeType::WhileStmt:
        return "WhileStmt";
    case NodeType::StmtList:
        return "StmtList";
    }
    HAMMER_UNREACHABLE("Invalid node type.");
}

Decl::Decl(NodeType child_type)
    : Node(child_type)
    , name_()
    , declared_symbol_() {
    HAMMER_ASSERT(
        child_type >= NodeType::FirstDecl && child_type <= NodeType::LastDecl,
        "Invalid child type.");
}

Decl::~Decl() {}

void Decl::name(const InternedString& value) {
    name_ = value;
}

const InternedString& Decl::name() const {
    return name_;
}

void Decl::declared_symbol(SymbolEntryPtr value) {
    declared_symbol_ = value;
}

SymbolEntryPtr Decl::declared_symbol() const {
    return declared_symbol_.lock();
}

FuncDecl::FuncDecl()
    : Decl(NodeType::FuncDecl)
    , params_()
    , body_()
    , param_scope_()
    , body_scope_() {}

FuncDecl::~FuncDecl() {}

void FuncDecl::params(const NodePtr<ParamList>& value) {
    params_ = value;
}

const NodePtr<ParamList>& FuncDecl::params() const {
    return params_;
}

void FuncDecl::body(const NodePtr<Expr>& value) {
    body_ = value;
}

const NodePtr<Expr>& FuncDecl::body() const {
    return body_;
}

void FuncDecl::param_scope(ScopePtr value) {
    param_scope_ = value;
}

ScopePtr FuncDecl::param_scope() const {
    return param_scope_.lock();
}

void FuncDecl::body_scope(ScopePtr value) {
    body_scope_ = value;
}

ScopePtr FuncDecl::body_scope() const {
    return body_scope_.lock();
}

ImportDecl::ImportDecl()
    : Decl(NodeType::ImportDecl)
    , path_elements_() {}

ImportDecl::~ImportDecl() {}

ParamDecl::ParamDecl()
    : Decl(NodeType::ParamDecl) {}

ParamDecl::~ParamDecl() {}

VarDecl::VarDecl()
    : Decl(NodeType::VarDecl)
    , initializer_()
    , is_const_() {}

VarDecl::~VarDecl() {}

void VarDecl::initializer(const NodePtr<Expr>& value) {
    initializer_ = value;
}

const NodePtr<Expr>& VarDecl::initializer() const {
    return initializer_;
}

void VarDecl::is_const(const bool& value) {
    is_const_ = value;
}

const bool& VarDecl::is_const() const {
    return is_const_;
}

Expr::Expr(NodeType child_type)
    : Node(child_type)
    , expr_type_(ExprType::None) {
    HAMMER_ASSERT(
        child_type >= NodeType::FirstExpr && child_type <= NodeType::LastExpr,
        "Invalid child type.");
}

Expr::~Expr() {}

void Expr::expr_type(const ExprType& value) {
    expr_type_ = value;
}

const ExprType& Expr::expr_type() const {
    return expr_type_;
}

BinaryExpr::BinaryExpr(BinaryOperator operation)
    : Expr(NodeType::BinaryExpr)
    , operation_(std::move(operation))
    , left_()
    , right_() {}

BinaryExpr::~BinaryExpr() {}

void BinaryExpr::operation(const BinaryOperator& value) {
    operation_ = value;
}

const BinaryOperator& BinaryExpr::operation() const {
    return operation_;
}

void BinaryExpr::left(const NodePtr<Expr>& value) {
    left_ = value;
}

const NodePtr<Expr>& BinaryExpr::left() const {
    return left_;
}

void BinaryExpr::right(const NodePtr<Expr>& value) {
    right_ = value;
}

const NodePtr<Expr>& BinaryExpr::right() const {
    return right_;
}

BlockExpr::BlockExpr()
    : Expr(NodeType::BlockExpr)
    , stmts_()
    , block_scope_() {}

BlockExpr::~BlockExpr() {}

void BlockExpr::stmts(const NodePtr<StmtList>& value) {
    stmts_ = value;
}

const NodePtr<StmtList>& BlockExpr::stmts() const {
    return stmts_;
}

void BlockExpr::block_scope(ScopePtr value) {
    block_scope_ = value;
}

ScopePtr BlockExpr::block_scope() const {
    return block_scope_.lock();
}

BreakExpr::BreakExpr()
    : Expr(NodeType::BreakExpr) {}

BreakExpr::~BreakExpr() {}

CallExpr::CallExpr()
    : Expr(NodeType::CallExpr)
    , func_()
    , args_() {}

CallExpr::~CallExpr() {}

void CallExpr::func(const NodePtr<Expr>& value) {
    func_ = value;
}

const NodePtr<Expr>& CallExpr::func() const {
    return func_;
}

void CallExpr::args(const NodePtr<ExprList>& value) {
    args_ = value;
}

const NodePtr<ExprList>& CallExpr::args() const {
    return args_;
}

ContinueExpr::ContinueExpr()
    : Expr(NodeType::ContinueExpr) {}

ContinueExpr::~ContinueExpr() {}

DotExpr::DotExpr()
    : Expr(NodeType::DotExpr)
    , inner_()
    , name_() {}

DotExpr::~DotExpr() {}

void DotExpr::inner(const NodePtr<Expr>& value) {
    inner_ = value;
}

const NodePtr<Expr>& DotExpr::inner() const {
    return inner_;
}

void DotExpr::name(const InternedString& value) {
    name_ = value;
}

const InternedString& DotExpr::name() const {
    return name_;
}

IfExpr::IfExpr()
    : Expr(NodeType::IfExpr)
    , condition_()
    , then_branch_()
    , else_branch_() {}

IfExpr::~IfExpr() {}

void IfExpr::condition(const NodePtr<Expr>& value) {
    condition_ = value;
}

const NodePtr<Expr>& IfExpr::condition() const {
    return condition_;
}

void IfExpr::then_branch(const NodePtr<Expr>& value) {
    then_branch_ = value;
}

const NodePtr<Expr>& IfExpr::then_branch() const {
    return then_branch_;
}

void IfExpr::else_branch(const NodePtr<Expr>& value) {
    else_branch_ = value;
}

const NodePtr<Expr>& IfExpr::else_branch() const {
    return else_branch_;
}

IndexExpr::IndexExpr()
    : Expr(NodeType::IndexExpr)
    , inner_()
    , index_() {}

IndexExpr::~IndexExpr() {}

void IndexExpr::inner(const NodePtr<Expr>& value) {
    inner_ = value;
}

const NodePtr<Expr>& IndexExpr::inner() const {
    return inner_;
}

void IndexExpr::index(const NodePtr<Expr>& value) {
    index_ = value;
}

const NodePtr<Expr>& IndexExpr::index() const {
    return index_;
}

Literal::Literal(NodeType child_type)
    : Expr(child_type) {
    HAMMER_ASSERT(child_type >= NodeType::FirstLiteral
                      && child_type <= NodeType::LastLiteral,
        "Invalid child type.");
}

Literal::~Literal() {}

ArrayLiteral::ArrayLiteral()
    : Literal(NodeType::ArrayLiteral)
    , entries_() {}

ArrayLiteral::~ArrayLiteral() {}

void ArrayLiteral::entries(const NodePtr<ExprList>& value) {
    entries_ = value;
}

const NodePtr<ExprList>& ArrayLiteral::entries() const {
    return entries_;
}

BooleanLiteral::BooleanLiteral(bool value)
    : Literal(NodeType::BooleanLiteral)
    , value_(std::move(value)) {}

BooleanLiteral::~BooleanLiteral() {}

void BooleanLiteral::value(const bool& value) {
    value_ = value;
}

const bool& BooleanLiteral::value() const {
    return value_;
}

FloatLiteral::FloatLiteral(f64 value)
    : Literal(NodeType::FloatLiteral)
    , value_(std::move(value)) {}

FloatLiteral::~FloatLiteral() {}

void FloatLiteral::value(const f64& value) {
    value_ = value;
}

const f64& FloatLiteral::value() const {
    return value_;
}

FuncLiteral::FuncLiteral()
    : Literal(NodeType::FuncLiteral)
    , func_() {}

FuncLiteral::~FuncLiteral() {}

void FuncLiteral::func(const NodePtr<FuncDecl>& value) {
    func_ = value;
}

const NodePtr<FuncDecl>& FuncLiteral::func() const {
    return func_;
}

IntegerLiteral::IntegerLiteral(i64 value)
    : Literal(NodeType::IntegerLiteral)
    , value_(std::move(value)) {}

IntegerLiteral::~IntegerLiteral() {}

void IntegerLiteral::value(const i64& value) {
    value_ = value;
}

const i64& IntegerLiteral::value() const {
    return value_;
}

MapLiteral::MapLiteral()
    : Literal(NodeType::MapLiteral)
    , entries_() {}

MapLiteral::~MapLiteral() {}

void MapLiteral::entries(const NodePtr<MapEntryList>& value) {
    entries_ = value;
}

const NodePtr<MapEntryList>& MapLiteral::entries() const {
    return entries_;
}

NullLiteral::NullLiteral()
    : Literal(NodeType::NullLiteral) {}

NullLiteral::~NullLiteral() {}

SetLiteral::SetLiteral()
    : Literal(NodeType::SetLiteral)
    , entries_() {}

SetLiteral::~SetLiteral() {}

void SetLiteral::entries(const NodePtr<ExprList>& value) {
    entries_ = value;
}

const NodePtr<ExprList>& SetLiteral::entries() const {
    return entries_;
}

StringLiteral::StringLiteral(InternedString value)
    : Literal(NodeType::StringLiteral)
    , value_(std::move(value)) {}

StringLiteral::~StringLiteral() {}

void StringLiteral::value(const InternedString& value) {
    value_ = value;
}

const InternedString& StringLiteral::value() const {
    return value_;
}

SymbolLiteral::SymbolLiteral(InternedString value)
    : Literal(NodeType::SymbolLiteral)
    , value_(std::move(value)) {}

SymbolLiteral::~SymbolLiteral() {}

void SymbolLiteral::value(const InternedString& value) {
    value_ = value;
}

const InternedString& SymbolLiteral::value() const {
    return value_;
}

TupleLiteral::TupleLiteral()
    : Literal(NodeType::TupleLiteral)
    , entries_() {}

TupleLiteral::~TupleLiteral() {}

void TupleLiteral::entries(const NodePtr<ExprList>& value) {
    entries_ = value;
}

const NodePtr<ExprList>& TupleLiteral::entries() const {
    return entries_;
}

ReturnExpr::ReturnExpr()
    : Expr(NodeType::ReturnExpr)
    , inner_() {}

ReturnExpr::~ReturnExpr() {}

void ReturnExpr::inner(const NodePtr<Expr>& value) {
    inner_ = value;
}

const NodePtr<Expr>& ReturnExpr::inner() const {
    return inner_;
}

StringSequenceExpr::StringSequenceExpr()
    : Expr(NodeType::StringSequenceExpr)
    , strings_() {}

StringSequenceExpr::~StringSequenceExpr() {}

void StringSequenceExpr::strings(const NodePtr<ExprList>& value) {
    strings_ = value;
}

const NodePtr<ExprList>& StringSequenceExpr::strings() const {
    return strings_;
}

UnaryExpr::UnaryExpr(UnaryOperator operation)
    : Expr(NodeType::UnaryExpr)
    , operation_(std::move(operation))
    , inner_() {}

UnaryExpr::~UnaryExpr() {}

void UnaryExpr::operation(const UnaryOperator& value) {
    operation_ = value;
}

const UnaryOperator& UnaryExpr::operation() const {
    return operation_;
}

void UnaryExpr::inner(const NodePtr<Expr>& value) {
    inner_ = value;
}

const NodePtr<Expr>& UnaryExpr::inner() const {
    return inner_;
}

VarExpr::VarExpr(InternedString name)
    : Expr(NodeType::VarExpr)
    , name_(std::move(name))
    , surrounding_scope_()
    , resolved_symbol_() {}

VarExpr::~VarExpr() {}

void VarExpr::name(const InternedString& value) {
    name_ = value;
}

const InternedString& VarExpr::name() const {
    return name_;
}

void VarExpr::surrounding_scope(ScopePtr value) {
    surrounding_scope_ = value;
}

ScopePtr VarExpr::surrounding_scope() const {
    return surrounding_scope_.lock();
}

void VarExpr::resolved_symbol(SymbolEntryPtr value) {
    resolved_symbol_ = value;
}

SymbolEntryPtr VarExpr::resolved_symbol() const {
    return resolved_symbol_.lock();
}

ExprList::ExprList()
    : Node(NodeType::ExprList) {}

ExprList::~ExprList() {}

File::File()
    : Node(NodeType::File)
    , file_name_()
    , items_()
    , file_scope_() {}

File::~File() {}

void File::file_name(const InternedString& value) {
    file_name_ = value;
}

const InternedString& File::file_name() const {
    return file_name_;
}

void File::items(const NodePtr<NodeList>& value) {
    items_ = value;
}

const NodePtr<NodeList>& File::items() const {
    return items_;
}

void File::file_scope(ScopePtr value) {
    file_scope_ = value;
}

ScopePtr File::file_scope() const {
    return file_scope_.lock();
}

MapEntry::MapEntry()
    : Node(NodeType::MapEntry)
    , key_()
    , value_() {}

MapEntry::~MapEntry() {}

void MapEntry::key(const NodePtr<Expr>& value) {
    key_ = value;
}

const NodePtr<Expr>& MapEntry::key() const {
    return key_;
}

void MapEntry::value(const NodePtr<Expr>& value) {
    value_ = value;
}

const NodePtr<Expr>& MapEntry::value() const {
    return value_;
}

MapEntryList::MapEntryList()
    : Node(NodeType::MapEntryList) {}

MapEntryList::~MapEntryList() {}

NodeList::NodeList()
    : Node(NodeType::NodeList) {}

NodeList::~NodeList() {}

ParamList::ParamList()
    : Node(NodeType::ParamList) {}

ParamList::~ParamList() {}

Root::Root()
    : Node(NodeType::Root)
    , file_()
    , root_scope_() {}

Root::~Root() {}

void Root::file(const NodePtr<File>& value) {
    file_ = value;
}

const NodePtr<File>& Root::file() const {
    return file_;
}

void Root::root_scope(ScopePtr value) {
    root_scope_ = value;
}

ScopePtr Root::root_scope() const {
    return root_scope_.lock();
}

Stmt::Stmt(NodeType child_type)
    : Node(child_type) {
    HAMMER_ASSERT(
        child_type >= NodeType::FirstStmt && child_type <= NodeType::LastStmt,
        "Invalid child type.");
}

Stmt::~Stmt() {}

AssertStmt::AssertStmt()
    : Stmt(NodeType::AssertStmt)
    , condition_()
    , message_() {}

AssertStmt::~AssertStmt() {}

void AssertStmt::condition(const NodePtr<Expr>& value) {
    condition_ = value;
}

const NodePtr<Expr>& AssertStmt::condition() const {
    return condition_;
}

void AssertStmt::message(const NodePtr<StringLiteral>& value) {
    message_ = value;
}

const NodePtr<StringLiteral>& AssertStmt::message() const {
    return message_;
}

DeclStmt::DeclStmt()
    : Stmt(NodeType::DeclStmt)
    , decl_() {}

DeclStmt::~DeclStmt() {}

void DeclStmt::decl(const NodePtr<VarDecl>& value) {
    decl_ = value;
}

const NodePtr<VarDecl>& DeclStmt::decl() const {
    return decl_;
}

EmptyStmt::EmptyStmt()
    : Stmt(NodeType::EmptyStmt) {}

EmptyStmt::~EmptyStmt() {}

ExprStmt::ExprStmt()
    : Stmt(NodeType::ExprStmt)
    , expr_() {}

ExprStmt::~ExprStmt() {}

void ExprStmt::expr(const NodePtr<Expr>& value) {
    expr_ = value;
}

const NodePtr<Expr>& ExprStmt::expr() const {
    return expr_;
}

ForStmt::ForStmt()
    : Stmt(NodeType::ForStmt)
    , decl_()
    , condition_()
    , step_()
    , body_()
    , decl_scope_()
    , body_scope_() {}

ForStmt::~ForStmt() {}

void ForStmt::decl(const NodePtr<DeclStmt>& value) {
    decl_ = value;
}

const NodePtr<DeclStmt>& ForStmt::decl() const {
    return decl_;
}

void ForStmt::condition(const NodePtr<Expr>& value) {
    condition_ = value;
}

const NodePtr<Expr>& ForStmt::condition() const {
    return condition_;
}

void ForStmt::step(const NodePtr<Expr>& value) {
    step_ = value;
}

const NodePtr<Expr>& ForStmt::step() const {
    return step_;
}

void ForStmt::body(const NodePtr<Expr>& value) {
    body_ = value;
}

const NodePtr<Expr>& ForStmt::body() const {
    return body_;
}

void ForStmt::decl_scope(ScopePtr value) {
    decl_scope_ = value;
}

ScopePtr ForStmt::decl_scope() const {
    return decl_scope_.lock();
}

void ForStmt::body_scope(ScopePtr value) {
    body_scope_ = value;
}

ScopePtr ForStmt::body_scope() const {
    return body_scope_.lock();
}

WhileStmt::WhileStmt()
    : Stmt(NodeType::WhileStmt)
    , condition_()
    , body_()
    , body_scope_() {}

WhileStmt::~WhileStmt() {}

void WhileStmt::condition(const NodePtr<Expr>& value) {
    condition_ = value;
}

const NodePtr<Expr>& WhileStmt::condition() const {
    return condition_;
}

void WhileStmt::body(const NodePtr<BlockExpr>& value) {
    body_ = value;
}

const NodePtr<BlockExpr>& WhileStmt::body() const {
    return body_;
}

void WhileStmt::body_scope(ScopePtr value) {
    body_scope_ = value;
}

ScopePtr WhileStmt::body_scope() const {
    return body_scope_.lock();
}

StmtList::StmtList()
    : Node(NodeType::StmtList) {}

StmtList::~StmtList() {}

} // namespace hammer::compiler
