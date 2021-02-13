#include "compiler/syntax/parse_expr.hpp"

#include "compiler/syntax/operators.hpp"

namespace tiro::next {

static std::optional<Parser::CompletedMarker>
parse_expr(Parser& p, int bp, const TokenSet& recovery);

static std::optional<Parser::CompletedMarker>
parse_prefix_expr(Parser& p, const TokenSet& recovery);

static Parser::CompletedMarker
parse_postfix_expr(Parser& p, Parser::CompletedMarker c, const TokenSet& recovery);

static std::optional<Parser::CompletedMarker>
parse_primary_expr(Parser& p, const TokenSet& recovery);

static std::optional<Parser::CompletedMarker> parse_literal(Parser& p);

static Parser::CompletedMarker parse_block_expr(Parser& p, const TokenSet& recovery);
static Parser::CompletedMarker parse_paren_expr(Parser& p, const TokenSet& recovery);
static Parser::CompletedMarker parse_if_expr(Parser& p, const TokenSet& recovery);
static Parser::CompletedMarker parse_func_expr(Parser& p, const TokenSet& recovery);
static Parser::CompletedMarker parse_array_expr(Parser& p, const TokenSet& recovery);
static Parser::CompletedMarker parse_map_expr(Parser& p, const TokenSet& recovery);
static Parser::CompletedMarker parse_set_expr(Parser& p, const TokenSet& recovery);

static const TokenSet LITERAL_FIRST = {
    TokenType::KwTrue,
    TokenType::KwFalse,
    TokenType::KwNull,
    TokenType::Symbol,
    TokenType::Float,
    TokenType::Integer,
};

static const TokenSet UNARY_OP_FIRST = {
    TokenType::Plus,
    TokenType::Minus,
    TokenType::BitwiseNot,
    TokenType::LogicalNot,
};

static const TokenSet EXPR_FIRST = //
    LITERAL_FIRST                  //
        .union_with(UNARY_OP_FIRST)
        .union_with({
            TokenType::KwFunc,
            TokenType::KwContinue,
            TokenType::KwBreak,
            TokenType::KwReturn,
            TokenType::KwIf,
            TokenType::MapStart,
            TokenType::SetStart,
            TokenType::Identifier,

            // Strings
            TokenType::StringStart,

            // ( expr ) either a braced expr or a tuple
            TokenType::LeftParen,

            // Array
            TokenType::LeftBracket,

            // { statements ... }
            TokenType::LeftBrace,
        });

/// Recursive function that implements a pratt parser.
///
/// See also:
///      http://crockford.com/javascript/tdop/tdop.html
///      https://www.oilshell.org/blog/2016/11/01.html
///      https://groups.google.com/forum/#!topic/comp.compilers/ruJLlQTVJ8o
std::optional<Parser::CompletedMarker> parse_expr(Parser& p, const TokenSet& recovery) {
    return parse_expr(p, 0, recovery);
}

std::optional<Parser::CompletedMarker> parse_expr(Parser& p, int bp, const TokenSet& recovery) {
    auto lhs = parse_prefix_expr(p, recovery);
    if (!lhs)
        return {};

    while (1) {
        auto op = infix_operator_precedence(p.current());
        if (!op)
            break; // Not an infix operator.

        if (op->precedence < bp)
            break; // Upper call will handle lower precedence

        auto m = lhs->precede();
        p.advance();

        int next_bp = op->precedence;
        if (!op->right_assoc)
            ++next_bp;
        parse_expr(p, next_bp, recovery);

        lhs = m.complete(SyntaxType::BinaryExpr);
    }

    return lhs;
}

std::optional<Parser::CompletedMarker> parse_prefix_expr(Parser& p, const TokenSet& recovery) {
    if (!p.at_any(UNARY_OP_FIRST)) {
        auto expr = parse_primary_expr(p, recovery);
        if (!expr)
            return {};

        return parse_postfix_expr(p, *expr, recovery);
    }

    auto m = p.start();
    p.advance();
    parse_expr(p, unary_precedence, recovery);
    return m.complete(SyntaxType::UnaryExpr);
}

Parser::CompletedMarker
parse_postfix_expr(Parser& p, Parser::CompletedMarker c, const TokenSet& recovery) {
    (void) p;
    (void) recovery;
    return c; // TODO
}

std::optional<Parser::CompletedMarker> parse_primary_expr(Parser& p, const TokenSet& recovery) {
    if (auto c = parse_literal(p))
        return *c;

    switch (p.current()) {

    // { stmts ... }
    case TokenType::LeftBrace:
        return parse_block_expr(p, recovery);

    // (expr) or tuple
    case TokenType::LeftParen:
        return parse_paren_expr(p, recovery);

    // if (expr) else ...
    case TokenType::KwIf:
        return parse_if_expr(p, recovery);

    // return [expr]
    case TokenType::KwReturn: {
        auto m = p.start();
        p.advance();
        if (p.at_any(EXPR_FIRST))
            parse_expr(p, recovery);
        return m.complete(SyntaxType::ReturnExpr);
    }

    // continue
    case TokenType::KwContinue: {
        auto m = p.start();
        p.advance();
        return m.complete(SyntaxType::ContinueExpr);
    }

    // break
    case TokenType::KwBreak: {
        auto m = p.start();
        p.advance();
        return m.complete(SyntaxType::BreakExpr);
    }

    // single identifier
    case TokenType::Identifier: {
        auto m = p.start();
        p.advance();
        return m.complete(SyntaxType::VarExpr);
    }

    case TokenType::KwFunc:
        return parse_func_expr(p, recovery);

    case TokenType::LeftBracket:
        return parse_array_expr(p, recovery);

    case TokenType::MapStart:
        return parse_map_expr(p, recovery);

    case TokenType::SetStart:
        return parse_set_expr(p, recovery);

    default:
        return {};
    }
}

std::optional<Parser::CompletedMarker> parse_literal(Parser& p) {
    if (p.at_any(LITERAL_FIRST)) {
        auto m = p.start();
        p.advance();
        return m.complete(SyntaxType::Literal);
    }
    return {};
}

Parser::CompletedMarker parse_block_expr(Parser& p, const TokenSet& recovery) {
    TIRO_NOT_IMPLEMENTED();
    (void) p;
    (void) recovery;
}

Parser::CompletedMarker parse_paren_expr(Parser& p, const TokenSet& recovery) {
    TIRO_NOT_IMPLEMENTED();
    (void) p;
    (void) recovery;
}

Parser::CompletedMarker parse_if_expr(Parser& p, const TokenSet& recovery) {
    TIRO_NOT_IMPLEMENTED();
    (void) p;
    (void) recovery;
}

Parser::CompletedMarker parse_func_expr(Parser& p, const TokenSet& recovery) {
    TIRO_NOT_IMPLEMENTED();
    (void) p;
    (void) recovery;
}

Parser::CompletedMarker parse_array_expr(Parser& p, const TokenSet& recovery) {
    TIRO_NOT_IMPLEMENTED();
    (void) p;
    (void) recovery;
}

Parser::CompletedMarker parse_map_expr(Parser& p, const TokenSet& recovery) {
    TIRO_NOT_IMPLEMENTED();
    (void) p;
    (void) recovery;
}

Parser::CompletedMarker parse_set_expr(Parser& p, const TokenSet& recovery) {
    TIRO_NOT_IMPLEMENTED();
    (void) p;
    (void) recovery;
}

} // namespace tiro::next
