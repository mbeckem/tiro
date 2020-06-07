#include <catch.hpp>

#include "tiro/bytecode_gen/locations.hpp"

#include <algorithm>

using namespace tiro;

TEST_CASE("Empty bytecode locations should behave correctly", "[bytecode-locations]") {
    BytecodeLocation loc;
    REQUIRE(loc.size() == 0);
    REQUIRE(loc.empty());
    REQUIRE(loc.begin() == loc.end());
}

TEST_CASE("Single register bytecode locations should behave correctly", "[bytecode-locations]") {
    BytecodeLocation loc(BytecodeRegister(1));
    REQUIRE(loc.size() == 1);
    REQUIRE(!loc.empty());
    REQUIRE(loc[0] == BytecodeRegister(1));
}

TEST_CASE(
    "Bytecode locations constructed from a span should behave correctly", "[bytecode-locations]") {
    std::vector<BytecodeRegister> regs;
    for (u32 i = 0; i < BytecodeLocation::max_registers; ++i) {
        regs.push_back(BytecodeRegister(i));
    }

    BytecodeLocation loc(regs);
    REQUIRE(!loc.empty());
    REQUIRE(loc.size() == regs.size());
    REQUIRE(std::equal(loc.begin(), loc.end(), regs.begin(), regs.end()));
}
