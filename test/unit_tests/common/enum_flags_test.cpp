#include <catch2/catch.hpp>

#include "common/enum_flags.hpp"

using namespace tiro;

namespace {

enum class Props {
    None = 0,
    A = 1 << 0,
    B = 1 << 1,
    C = 1 << 2,
    All = A | B | C,
};

using flags_t = Flags<Props>;

}; // namespace

TEST_CASE("Empty flags should be initialized to 0", "[enum-flags]") {
    flags_t flags;
    REQUIRE(flags.raw() == 0);
    REQUIRE(flags.test(Props::None)); // set contains subset

    flags.clear(Props::C);
    REQUIRE(flags.raw() == 0);
}

TEST_CASE("Single-value flags should contain the specified value", "[enum-flags]") {
    flags_t flags(Props::B);
    REQUIRE(flags.raw() == static_cast<int>(Props::B));
    REQUIRE(flags.test(Props::B));
    REQUIRE_FALSE(flags.test(Props::A));
    REQUIRE_FALSE(flags.test(Props::All));
}

TEST_CASE("Enum flags should support setting and clearing flags", "[enum-flags]") {
    flags_t flags;

    flags.set(Props::A);
    REQUIRE(flags.test(Props::A));

    flags.set(Props::C);
    REQUIRE(flags.test(Props::C));

    flags.clear(Props::A);
    REQUIRE_FALSE(flags.test(Props::A));

    flags.set(Props::A);
    flags.set(Props::B);
    REQUIRE(flags.test(Props::All));
    REQUIRE(flags.test(Props::None));

    // No-op
    flags.clear(Props::None);
    REQUIRE(flags.test(Props::All));

    // Clear all
    flags.clear(Props::All);
    REQUIRE(flags.raw() == 0);
}

TEST_CASE("Enum flags should support clearning", "[enum-flags]") {
    flags_t flags;

    flags.set(Props::A);
    REQUIRE(flags.raw() == static_cast<int>(Props::A));

    flags.clear();
    REQUIRE(flags.raw() == 0);
}
