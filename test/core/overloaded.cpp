#include <catch.hpp>

#include "hammer/core/overloaded.hpp"

using namespace hammer;

TEST_CASE("overloaded example", "[overloaded]") {
    int seen_int = 0;
    double seen_double = 0;

    auto visitor = Overloaded{[&](int i) {
                                  REQUIRE(seen_int == 0);
                                  seen_int = i;
                              },
        [&](double d) {
            REQUIRE(seen_double == 0);
            seen_double = d;
        }};
    visitor(4);   // int
    visitor(4.5); // double

    REQUIRE(seen_int == 4);
    REQUIRE(seen_double == 4.5);
}
