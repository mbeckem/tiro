#include "compiler/syntax/grammar/misc.hpp"

#include "common/assert.hpp"
#include "compiler/syntax/grammar/expr.hpp"
#include "compiler/syntax/parser.hpp"

namespace tiro {

const TokenSet VAR_FIRST = {
    TokenType::KwConst,
    TokenType::KwVar,
};

const TokenSet BINDING_PATTERN_FIRST = {
    TokenType::LeftParen,
    TokenType::Identifier,
};

void parse_name(Parser& p, const TokenSet& recovery) {
    if (!p.at(TokenType::Identifier)) {
        p.error_recover("expected a name", recovery);
        return;
    }

    auto m = p.start();
    p.expect(TokenType::Identifier);
    m.complete(SyntaxType::Name);
}

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

void parse_param_list(Parser& p, const TokenSet& recovery) {
    if (!p.at_any({TokenType::LeftParen, TokenType::QuestionLeftParen})) {
        p.error("expected a parameter list");
        return;
    }

    auto args = p.start();
    p.advance();
    while (!p.at_any({TokenType::RightParen, TokenType::Eof})) {
        if (!p.accept(TokenType::Identifier)) {
            p.error_recover("expected a function parameter name",
                recovery.union_with({TokenType::Comma, TokenType::RightParen}));
        }

        if (!p.at(TokenType::RightParen) && !p.expect(TokenType::Comma))
            break;
    }
    p.expect(TokenType::RightParen);
    args.complete(SyntaxType::ParamList);
}

FunctionKind
parse_func(Parser& p, const TokenSet& recovery, std::optional<CompletedMarker> modifiers) {
    if (!p.at(TokenType::KwFunc)) {
        if (modifiers) {
            auto m = modifiers->precede();
            p.error("expected a function declaration");
            m.complete(SyntaxType::Error);
            return FunctionKind::Error;
        }

        p.error("expected a function declaration");
        return FunctionKind::Error;
    }

    auto m = modifiers ? modifiers->precede() : p.start();
    p.advance(); // func kw

    // Optional name
    if (p.at(TokenType::Identifier)) {
        parse_name(p, recovery.union_with(TokenType::LeftParen));
    }
    parse_param_list(p, recovery.union_with(TokenType::LeftBrace));

    FunctionKind kind = FunctionKind::BlockBody;
    if (p.accept(TokenType::Equals) && !p.at(TokenType::LeftBrace)) {
        kind = FunctionKind::ShortExprBody;
        parse_expr(p, recovery);
    } else {
        parse_block_expr(p, recovery);
    }
    m.complete(SyntaxType::Func);
    return kind;
}

static void parse_var_decl_unchecked(
    Parser& p, const TokenSet& recovery, std::optional<CompletedMarker> modifiers) {
    TIRO_DEBUG_ASSERT(p.at_any(VAR_FIRST), "Not at the start of a var declaration.");

    auto m = modifiers ? modifiers->precede() : p.start();
    p.advance(); // var | const

    while (!p.at(TokenType::Eof)) {
        parse_binding(p, recovery.union_with(TokenType::Comma));
        if (!p.accept(TokenType::Comma))
            break;
    }

    m.complete(SyntaxType::Var);
}

void parse_var(Parser& p, const TokenSet& recovery, std::optional<CompletedMarker> modifiers) {
    if (!p.at_any(VAR_FIRST)) {
        if (modifiers) {
            auto m = modifiers->precede();
            p.error("expected a variable declaration");
            m.complete(SyntaxType::Error);
            return;
        }

        p.error("expected a variable declaration");
        return;
    }

    parse_var_decl_unchecked(p, recovery, modifiers);
}

void parse_binding_pattern(Parser& p, const TokenSet& recovery) {
    // Parse left hand side
    switch (p.current()) {
    case TokenType::LeftParen: {
        auto lhs = p.start();
        p.advance();
        while (!p.at_any({TokenType::Eof, TokenType::RightParen})) {
            if (!p.accept(TokenType::Identifier)) {
                p.error("expected a variable name");
                break;
            }

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

void parse_condition(Parser& p, const TokenSet& recovery) {
    auto cond = p.start();
    parse_expr_no_block(p, recovery);
    cond.complete(SyntaxType::Condition);
}

} // namespace tiro
