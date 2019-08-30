#include <catch.hpp>

#include "hammer/compiler/token.hpp"

#include <set>

using namespace hammer;

TEST_CASE("token type modifies operations", "[token]") {
    TokenTypes set;
    REQUIRE(set.size() == 0);
    REQUIRE(set.empty());
    REQUIRE_FALSE(set.contains(TokenType::eqeq));

    set.insert(TokenType::eqeq);
    REQUIRE(set.contains(TokenType::eqeq));
    REQUIRE(set.size() == 1);
    REQUIRE_FALSE(set.empty());

    set.insert(TokenType::dot);
    REQUIRE(set.contains(TokenType::dot));
    REQUIRE(set.size() == 2);

    set.remove(TokenType::eqeq);
    REQUIRE_FALSE(set.contains(TokenType::eqeq));
    REQUIRE(set.size() == 1);

    set.remove(TokenType::dot);
    REQUIRE(set.size() == 0);
}

TEST_CASE("token type set operations", "[token]") {
    const TokenTypes a{TokenType::eqeq, TokenType::dot, TokenType::minus};
    const TokenTypes b{TokenType::eqeq, TokenType::eof};

    const TokenTypes expected_union{TokenType::eqeq, TokenType::dot, TokenType::minus,
                                    TokenType::eof};
    REQUIRE(a.union_with(b) == expected_union);

    const TokenTypes expected_intersection{TokenType::eqeq};
    REQUIRE(a.intersection_with(b) == expected_intersection);
}

TEST_CASE("token type set iteration", "[token]") {
    const TokenTypes set{TokenType::eof, TokenType::integer_literal, TokenType::dot,
                         TokenType::bxor};

    const std::set<TokenType> expected{TokenType::eof, TokenType::integer_literal, TokenType::dot,
                                       TokenType::bxor};
    const std::set<TokenType> got(set.begin(), set.end());
    REQUIRE(got == expected);
}
