#include <catch.hpp>

#include "tiro/core/not_null.hpp"

using namespace tiro;

using nn = NotNull<int*>;
using cnn = NotNull<const int*>;

static_assert(std::is_constructible_v<nn, int*>);
static_assert(std::is_constructible_v<nn, nn>);
static_assert(std::is_assignable_v<nn, nn>);
static_assert(std::is_convertible_v<nn, int*>);
static_assert(std::is_assignable_v<nn, int*>);

static_assert(std::is_constructible_v<cnn, const int*>);
static_assert(std::is_constructible_v<cnn, nn>);
static_assert(std::is_constructible_v<cnn, int*>);
static_assert(std::is_convertible_v<cnn, const int*>);
static_assert(std::is_assignable_v<cnn, nn>);
static_assert(std::is_assignable_v<cnn, const int*>);

TEST_CASE("NotNull<T> behaviour", "[not-null]") {
    int a = 0;
    int b = 1;

    nn na = &a;
    nn nb = &b;

    REQUIRE(na != nullptr);
    REQUIRE(nb != nullptr);

    REQUIRE(na == na);
    REQUIRE(na != nb);

    REQUIRE(*na == a);
    REQUIRE(*nb == b);

    nb = na;
    REQUIRE(na == nb);
    REQUIRE(*nb == a);
}
