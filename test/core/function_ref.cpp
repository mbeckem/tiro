#include <catch.hpp>

#include "hammer/core/function_ref.hpp"

using namespace hammer;

TEST_CASE(
    "function ref should invoke the passed function object", "[function-ref]") {
    int i = 3;
    auto return_int = [&](int j) { return i + j; };

    FunctionRef<int(int)> ref = return_int;
    REQUIRE(ref(4) == 7);
}
