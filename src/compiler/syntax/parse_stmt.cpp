#include "compiler/syntax/parse_stmt.hpp"

#include "common/assert.hpp"
#include "compiler/syntax/parse_expr.hpp"
#include "compiler/syntax/parser.hpp"
#include "compiler/syntax/token_set.hpp"

namespace tiro::next {

static const TokenSet VAR_DECL_FIRST = {
    TokenType::KwConst,
    TokenType::KwVar,
};

static const TokenSet EXPR_STMT_OPTIONAL_SEMI = {
    TokenType::KwFunc,
    TokenType::KwIf,
    TokenType::LeftBrace,
};

static void parse_assert_stmt(Parser& p, const TokenSet& recovery);
static void parse_while_stmt(Parser& p, const TokenSet& recovery);
static void parse_for_stmt(Parser& p, const TokenSet& recovery);
static void parse_var_decl_stmt(Parser& p, const TokenSet& recovery);
static void parse_expr_stmt(Parser& p, const TokenSet& recovery);

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

    case TokenType::KwAssert:
        return parse_assert_stmt(p, recovery);

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

void parse_assert_stmt(Parser& p, const TokenSet& recovery) {
    TIRO_NOT_IMPLEMENTED();
    (void) p;
    (void) recovery;
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
    TIRO_NOT_IMPLEMENTED();
    (void) p;
    (void) recovery;
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

} // namespace tiro::next
