#include <catch2/catch.hpp>

#include <stdexcept>

#include "common/scope_guards.hpp"

namespace tiro::test {

TEST_CASE("Scope guards should throw exceptions when not unwinding", "[scope_guard]") {
    auto f = [&]() { ScopeExit exit = [&]() { throw 0; }; };
    REQUIRE_THROWS_AS(f(), int);
}

TEST_CASE("Scope guards should not throw exceptions if already unwinding", "[scope_guard]") {
    int i = 0;
    auto f = [&]() {
        ScopeExit exit = [&]() {
            i = 1;
            throw 0;
        };
        throw std::runtime_error("Runtime error");
    };
    REQUIRE_THROWS_AS(f(), std::runtime_error);
    REQUIRE(i == 1);
}

TEST_CASE("ScopeExit should execute on scope exit", "[scope_guards]") {
    int i = 0;
    {
        ScopeExit exit = [&] { i = 1; };
        REQUIRE(i == 0);
    }
    REQUIRE(i == 1);
}

TEST_CASE("ScopeSuccess should execute when scope is left normally", "[scope_guards]") {

    int i = 0;
    {
        ScopeSuccess exit = [&] { i = 1; };
        REQUIRE(i == 0);
    }

    REQUIRE(i == 1);
}

TEST_CASE("ScopeSuccess should not execute when scope is left with exception", "[scope_guards]") {

    int i = 0;
    try {
        ScopeSuccess exit = [&] { i = 1; };
        REQUIRE(i == 0);

        throw 0;
    } catch (...) {
    }

    REQUIRE(i == 0);
}

TEST_CASE(
    "ScopeSuccess should execute when located in an active catch block if the scope itself is "
    "successful",
    "[scope_guards]") {
    int i = 0;
    try {
        ScopeExit exit = [&]() {
            REQUIRE(std::uncaught_exceptions() == 1);
            ScopeSuccess succ = [&]() { i = 1; };
        };

        throw 0;
    } catch (...) {
    }
    REQUIRE(i == 1);
}

TEST_CASE("ScopeFailure should not execute when the scope is exited normally", "[scope_guards]") {

    int i = 0;
    {
        ScopeFailure fail = [&]() { i = 1; };
    }
    REQUIRE(i == 0);
}

TEST_CASE(
    "ScopeFailure should execute when the scope is left through an exception", "[scope_guards]") {
    int i = 0;
    try {
        ScopeFailure fail = [&]() { i = 1; };
        throw 0;
    } catch (...) {
    }
    REQUIRE(i == 1);
}

} // namespace tiro::test
