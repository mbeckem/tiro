#include <catch2/catch.hpp>

#include "vm/context.hpp"
#include "vm/handles/scope.hpp"
#include "vm/objects/exception.hpp"

namespace tiro::vm::test {

static bool starts_with(std::string_view str, std::string_view prefix) {
    return str.size() >= prefix.size()
           && std::equal(str.begin(), str.begin() + prefix.size(), prefix.begin(), prefix.end());
}

TEST_CASE("Exceptions should be constructible from strings", "[exception]") {
    Context ctx;
    Scope sc(ctx);

    Local message = sc.local(String::make(ctx, "Ooops!"));
    Local exception = sc.local(Exception::make(ctx, message));
    REQUIRE(exception->message().view() == "Ooops!");
}

TEST_CASE("Exceptions should be constructible from format strings", "[exception]") {
    Context ctx;
    Scope sc(ctx);

    Local exception = sc.local(TIRO_FORMAT_EXCEPTION(ctx, "Test {0}{1}{2}!", 1, 2, 3));
    REQUIRE(starts_with(exception->message().view(), "Test 123!"));
}

TEST_CASE("Fallible<T> can contain exceptions", "[exception]") {
    Context ctx;
    Scope sc(ctx);

    Local message = sc.local(String::make(ctx, "Ooops!"));
    Local exception = sc.local(Exception::make(ctx, message));
    Fallible<HeapInteger> fallible(exception);
    REQUIRE(fallible.has_exception());
    REQUIRE(!fallible);
    REQUIRE(!fallible.has_value());
    REQUIRE(fallible.exception().same(*exception));
}

TEST_CASE("Fallible<T> can contain values", "[exception]") {
    Context ctx;
    Scope sc(ctx);
    Local string = sc.local(String::make(ctx, "Hello"));
    Fallible<String> fallible(string);
    REQUIRE(!fallible.has_exception());
    REQUIRE(fallible);
    REQUIRE(fallible.has_value());
    REQUIRE(fallible.value().same(*string));
}

TEST_CASE("is_fallible<T> recognizes fallible types", "[exception]") {
    STATIC_REQUIRE(is_fallible<Fallible<int>>);
    STATIC_REQUIRE(is_fallible<Fallible<void>>);
    STATIC_REQUIRE(is_fallible<Fallible<Number>>);

    STATIC_REQUIRE_FALSE(is_fallible<int>);
    STATIC_REQUIRE_FALSE(is_fallible<void>);
    STATIC_REQUIRE_FALSE(is_fallible<Number>);
}

} // namespace tiro::vm::test
