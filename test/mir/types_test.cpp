#include <catch.hpp>

#include "tiro/mir/types.hpp"

#include <cmath>

using namespace tiro::compiler::mir;

TEST_CASE("Floating point constants should support equality", "[mir-types]") {
    REQUIRE(FloatConstant(1) == FloatConstant(1));
    REQUIRE(FloatConstant(2) != FloatConstant(3));

    REQUIRE(FloatConstant(1) < FloatConstant(2));
    REQUIRE(FloatConstant(1) <= FloatConstant(2));

    REQUIRE(FloatConstant(3) > FloatConstant(2));
    REQUIRE(FloatConstant(3) >= FloatConstant(2));
}

TEST_CASE("Nan constants must be equal", "[mir-types]") {
    const double n1 = std::nan("1");
    const double n2 = std::nan("2");

    REQUIRE(FloatConstant(n1) == FloatConstant(n2));
    REQUIRE(!(FloatConstant(n1) != FloatConstant(n2)));

    REQUIRE(!(FloatConstant(n1) < FloatConstant(n2)));
    REQUIRE(FloatConstant(n1) <= FloatConstant(n2));

    REQUIRE(!(FloatConstant(n1) > FloatConstant(n2)));
    REQUIRE(FloatConstant(n1) >= FloatConstant(n2));
}
