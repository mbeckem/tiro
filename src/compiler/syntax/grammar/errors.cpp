#include "compiler/syntax/grammar/errors.hpp"

#include "common/assert.hpp"
#include "compiler/syntax/parser.hpp"
#include "compiler/syntax/token.hpp"
#include "compiler/syntax/token_set.hpp"

#include <absl/container/inlined_vector.h>

namespace tiro {

const TokenSet NESTING_START = {
    TokenType::LeftBrace,
    TokenType::StringBlockStart,
    TokenType::StringStart,
};

const TokenSet NESTING_END = {
    TokenType::LeftBrace,
    TokenType::StringBlockEnd,
    TokenType::StringEnd,
};

static std::string_view unexpected_message(TokenType type) {
    TIRO_DEBUG_ASSERT(NESTING_START.contains(type), "invalid nesting token");
    switch (type) {
    case TokenType::LeftBrace:
    case TokenType::StringBlockStart:
        return "unexpected block"sv;
    case TokenType::StringStart:
        return "unexpected string"sv;
    default:
        break;
    }
    TIRO_UNREACHABLE("invalid nesting token");
}

static void discard_block_impl(Parser& p) {
    auto closing_token = [&](TokenType t) {
        switch (t) {
        case TokenType::LeftBrace:
            return TokenType::RightBrace;
        case TokenType::StringBlockStart:
            return TokenType::StringBlockEnd;
        case TokenType::StringStart:
            return TokenType::StringEnd;
        default:
            TIRO_UNREACHABLE("Invalid nesting token");
        }
    };

    absl::InlinedVector<TokenType, 16> stack;
    stack.push_back(closing_token(p.current()));
    p.advance();

    while (!p.at(TokenType::Eof) && !stack.empty()) {
        if (auto nested = p.accept_any(NESTING_START)) {
            stack.push_back(*nested);
            continue;
        }

        if (p.at(stack.back()))
            stack.pop_back();

        p.advance();
    }
}

void discard_nested(Parser& p) {
    TIRO_DEBUG_ASSERT(p.at_any(NESTING_START), "Not at the start of a nested block.");

    auto m = p.start();
    p.error(std::string(unexpected_message(p.current())));
    discard_block_impl(p);
    m.complete(SyntaxType::Error);
}

void discard_input(Parser& p, const TokenSet& recovery) {
    while (!p.at(TokenType::Eof)) {
        if (p.at_any(NESTING_START)) {
            discard_block_impl(p);
            continue;
        }

        if (p.at_any(recovery))
            break;

        p.advance();
    }
}

} // namespace tiro
