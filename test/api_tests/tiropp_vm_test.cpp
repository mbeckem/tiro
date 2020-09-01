#include <catch2/catch.hpp>

#include "tiropp/vm.hpp"

#include "./helpers.hpp"

TEST_CASE("tiropp::vm should not be movable", "[api]") {
    STATIC_REQUIRE(!std::is_move_constructible_v<tiro::vm>);
    STATIC_REQUIRE(!std::is_move_assignable_v<tiro::vm>);
}

TEST_CASE("tiropp::vm should be constructible", "[api]") {
    tiro::vm vm;
    REQUIRE(vm.raw_vm() != nullptr);
}

TEST_CASE("tiropp::vm references should be obtainable from a raw vm pointer", "[api]") {
    tiro::vm vm;
    tiro::vm& converted = tiro::vm::unsafe_from_raw_vm(vm.raw_vm());
    REQUIRE(&vm == &converted);
}

TEST_CASE("tiropp::vm should support arbitrary userdata", "[api]") {
    tiro::vm vm;
    vm.userdata() = static_cast<double>(123);
    REQUIRE(std::any_cast<double>(const_cast<const tiro::vm&>(vm).userdata()) == 123);
}

TEST_CASE("tiropp::vm should be able to load bytecode modules", "[api]") {
    tiro::vm vm;
    load_test(vm, "export const foo = 123;");

    tiro::handle foo = tiro::get_export(vm, "test", "foo");
    REQUIRE(foo.as<tiro::integer>().value() == 123);
}
