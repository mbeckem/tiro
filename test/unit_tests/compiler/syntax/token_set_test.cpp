#include <catch2/catch.hpp>

#include "compiler/syntax/token_set.hpp"

#include <set>

namespace tiro::test {

TEST_CASE("TokenSet sets should behave like containers of token type enum values", "[token-set]") {
    TokenSet set;
    REQUIRE(set.size() == 0);
    REQUIRE(set.empty());
    REQUIRE_FALSE(set.contains(TokenType::EqualsEquals));

    set.insert(TokenType::EqualsEquals);
    REQUIRE(set.contains(TokenType::EqualsEquals));
    REQUIRE(set.size() == 1);
    REQUIRE_FALSE(set.empty());

    set.insert(TokenType::Dot);
    REQUIRE(set.contains(TokenType::Dot));
    REQUIRE(set.size() == 2);

    set.remove(TokenType::EqualsEquals);
    REQUIRE_FALSE(set.contains(TokenType::EqualsEquals));
    REQUIRE(set.size() == 1);

    set.remove(TokenType::Dot);
    REQUIRE(set.size() == 0);
}

TEST_CASE("TokenSet should support set operations", "[token-set]") {
    const TokenSet a{TokenType::EqualsEquals, TokenType::Dot, TokenType::Minus};
    const TokenSet b{TokenType::EqualsEquals, TokenType::Eof};

    const TokenSet expected_union{
        TokenType::EqualsEquals, TokenType::Dot, TokenType::Minus, TokenType::Eof};
    REQUIRE(a.union_with(b) == expected_union);

    const TokenSet expected_intersection{TokenType::EqualsEquals};
    REQUIRE(a.intersection_with(b) == expected_intersection);
}

TEST_CASE("TokenSet should support iteration", "[token-set]") {
    const TokenSet set{TokenType::Eof, TokenType::Integer, TokenType::Dot, TokenType::BitwiseXor};

    const std::set<TokenType> expected{
        TokenType::Eof, TokenType::Integer, TokenType::Dot, TokenType::BitwiseXor};
    const std::set<TokenType> got(set.begin(), set.end());
    REQUIRE(got == expected);
}

} // namespace tiro::test
