#include "compiler/syntax/parse_stmt.hpp"

#include "common/assert.hpp"
#include "compiler/syntax/parse_expr.hpp"
#include "compiler/syntax/parse_misc.hpp"
#include "compiler/syntax/parser.hpp"
#include "compiler/syntax/token_set.hpp"

namespace tiro::next {

namespace {

enum class DeclKind {
    NormalDecl,
    ForEachDecl, // (a, b) in
};

} // namespace

static const TokenSet VAR_DECL_FIRST = {
    TokenType::KwConst,
    TokenType::KwVar,
};

static const TokenSet EXPR_STMT_OPTIONAL_SEMI = {
    TokenType::KwFunc,
    TokenType::KwIf,
    TokenType::LeftBrace,
};

static void parse_while_stmt(Parser& p, const TokenSet& recovery);
static void parse_for_stmt(Parser& p, const TokenSet& recovery);
static void parse_var_decl_stmt(Parser& p, const TokenSet& recovery);
static void parse_expr_stmt(Parser& p, const TokenSet& recovery);

static DeclKind parse_var_decl(Parser& p, const TokenSet& recovery, bool allow_for_each);
static DeclKind parse_binding(Parser& p, const TokenSet& recovery, bool allow_for_each);

void parse_stmt(Parser& p, const TokenSet& recovery) {
    switch (p.current()) {

    case TokenType::KwDefer: {
        auto m = p.start();
        p.advance();
        parse_expr(p, recovery.union_with(TokenType::Semicolon));
        p.expect(TokenType::Semicolon);
        m.complete(SyntaxType::DeferStmt);
        return;
    }

    case TokenType::KwAssert: {
        auto m = p.start();
        p.advance();
        parse_arg_list(p, recovery);
        p.expect(TokenType::Semicolon);
        m.complete(SyntaxType::AssertStmt);
        return;
    }

    case TokenType::KwWhile:
        return parse_while_stmt(p, recovery);

    case TokenType::KwFor:
        return parse_for_stmt(p, recovery);

    default:
        break;
    }

    if (p.at_any(VAR_DECL_FIRST))
        return parse_var_decl_stmt(p, recovery);

    if (p.at_any(EXPR_FIRST))
        return parse_expr_stmt(p, recovery);

    p.error_recover("expected a statement", recovery);
}

void parse_while_stmt(Parser& p, const TokenSet& recovery) {
    TIRO_NOT_IMPLEMENTED();
    (void) p;
    (void) recovery;
}

void parse_for_stmt(Parser& p, const TokenSet& recovery) {
    TIRO_NOT_IMPLEMENTED();
    (void) p;
    (void) recovery;
}

void parse_var_decl_stmt(Parser& p, const TokenSet& recovery) {
    TIRO_DEBUG_ASSERT(p.at_any(VAR_DECL_FIRST), "Not at the start of a var declaration.");
    auto m = p.start();
    parse_var_decl(p, recovery.union_with(TokenType::Semicolon), false);
    p.expect(TokenType::Semicolon);
    m.complete(SyntaxType::VarDeclStmt);
}

void parse_expr_stmt(Parser& p, const TokenSet& recovery) {
    TIRO_DEBUG_ASSERT(p.at_any(EXPR_FIRST), "Not at the start of an expression.");
    bool need_semi = !p.at_any(EXPR_STMT_OPTIONAL_SEMI);

    auto m = p.start();
    parse_expr(p, recovery.union_with(TokenType::Semicolon));
    if (need_semi) {
        p.expect(TokenType::Semicolon);
    } else {
        p.accept(TokenType::Semicolon);
    }
    m.complete(SyntaxType::ExprStmt);
}

DeclKind parse_var_decl(Parser& p, const TokenSet& recovery, bool allow_for_each) {
    TIRO_DEBUG_ASSERT(p.at_any(VAR_DECL_FIRST), "Not at the start of a var declaration.");

    auto m = p.start();
    p.advance(); // var | const

    DeclKind kind = DeclKind::NormalDecl;
    while (!p.at(TokenType::Eof)) {
        // allow_for_each can only be true in the first iteration
        kind = parse_binding(p, recovery.union_with(TokenType::Comma), allow_for_each);
        if (allow_for_each && kind == DeclKind::ForEachDecl)
            break;

        allow_for_each = false;
        if (!p.accept(TokenType::Comma))
            break;
    }

    m.complete(SyntaxType::VarDecl);
    return kind;
}

DeclKind parse_binding(Parser& p, const TokenSet& recovery, bool allow_for_each) {
    auto m = p.start();
    DeclKind kind = DeclKind::NormalDecl;

    // Parse left hand side
    switch (p.current()) {
    case TokenType::LeftParen: {
        auto lhs = p.start();
        p.advance();
        while (!p.at_any({TokenType::Eof, TokenType::RightParen})) {
            if (!p.expect(TokenType::Identifier))
                break;

            if (!p.at(TokenType::RightParen) && !p.expect(TokenType::Comma))
                break;
        }
        p.expect(TokenType::RightParen);
        lhs.complete(SyntaxType::BindingTuple);
        break;
    }
    case TokenType::Identifier: {
        auto lhs = p.start();
        p.advance();
        lhs.complete(SyntaxType::BindingName);
        break;
    }
    default: {
        auto rec = recovery.union_with(TokenType::Equals);
        if (allow_for_each)
            rec = rec.union_with(TokenType::KwIn);

        p.error_recover("expected a variable name or a tuple pattern", rec);
        break;
    }
    }

    // Parse initializer expression
    if (p.accept(TokenType::Equals)) {
        parse_expr(p, recovery);
    } else if (allow_for_each && p.accept(TokenType::KwIn)) {
        kind = DeclKind::ForEachDecl;
        parse_expr(p, recovery);
    }

    m.complete(SyntaxType::Binding);
    return kind;
}

} // namespace tiro::next
