#include "compiler/syntax/parse_expr.hpp"

#include "compiler/syntax/operators.hpp"

namespace tiro::next {

static std::optional<Parser::CompletedMarker>
parse_expr(Parser& p, int bp, const TokenSet& recovery);

static Parser::CompletedMarker parse_infix_expr(
    Parser& p, Parser::CompletedMarker c, const InfixOperator& op, const TokenSet& recovery);

static std::optional<Parser::CompletedMarker>
parse_prefix_expr(Parser& p, const TokenSet& recovery);

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
static Parser::CompletedMarker parse_string_expr(Parser& p, const TokenSet& recovery);

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
///      http://journal.stuffwithstuff.com/2011/03/19/pratt-parsers-expression-parsing-made-easy/
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

        lhs = parse_infix_expr(p, *lhs, *op, recovery);
    }

    return lhs;
}

Parser::CompletedMarker parse_infix_expr(
    Parser& p, Parser::CompletedMarker c, const InfixOperator& op, const TokenSet& recovery) {

    auto m = c.precede();
    switch (p.current()) {

    // Member access a.b or a?.b
    case TokenType::Dot:
    case TokenType::QuestionDot: {
        p.advance();

        auto name = p.start();
        if (p.at(TokenType::Identifier) || p.at(TokenType::TupleField)) {
            p.advance();
        } else {
            p.error("expected a member name or number");
        }
        name.complete(SyntaxType::Name);
        return m.complete(SyntaxType::MemberExpr);
    }

    // Array access a[b] or a?[b]
    case TokenType::LeftBracket:
    case TokenType::QuestionLeftBracket: {
        p.advance();
        parse_expr(p, TokenType::RightBracket);
        p.expect(TokenType::RightBracket);
        return m.complete(SyntaxType::IndexExpr);
    }

    // Function call, a(b) or a?(b)
    case TokenType::LeftParen:
    case TokenType::QuestionLeftParen: {
        auto args = p.start();
        p.advance();
        while (!p.at(TokenType::RightParen) && !p.at(TokenType::Eof)) {
            if (!parse_expr(p, recovery.union_with({TokenType::Comma, TokenType::RightParen})))
                break;

            if (!p.at(TokenType::RightParen) && !p.expect(TokenType::Comma))
                break;
        }
        p.expect(TokenType::RightParen);
        args.complete(SyntaxType::ArgList);
        return m.complete(SyntaxType::CallExpr);
    }

    // Normal binary operator
    default: {
        p.advance();
        int next_bp = op.precedence;
        if (!op.right_assoc)
            ++next_bp;
        parse_expr(p, next_bp, recovery);
        return m.complete(SyntaxType::BinaryExpr);
    }
    }
}

std::optional<Parser::CompletedMarker> parse_prefix_expr(Parser& p, const TokenSet& recovery) {
    if (!p.at_any(UNARY_OP_FIRST)) {
        return parse_primary_expr(p, recovery);
    }

    auto m = p.start();
    p.advance();
    parse_expr(p, unary_precedence, recovery);
    return m.complete(SyntaxType::UnaryExpr);
}

std::optional<Parser::CompletedMarker> parse_primary_expr(Parser& p, const TokenSet& recovery) {
    if (auto c = parse_literal(p))
        return *c;

    switch (p.current()) {

    // { stmts ... }
    case TokenType::LeftBrace:
        return parse_block_expr(p, recovery);

    // (expr) or record or tuple
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
        return m.complete(SyntaxType::Name);
    }

    case TokenType::KwFunc:
        return parse_func_expr(p, recovery);

    case TokenType::LeftBracket:
        return parse_array_expr(p, recovery);

    case TokenType::MapStart:
        return parse_map_expr(p, recovery);

    case TokenType::SetStart:
        return parse_set_expr(p, recovery);

    case TokenType::StringStart:
        return parse_string_expr(p, recovery);

    default:
        p.error_recover("expected an expression", recovery);
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
    TIRO_DEBUG_ASSERT(p.at(TokenType::LeftParen), "Not at the start of a paren expression.");

    auto m = p.start();
    p.advance();

    // () is the empty tuple
    if (p.accept(TokenType::RightParen))
        return m.complete(SyntaxType::TupleExpr);

    // (:) is the empty record
    if (p.accept(TokenType::Colon)) {
        p.expect(TokenType::RightParen);
        return m.complete(SyntaxType::RecordExpr);
    }

    // Either:
    // - a grouped expression, e.g. "(expr)"
    // - a non-empty tuple literal, e.g. "(expr,)" or "(exprA, exprB)" and so on
    // - a non-empty record literal, e.g. "(a: expr, b: expr)"
    bool is_empty = true;
    bool is_record = false;
    bool has_comma = false;
    while (!p.at_any({TokenType::Eof, TokenType::RightParen})) {
        is_empty = false;

        if (!parse_expr(p, recovery.union_with({
                               TokenType::Comma,
                               TokenType::Colon,
                               TokenType::RightParen,
                           })))
            break;

        if (is_record || p.at(TokenType::Colon)) {
            p.expect(TokenType::Colon);
            is_record = true;
            if (!parse_expr(p, recovery.union_with({TokenType::Comma, TokenType::RightParen})))
                break;
        }

        if (!p.at(TokenType::RightParen)) {
            p.expect(TokenType::Comma);
            has_comma = true;
        }
    }

    p.expect(TokenType::RightParen);
    return m.complete(
        is_record ? SyntaxType::RecordExpr
                  : !is_empty && !has_comma ? SyntaxType::GroupedExpr : SyntaxType::TupleExpr);
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
    TIRO_DEBUG_ASSERT(p.at(TokenType::LeftBracket), "Not at the start of an array.");

    auto m = p.start();
    p.advance();
    while (!p.at_any({TokenType::Eof, TokenType::RightBracket})) {
        if (!parse_expr(p, recovery.union_with({TokenType::Comma, TokenType::RightBracket})))
            break;

        if (!p.at(TokenType::RightBracket) && !p.expect(TokenType::Comma)) {
            break;
        }
    }
    p.expect(TokenType::RightBracket);
    return m.complete(SyntaxType::ArrayExpr);
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

Parser::CompletedMarker parse_string_expr(Parser& p, const TokenSet& recovery) {
    TIRO_DEBUG_ASSERT(p.at(TokenType::StringStart), "Not at the start of a string.");

    auto string = p.start();
    p.advance();

    while (!p.at_any({TokenType::Eof, TokenType::StringEnd})) {
        switch (p.current()) {
        // Literal string
        case TokenType::StringContent:
            p.advance();
            break;

        // $var
        case TokenType::StringVar: {
            auto item = p.start();
            p.advance();

            auto name = p.start();
            p.expect(TokenType::Identifier);
            name.complete(SyntaxType::Name);

            item.complete(SyntaxType::StringFormatItem);
            break;
        }

        // ${ expr }
        case TokenType::StringBlockStart: {
            auto block = p.start();
            p.advance();
            parse_expr(p, recovery.union_with(TokenType::StringBlockEnd));
            p.expect(TokenType::StringBlockEnd);
            block.complete(SyntaxType::StringFormatBlock);
            break;
        }

        default:
            p.error_recover("expected string content", recovery.union_with({
                                                           TokenType::StringContent,
                                                           TokenType::StringVar,
                                                           TokenType::StringBlockStart,
                                                           TokenType::StringEnd,
                                                       }));
            break;
        }
    }

    p.expect(TokenType::StringEnd);
    return string.complete(SyntaxType::StringExpr);
}

} // namespace tiro::next
