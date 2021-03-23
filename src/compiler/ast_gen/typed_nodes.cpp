#include "compiler/ast_gen/typed_nodes.hpp"

#include "compiler/syntax/grammar/misc.hpp"

namespace tiro {
namespace typed_syntax {

#define TIRO_TRY_IMPL(var, tempvar, expr) \
    std::optional tempvar = (expr);       \
    if (!tempvar) {                       \
        return {};                        \
    }                                     \
    auto var = std::move(*tempvar)

// expr should return an std::optional
#define TIRO_TRY(var, expr) TIRO_TRY_IMPL(var, var##__LINE__, expr)

static SyntaxNodeScanner scan(SyntaxNodeId node_id, const SyntaxTree& tree) {
    TIRO_DEBUG_ASSERT(node_id, "invalid node id");
    return SyntaxNodeScanner(node_id, tree);
}

[[maybe_unused]] static SyntaxType type(SyntaxNodeId node_id, const SyntaxTree& tree) {
    return tree[node_id]->type();
}

std::optional<Root> Root::read(SyntaxNodeId node_id, const SyntaxTree& tree) {
    auto scanner = scan(node_id, tree);
    TIRO_TRY(item, scanner.search_node());
    return Root{item};
}

std::optional<File> File::read(SyntaxNodeId node_id, const SyntaxTree& tree) {
    return File{scan(node_id, tree)};
}

std::optional<Condition> Condition::read(SyntaxNodeId node_id, const SyntaxTree& tree) {
    auto scanner = scan(node_id, tree);
    TIRO_TRY(expr, scanner.search_node());
    return Condition{expr};
}

std::optional<Modifiers> Modifiers::read(SyntaxNodeId node_id, const SyntaxTree& tree) {
    return Modifiers{scan(node_id, tree)};
}

std::optional<Name> Name::read(SyntaxNodeId node_id, const SyntaxTree& tree) {
    auto scanner = scan(node_id, tree);
    TIRO_TRY(value, scanner.search_token(TokenType::Identifier));
    return Name{value};
}

std::optional<Var> Var::read(SyntaxNodeId node_id, const SyntaxTree& tree) {
    auto scanner = scan(node_id, tree);

    scanner.find([&](auto& child) {
        return scanner.is_node_type(child, SyntaxType::Modifiers)
               || scanner.is_token_type_of(child, VAR_FIRST);
    });

    auto modifiers = scanner.accept_node(SyntaxType::Modifiers);
    TIRO_TRY(decl, scanner.search_token(VAR_FIRST));
    return Var{modifiers, decl, scanner};
}

std::optional<Binding> Binding::read(SyntaxNodeId node_id, const SyntaxTree& tree) {
    auto scanner = scan(node_id, tree);
    TIRO_TRY(spec, scanner.search_node({SyntaxType::BindingName, SyntaxType::BindingTuple}));
    auto init = scanner.search_node();
    return Binding{spec, init};
}

std::optional<BindingName> BindingName::read(SyntaxNodeId node_id, const SyntaxTree& tree) {
    auto scanner = scan(node_id, tree);
    TIRO_TRY(name, scanner.search_token(TokenType::Identifier));
    return BindingName{name};
}

std::optional<BindingTuple> BindingTuple::read(SyntaxNodeId node_id, const SyntaxTree& tree) {
    return BindingTuple{scan(node_id, tree)};
}

std::optional<Func> Func::read(SyntaxNodeId node_id, const SyntaxTree& tree) {
    auto scanner = scan(node_id, tree);
    auto modifiers = scanner.accept_node(SyntaxType::Modifiers);
    auto name = scanner.accept_node(SyntaxType::Name, true);
    TIRO_TRY(params, scanner.search_node(SyntaxType::ParamList));
    bool body_is_value = scanner.accept_token(TokenType::Equals).has_value();
    TIRO_TRY(body, scanner.search_node());
    return Func{modifiers, name, params, body_is_value, body};
}

std::optional<ArgList> ArgList::read(SyntaxNodeId node_id, const SyntaxTree& tree) {
    auto scanner = scan(node_id, tree);
    TIRO_TRY(paren, scanner.search_token({TokenType::LeftParen, TokenType::QuestionLeftParen}));
    return ArgList(paren, scanner);
}

std::optional<ParamList> ParamList::read(SyntaxNodeId node_id, const SyntaxTree& tree) {
    return ParamList{scan(node_id, tree)};
}

std::optional<VarExpr> VarExpr::read(SyntaxNodeId node_id, const SyntaxTree& tree) {
    auto scanner = scan(node_id, tree);
    TIRO_TRY(identifier, scanner.search_token(TokenType::Identifier));
    return VarExpr{identifier};
}

std::optional<Literal> Literal::read(SyntaxNodeId node_id, const SyntaxTree& tree) {
    auto scanner = scan(node_id, tree);
    TIRO_TRY(value, scanner.search_token());
    return Literal{value};
}

std::optional<GroupedExpr> GroupedExpr::read(SyntaxNodeId node_id, const SyntaxTree& tree) {
    auto scanner = scan(node_id, tree);
    TIRO_TRY(expr, scanner.search_node());
    return GroupedExpr{expr};
}

std::optional<ReturnExpr> ReturnExpr::read(SyntaxNodeId node_id, const SyntaxTree& tree) {
    auto scanner = scan(node_id, tree);
    auto value = scanner.search_node();
    return ReturnExpr{value};
}

std::optional<FieldExpr> FieldExpr::read(SyntaxNodeId node_id, const SyntaxTree& tree) {

    auto scanner = scan(node_id, tree);
    TIRO_TRY(instance, scanner.search_node());
    TIRO_TRY(access, scanner.search_token({TokenType::Dot, TokenType::QuestionDot}));
    TIRO_TRY(field, scanner.search_token(TokenType::Identifier));
    return FieldExpr{instance, access, field};
}

std::optional<TupleFieldExpr> TupleFieldExpr::read(SyntaxNodeId node_id, const SyntaxTree& tree) {
    TIRO_DEBUG_ASSERT(
        type(node_id, tree) == SyntaxType::TupleFieldExpr, "expected tuple field expr");
    auto scanner = scan(node_id, tree);
    TIRO_TRY(instance, scanner.search_node());
    TIRO_TRY(access, scanner.search_token({TokenType::Dot, TokenType::QuestionDot}));
    TIRO_TRY(field, scanner.search_token(TokenType::TupleField));
    return TupleFieldExpr{instance, access, field};
}

std::optional<IndexExpr> IndexExpr::read(SyntaxNodeId node_id, const SyntaxTree& tree) {
    auto scanner = scan(node_id, tree);
    TIRO_TRY(instance, scanner.search_node());
    TIRO_TRY(
        bracket, scanner.search_token({TokenType::LeftBracket, TokenType::QuestionLeftBracket}));
    TIRO_TRY(index, scanner.search_node());
    return IndexExpr{instance, bracket, index};
}

std::optional<BinaryExpr> BinaryExpr::read(SyntaxNodeId node_id, const SyntaxTree& tree) {
    auto scanner = scan(node_id, tree);
    TIRO_TRY(lhs, scanner.search_node());
    TIRO_TRY(op, scanner.search_token());
    TIRO_TRY(rhs, scanner.search_node());
    return BinaryExpr{lhs, op, rhs};
}

std::optional<UnaryExpr> UnaryExpr::read(SyntaxNodeId node_id, const SyntaxTree& tree) {
    auto scanner = scan(node_id, tree);
    TIRO_TRY(op, scanner.search_token());
    TIRO_TRY(expr, scanner.search_node());
    return UnaryExpr{op, expr};
}

std::optional<TupleExpr> TupleExpr::read(SyntaxNodeId node_id, const SyntaxTree& tree) {
    return TupleExpr{scan(node_id, tree)};
}

std::optional<RecordExpr> RecordExpr::read(SyntaxNodeId node_id, const SyntaxTree& tree) {
    return RecordExpr{scan(node_id, tree)};
}

std::optional<RecordItem> RecordItem::read(SyntaxNodeId node_id, const SyntaxTree& tree) {
    auto scanner = scan(node_id, tree);
    TIRO_TRY(name, scanner.search_node(SyntaxType::Name));
    TIRO_TRY(value, scanner.search_node());
    return RecordItem{name, value};
}

std::optional<ArrayExpr> ArrayExpr::read(SyntaxNodeId node_id, const SyntaxTree& tree) {
    return ArrayExpr{scan(node_id, tree)};
}

std::optional<SetExpr> SetExpr::read(SyntaxNodeId node_id, const SyntaxTree& tree) {
    return SetExpr{scan(node_id, tree)};
}

std::optional<MapExpr> MapExpr::read(SyntaxNodeId node_id, const SyntaxTree& tree) {
    return MapExpr{scan(node_id, tree)};
}

std::optional<MapItem> MapItem::read(SyntaxNodeId node_id, const SyntaxTree& tree) {
    auto scanner = scan(node_id, tree);
    TIRO_TRY(key, scanner.search_node());
    TIRO_TRY(value, scanner.search_node());
    return MapItem{key, value};
}

std::optional<StringExpr> StringExpr::read(SyntaxNodeId node_id, const SyntaxTree& tree) {
    return StringExpr{scan(node_id, tree)};
}

std::optional<StringFormatItem>
StringFormatItem::read(SyntaxNodeId node_id, const SyntaxTree& tree) {
    TIRO_DEBUG_ASSERT(
        type(node_id, tree) == SyntaxType::StringFormatItem, "expected string format item");
    auto scanner = scan(node_id, tree);
    TIRO_TRY(expr, scanner.search_node());
    return StringFormatItem{expr};
}

std::optional<StringFormatBlock>
StringFormatBlock::read(SyntaxNodeId node_id, const SyntaxTree& tree) {
    TIRO_DEBUG_ASSERT(
        type(node_id, tree) == SyntaxType::StringFormatBlock, "expected string format block");
    auto scanner = scan(node_id, tree);
    TIRO_TRY(expr, scanner.search_node());
    return StringFormatBlock{expr};
}

std::optional<StringGroup> StringGroup::read(SyntaxNodeId node_id, const SyntaxTree& tree) {
    return StringGroup{scan(node_id, tree)};
}

std::optional<IfExpr> IfExpr::read(SyntaxNodeId node_id, const SyntaxTree& tree) {
    auto scanner = scan(node_id, tree);
    TIRO_TRY(cond, scanner.search_node(SyntaxType::Condition));
    TIRO_TRY(then_branch, scanner.search_node());
    auto else_branch = scanner.search_node();
    return IfExpr{cond, then_branch, else_branch};
}

std::optional<BlockExpr> BlockExpr::read(SyntaxNodeId node_id, const SyntaxTree& tree) {
    return BlockExpr{scan(node_id, tree)};
}

std::optional<FuncExpr> FuncExpr::read(SyntaxNodeId node_id, const SyntaxTree& tree) {
    auto scanner = scan(node_id, tree);
    TIRO_TRY(func, scanner.search_node(SyntaxType::Func));
    return FuncExpr{func};
}

std::optional<CallExpr> CallExpr::read(SyntaxNodeId node_id, const SyntaxTree& tree) {
    auto scanner = scan(node_id, tree);
    TIRO_TRY(func, scanner.search_node());
    TIRO_TRY(args, scanner.search_node(SyntaxType::ArgList));
    return CallExpr{func, args};
}

std::optional<ExprStmt> ExprStmt::read(SyntaxNodeId node_id, const SyntaxTree& tree) {
    auto scanner = scan(node_id, tree);
    TIRO_TRY(expr, scanner.search_node());
    return ExprStmt{expr};
}

std::optional<DeferStmt> DeferStmt::read(SyntaxNodeId node_id, const SyntaxTree& tree) {
    auto scanner = scan(node_id, tree);
    TIRO_TRY(expr, scanner.search_node());
    return DeferStmt{expr};
}

std::optional<AssertStmt> AssertStmt::read(SyntaxNodeId node_id, const SyntaxTree& tree) {
    auto scanner = scan(node_id, tree);
    TIRO_TRY(args, scanner.search_node(SyntaxType::ArgList));
    return AssertStmt{args};
}

std::optional<VarStmt> VarStmt::read(SyntaxNodeId node_id, const SyntaxTree& tree) {
    auto scanner = scan(node_id, tree);
    TIRO_TRY(var, scanner.search_node(SyntaxType::Var));
    return VarStmt{var};
}

std::optional<WhileStmt> WhileStmt::read(SyntaxNodeId node_id, const SyntaxTree& tree) {
    auto scanner = scan(node_id, tree);
    TIRO_TRY(cond, scanner.search_node(SyntaxType::Condition));
    TIRO_TRY(body, scanner.search_node());
    return WhileStmt{cond, body};
}

std::optional<ForStmt> ForStmt::read(SyntaxNodeId node_id, const SyntaxTree& tree) {
    auto scanner = scan(node_id, tree);
    TIRO_TRY(header, scanner.search_node(SyntaxType::ForStmtHeader));
    TIRO_TRY(body, scanner.search_node());
    return ForStmt{header, body};
}

std::optional<ForStmtHeader> ForStmtHeader::read(SyntaxNodeId node_id, const SyntaxTree& tree) {
    auto scanner = scan(node_id, tree);

    auto seek_item = [&](SyntaxType expected) -> std::optional<SyntaxNodeId> {
        auto result = scanner.search([&](auto& child) {
            return scanner.is_token_type(child, TokenType::Semicolon)
                   || scanner.is_node_type(child, expected);
        });

        if (result && result->type() == SyntaxChildType::NodeId) {
            scanner.search_token(TokenType::Semicolon);
            return result->as_node_id();
        }

        // Semicolon was consumed already
        return {};
    };

    auto decl = seek_item(SyntaxType::Var);
    auto cond = seek_item(SyntaxType::Condition);
    auto step = scanner.search_node();
    return ForStmtHeader{decl, cond, step};
}

std::optional<ForEachStmt> ForEachStmt::read(SyntaxNodeId node_id, const SyntaxTree& tree) {
    auto scanner = scan(node_id, tree);
    TIRO_TRY(header, scanner.search_node(SyntaxType::ForEachStmtHeader));
    TIRO_TRY(body, scanner.search_node());
    return ForEachStmt{header, body};
}

std::optional<ForEachStmtHeader>
ForEachStmtHeader::read(SyntaxNodeId node_id, const SyntaxTree& tree) {
    TIRO_DEBUG_ASSERT(
        type(node_id, tree) == SyntaxType::ForEachStmtHeader, "expected for each stmt header");
    auto scanner = scan(node_id, tree);
    TIRO_TRY(spec, scanner.search_node({SyntaxType::BindingName, SyntaxType::BindingTuple}));
    TIRO_TRY(expr, scanner.search_node());
    return ForEachStmtHeader{spec, expr};
}

std::optional<FuncItem> FuncItem::read(SyntaxNodeId node_id, const SyntaxTree& tree) {
    auto scanner = scan(node_id, tree);
    TIRO_TRY(func, scanner.search_node());
    return FuncItem{func};
}

std::optional<VarItem> VarItem::read(SyntaxNodeId node_id, const SyntaxTree& tree) {
    auto scanner = scan(node_id, tree);
    TIRO_TRY(var, scanner.search_node());
    return VarItem{var};
}

std::optional<ImportItem> ImportItem::read(SyntaxNodeId node_id, const SyntaxTree& tree) {
    return ImportItem{scan(node_id, tree)};
}

} // namespace typed_syntax
} // namespace tiro
