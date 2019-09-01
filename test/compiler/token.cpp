#include <catch.hpp>

#include "hammer/compiler/token.hpp"

#include <set>

using namespace hammer;

TEST_CASE("token type modifies operations", "[token]") {
    TokenTypes set;
    REQUIRE(set.size() == 0);
    REQUIRE(set.empty());
    REQUIRE_FALSE(set.contains(TokenType::EqEq));

    set.insert(TokenType::EqEq);
    REQUIRE(set.contains(TokenType::EqEq));
    REQUIRE(set.size() == 1);
    REQUIRE_FALSE(set.empty());

    set.insert(TokenType::Dot);
    REQUIRE(set.contains(TokenType::Dot));
    REQUIRE(set.size() == 2);

    set.remove(TokenType::EqEq);
    REQUIRE_FALSE(set.contains(TokenType::EqEq));
    REQUIRE(set.size() == 1);

    set.remove(TokenType::Dot);
    REQUIRE(set.size() == 0);
}

TEST_CASE("token type set operations", "[token]") {
    const TokenTypes a{TokenType::EqEq, TokenType::Dot, TokenType::Minus};
    const TokenTypes b{TokenType::EqEq, TokenType::Eof};

    const TokenTypes expected_union{
        TokenType::EqEq, TokenType::Dot, TokenType::Minus, TokenType::Eof};
    REQUIRE(a.union_with(b) == expected_union);

    const TokenTypes expected_intersection{TokenType::EqEq};
    REQUIRE(a.intersection_with(b) == expected_intersection);
}

TEST_CASE("token type set iteration", "[token]") {
    const TokenTypes set{TokenType::Eof, TokenType::IntegerLiteral,
        TokenType::Dot, TokenType::BXor};

    const std::set<TokenType> expected{TokenType::Eof,
        TokenType::IntegerLiteral, TokenType::Dot, TokenType::BXor};
    const std::set<TokenType> got(set.begin(), set.end());
    REQUIRE(got == expected);
}
