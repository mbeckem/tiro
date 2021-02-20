#include "compiler/syntax/parse_misc.hpp"

#include "common/assert.hpp"
#include "compiler/syntax/parse_expr.hpp"
#include "compiler/syntax/parser.hpp"

namespace tiro::next {

static void parse_var_decl_unchecked(Parser& p, const TokenSet& recovery);

const TokenSet VAR_DECL_FIRST = {
    TokenType::KwConst,
    TokenType::KwVar,
};

const TokenSet BINDING_PATTERN_FIRST = {
    TokenType::LeftParen,
    TokenType::Identifier,
};

void parse_arg_list(Parser& p, const TokenSet& recovery) {
    if (!p.at_any({TokenType::LeftParen, TokenType::QuestionLeftParen})) {
        p.error("expected an argument list");
        return;
    }

    auto args = p.start();
    p.advance();
    while (!p.at_any({TokenType::RightParen, TokenType::Eof})) {
        if (!parse_expr(p, recovery.union_with({TokenType::Comma, TokenType::RightParen})))
            break;

        if (!p.at(TokenType::RightParen) && !p.expect(TokenType::Comma))
            break;
    }
    p.expect(TokenType::RightParen);
    args.complete(SyntaxType::ArgList);
}

void parse_var_decl(Parser& p, const TokenSet& recovery) {
    if (!p.at_any(VAR_DECL_FIRST)) {
        p.error("expected a variable declaration");
        return;
    }

    parse_var_decl_unchecked(p, recovery);
}

void parse_var_decl_unchecked(Parser& p, const TokenSet& recovery) {
    TIRO_DEBUG_ASSERT(p.at_any(VAR_DECL_FIRST), "Not at the start of a var declaration.");

    auto m = p.start();
    p.advance(); // var | const

    while (!p.at(TokenType::Eof)) {
        parse_binding(p, recovery.union_with(TokenType::Comma));
        if (!p.accept(TokenType::Comma))
            break;
    }

    m.complete(SyntaxType::VarDecl);
}

void parse_binding_pattern(Parser& p, const TokenSet& recovery) {
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
        p.error_recover("expected a variable name or a tuple pattern", recovery);
        break;
    }
    }
}

void parse_binding(Parser& p, const TokenSet& recovery) {
    auto m = p.start();

    // Parse left hand side
    parse_binding_pattern(p, recovery.union_with(TokenType::Equals));

    // Parse initializer expression
    if (p.accept(TokenType::Equals))
        parse_expr(p, recovery);

    m.complete(SyntaxType::Binding);
}

} // namespace tiro::next
