#include <catch2/catch.hpp>

#include "tiropp/compiler.hpp"

#include "./matchers.hpp"

TEST_CASE("tiropp::severity should wrap tiro_severity_t", "[api]") {
    REQUIRE(static_cast<tiro_severity_t>(tiro::severity::error) == TIRO_SEVERITY_ERROR);
    REQUIRE(tiro::to_string(tiro::severity::error) == tiro_severity_str(TIRO_SEVERITY_ERROR));
}

TEST_CASE("tiro::compiler should return compiled modules", "[api]") {
    tiro::compiler comp;
    REQUIRE(!comp.has_module());

    comp.add_file("foo", "export func bar() {}");
    comp.run();

    REQUIRE(comp.has_module());
    tiro::module module = comp.take_module();
}

TEST_CASE("tiro::compiler throws on dump_* when not configured", "[api]") {
    tiro::compiler comp;
    comp.add_file("foo", "export func bar() {}");
    comp.run();

    REQUIRE_THROWS_MATCHES(
        comp.dump_ast(), tiro::api_error, throws_code(tiro::api_errc::bad_state));
    REQUIRE_THROWS_MATCHES(comp.dump_ir(), tiro::api_error, throws_code(tiro::api_errc::bad_state));
    REQUIRE_THROWS_MATCHES(
        comp.dump_bytecode(), tiro::api_error, throws_code(tiro::api_errc::bad_state));
}

TEST_CASE("tiro::compiler supports dump_* when configured", "[api]") {
    tiro::compiler_settings settings;
    settings.enable_dump_ast = true;
    settings.enable_dump_bytecode = true;
    settings.enable_dump_ir = true;

    tiro::compiler comp(settings);
    comp.add_file("foo", "export func bar() {}");
    comp.run();

    REQUIRE(comp.dump_ast() != "");
    REQUIRE(comp.dump_ir() != "");
    REQUIRE(comp.dump_bytecode() != "");
}

TEST_CASE("tiro::compiler supports move construction", "[api]") {
    tiro::compiler source;
    tiro_compiler_t raw_source = source.raw_compiler();

    tiro::compiler target = std::move(source);
    REQUIRE(source.raw_compiler() == nullptr);
    REQUIRE(target.raw_compiler() == raw_source);
}

TEST_CASE("tiro::compiler supports move assignment", "[api]") {
    tiro::compiler source;
    tiro_compiler_t raw_source = source.raw_compiler();

    tiro::compiler target;
    target = std::move(source);
    REQUIRE(source.raw_compiler() == nullptr);
    REQUIRE(target.raw_compiler() == raw_source);
}
