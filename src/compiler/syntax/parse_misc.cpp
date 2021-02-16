#include "compiler/syntax/parse_misc.hpp"

#include "compiler/syntax/parse_expr.hpp"
#include "compiler/syntax/parser.hpp"

namespace tiro::next {

void parse_arg_list(Parser& p, const TokenSet& recovery) {
    if (!p.at_any({TokenType::LeftParen, TokenType::QuestionLeftParen})) {
        p.error("Expected an argument list");
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

} // namespace tiro::next
