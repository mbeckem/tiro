#include <catch.hpp>

#include "tiro/ast/token_types.hpp"

#include <set>

using namespace tiro;

TEST_CASE("TokenTypes sets should behave like containers of token type enum values", "[token]") {
    TokenTypes set;
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

TEST_CASE("TokenTypes should support set operations", "[token]") {
    const TokenTypes a{TokenType::EqualsEquals, TokenType::Dot, TokenType::Minus};
    const TokenTypes b{TokenType::EqualsEquals, TokenType::Eof};

    const TokenTypes expected_union{
        TokenType::EqualsEquals, TokenType::Dot, TokenType::Minus, TokenType::Eof};
    REQUIRE(a.union_with(b) == expected_union);

    const TokenTypes expected_intersection{TokenType::EqualsEquals};
    REQUIRE(a.intersection_with(b) == expected_intersection);
}

TEST_CASE("TokenTypes should support iteration", "[token]") {
    const TokenTypes set{
        TokenType::Eof, TokenType::IntegerLiteral, TokenType::Dot, TokenType::BitwiseXor};

    const std::set<TokenType> expected{
        TokenType::Eof, TokenType::IntegerLiteral, TokenType::Dot, TokenType::BitwiseXor};
    const std::set<TokenType> got(set.begin(), set.end());
    REQUIRE(got == expected);
}
