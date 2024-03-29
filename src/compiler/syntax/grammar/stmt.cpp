#include "compiler/syntax/grammar/stmt.hpp"

#include "common/assert.hpp"
#include "compiler/syntax/grammar/expr.hpp"
#include "compiler/syntax/grammar/misc.hpp"
#include "compiler/syntax/parser.hpp"
#include "compiler/syntax/token_set.hpp"

namespace tiro {

static const TokenSet EXPR_STMT_OPTIONAL_SEMI = {
    TokenType::KwFunc,
    TokenType::KwIf,
    TokenType::LeftBrace,
};

const TokenSet STMT_FIRST = EXPR_FIRST //
                                .union_with(VAR_FIRST)
                                .union_with({
                                    TokenType::KwDefer,
                                    TokenType::KwAssert,
                                    TokenType::KwWhile,
                                    TokenType::KwFor,
                                });

static void parse_while_stmt(Parser& p, const TokenSet& recovery);
static void parse_for_stmt(Parser& p, const TokenSet& recovery);
static void parse_expr_stmt(Parser& p, const TokenSet& recovery);
static void parse_var_stmt(Parser& p, const TokenSet& recovery);

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

    if (p.at_any(VAR_FIRST))
        return parse_var_stmt(p, recovery);

    if (p.at_any(EXPR_FIRST))
        return parse_expr_stmt(p, recovery);

    p.error_recover("expected a statement", recovery);
}

void parse_while_stmt(Parser& p, const TokenSet& recovery) {
    TIRO_DEBUG_ASSERT(p.at(TokenType::KwWhile), "Not at the start of a while loop.");

    auto m = p.start();
    p.advance();
    parse_condition(p, recovery.union_with(TokenType::LeftBrace));
    parse_block_expr(p, recovery);
    m.complete(SyntaxType::WhileStmt);
}

void parse_for_stmt(Parser& p, const TokenSet& recovery) {
    TIRO_DEBUG_ASSERT(p.at(TokenType::KwFor), "Not at the start of a for loop.");

    auto m = p.start();
    p.advance();

    if (p.ahead(0) == TokenType::LeftParen && VAR_FIRST.contains(p.ahead(1))) {
        p.error(fmt::format(
            "classic for loops do not start with {}", to_description(TokenType::LeftParen)));
    }

    // Classic for loop
    if (p.at(TokenType::Semicolon) || p.at_any(VAR_FIRST)) {
        auto h = p.start();
        // Optional variable declaration
        if (!p.accept(TokenType::Semicolon)) {
            parse_var(p, VarKind::NoBlock, recovery.union_with(TokenType::Semicolon), {});
            p.expect(TokenType::Semicolon);
        }
        // Optional condition
        if (!p.accept(TokenType::Semicolon)) {
            auto cond = p.start();
            parse_expr(p, recovery.union_with(TokenType::Semicolon));
            cond.complete(SyntaxType::Condition);
            p.expect(TokenType::Semicolon);
        }
        // Optional update step
        if (!p.at(TokenType::LeftBrace)) {
            parse_expr_no_block(p, recovery.union_with(TokenType::LeftBrace));
        }
        h.complete(SyntaxType::ForStmtHeader);
        parse_block_expr(p, recovery);
        m.complete(SyntaxType::ForStmt);
        return;
    }

    // For each loop
    if (p.at_any(BINDING_PATTERN_FIRST)) {
        auto h = p.start();
        parse_binding_pattern(p, recovery.union_with(TokenType::KwIn));
        p.expect(TokenType::KwIn);
        parse_expr_no_block(p, recovery.union_with(TokenType::LeftBrace));
        h.complete(SyntaxType::ForEachStmtHeader);

        parse_block_expr(p, recovery);
        m.complete(SyntaxType::ForEachStmt);
        return;
    }

    p.error_recover("expected a for each loop or a classic for loop", recovery);
}

void parse_var_stmt(Parser& p, const TokenSet& recovery) {
    TIRO_DEBUG_ASSERT(p.at_any(VAR_FIRST), "Not at the start of a var declaration.");
    auto m = p.start();
    parse_var(p, VarKind::Default, recovery.union_with(TokenType::Semicolon), {});
    p.expect(TokenType::Semicolon);
    m.complete(SyntaxType::VarStmt);
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

} // namespace tiro
