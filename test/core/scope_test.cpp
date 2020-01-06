#include <catch.hpp>

#include "hammer/core/scope.hpp"

using namespace hammer;

TEST_CASE("ScopeExit should execute on scope exit", "[scope]") {
    int i = 0;
    {
        ScopeExit exit = [&] { i = 1; };
        REQUIRE(i == 0);
    }
    REQUIRE(i == 1);
}

TEST_CASE("ScopeExit should not execute when disabled", "[scope]") {
    int i = 0;
    {
        ScopeExit exit = [&] { i = 1; };
        REQUIRE(i == 0);

        exit.disable();
        REQUIRE_FALSE(exit.enabled());
    }
    REQUIRE(i == 0);
}

TEST_CASE(
    "ScopeSuccess should execute when "
    "scope is left normally",
    "[scope]") {

    int i = 0;
    {
        ScopeSuccess exit = [&] { i = 1; };
        REQUIRE(i == 0);
    }

    REQUIRE(i == 1);
}

TEST_CASE(
    "ScopeSuccess should not execute "
    "when disabled",
    "[scope]") {

    int i = 0;
    {
        ScopeExit exit = [&] { i = 1; };
        REQUIRE(i == 0);

        exit.disable();
        REQUIRE_FALSE(exit.enabled());
    }

    REQUIRE(i == 0);
}

TEST_CASE(
    "ScopeSuccess should not execute "
    "when scope is left with exception",
    "[scope]") {

    int i = 0;
    try {
        ScopeSuccess exit = [&] { i = 1; };
        REQUIRE(i == 0);

        throw 0;
    } catch (...) {
    }

    REQUIRE(i == 0);
}
