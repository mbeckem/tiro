#include <catch2/catch.hpp>

#include "common/adt/not_null.hpp"

using namespace tiro;

using nn = NotNull<int*>;
using cnn = NotNull<const int*>;

static_assert(std::is_constructible_v<nn, GuaranteedNotNull, int*>);
static_assert(std::is_constructible_v<nn, nn>);
static_assert(std::is_assignable_v<nn, nn>);
static_assert(std::is_convertible_v<nn, int*>);

static_assert(std::is_constructible_v<cnn, GuaranteedNotNull, const int*>);
static_assert(std::is_constructible_v<cnn, GuaranteedNotNull, int*>);
static_assert(std::is_constructible_v<cnn, nn>);
static_assert(std::is_convertible_v<cnn, const int*>);
static_assert(std::is_assignable_v<cnn, nn>);

TEST_CASE("NotNull<T> behaviour", "[not-null]") {
    int a = 0;
    int b = 1;

    nn na = TIRO_NN(&a);
    nn nb = TIRO_NN(&b);

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

TEST_CASE("NotNull<T*> is constructible from T&", "[not-null]") {
    int value = 3;

    nn ptr(value);
    REQUIRE(ptr.get() == &value);

    const int cvalue = 4;

    cnn cptr(cvalue);
    REQUIRE(cptr.get() == &cvalue);

    cptr = cnn(value);
    REQUIRE(cptr.get() == &value);
}
