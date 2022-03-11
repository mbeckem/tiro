#include <catch2/catch.hpp>

#include "tiropp/compiler.hpp"

#include "./matchers.hpp"

TEST_CASE("tiropp::severity should wrap tiro_severity_t", "[api]") {
    REQUIRE(static_cast<tiro_severity_t>(tiro::severity::error) == TIRO_SEVERITY_ERROR);
    REQUIRE(tiro::to_string(tiro::severity::error) == tiro_severity_str(TIRO_SEVERITY_ERROR));
}

TEST_CASE("tiro::compiler should return compiled modules", "[api]") {
    tiro::compiler comp("test");
    REQUIRE(!comp.has_module());

    comp.add_file("foo", "export func bar() {}");
    comp.run();

    REQUIRE(comp.has_module());
    tiro::compiled_module module = comp.take_module();
    REQUIRE(module.raw_module() != nullptr);
}

TEST_CASE("tiro::compiler should throw if run is called more than once", "[api]") {
    tiro::compiler comp("test");
    REQUIRE(!comp.has_module());

    comp.add_file("foo", "export func bar() {}");
    comp.run();

    REQUIRE_THROWS_MATCHES(comp.run(), tiro::api_error, throws_code(tiro::api_errc::bad_state));
}

TEST_CASE("tiro::compiler should support multiple files in a module", "[api]") {
    tiro::compiler comp("test");
    comp.add_file("foo", "export func foo() {}");
    comp.add_file("bar", "export func bar() {}");
    comp.run();

    REQUIRE(comp.has_module());
    tiro::compiled_module module = comp.take_module();
    REQUIRE(module.raw_module() != nullptr);
}

TEST_CASE("tiro::compiler should throw if file names are not unique", "[api]") {
    tiro::compiler comp("test");
    comp.add_file("foo", "export func foo() {}");
    comp.add_file("foo", "export func bar() {}");
    REQUIRE_THROWS_MATCHES(comp.run(), tiro::api_error, throws_code(tiro::api_errc::bad_arg));
}

TEST_CASE("tiro::compiler should throw if files are added after run", "[api]") {
    tiro::compiler comp("test");
    REQUIRE(!comp.has_module());

    comp.add_file("foo", "export func bar() {}");
    comp.run();

    REQUIRE_THROWS_MATCHES(comp.add_file("foo", "export func bar() {}"), tiro::api_error,
        throws_code(tiro::api_errc::bad_state));
}

TEST_CASE("tiro::compiler throws on get_attachment when not configured", "[api]") {
    tiro::compiler comp("test");
    comp.add_file("foo", "export func bar() {}");
    comp.run();

    REQUIRE_THROWS_MATCHES(comp.get_attachment(tiro::attachment::cst), tiro::api_error,
        throws_code(tiro::api_errc::bad_state));
    REQUIRE_THROWS_MATCHES(comp.get_attachment(tiro::attachment::ast), tiro::api_error,
        throws_code(tiro::api_errc::bad_state));
    REQUIRE_THROWS_MATCHES(comp.get_attachment(tiro::attachment::ir), tiro::api_error,
        throws_code(tiro::api_errc::bad_state));
    REQUIRE_THROWS_MATCHES(comp.get_attachment(tiro::attachment::bytecode), tiro::api_error,
        throws_code(tiro::api_errc::bad_state));
}

TEST_CASE("tiro::compiler supports dump_* when configured", "[api]") {
    tiro::compiler comp("test");
    comp.request_attachment(tiro::attachment::cst);
    comp.request_attachment(tiro::attachment::ast);
    comp.request_attachment(tiro::attachment::ir);
    comp.request_attachment(tiro::attachment::bytecode);
    comp.add_file("foo", "export func bar() {}");
    comp.run();

    REQUIRE(comp.get_attachment(tiro::attachment::cst) != "");
    REQUIRE(comp.get_attachment(tiro::attachment::ast) != "");
    REQUIRE(comp.get_attachment(tiro::attachment::ir) != "");
    REQUIRE(comp.get_attachment(tiro::attachment::bytecode) != "");
}

TEST_CASE("tiro::compiler supports move construction", "[api]") {
    tiro::compiler source("test");
    tiro_compiler_t raw_source = source.raw_compiler();

    tiro::compiler target = std::move(source);
    REQUIRE(source.raw_compiler() == nullptr);
    REQUIRE(target.raw_compiler() == raw_source);
}

TEST_CASE("tiro::compiler supports move assignment", "[api]") {
    tiro::compiler source("test");
    tiro_compiler_t raw_source = source.raw_compiler();

    tiro::compiler target("test");
    target = std::move(source);
    REQUIRE(source.raw_compiler() == nullptr);
    REQUIRE(target.raw_compiler() == raw_source);
}

TEST_CASE("tiro::compiler supports custom message callbacks", "[api]") {
    std::vector<std::string> messages;

    tiro::compiler comp("test");
    comp.set_message_callback([&](const tiro::compiler_message& message) {
        messages.push_back(std::string(message.text));
    });
    comp.add_file("foo", "export func bar() {");
    REQUIRE_THROWS_MATCHES(comp.run(), tiro::api_error, throws_code(tiro::api_errc::bad_source));

    REQUIRE(messages.size() == 1);
    REQUIRE(messages[0] == "expected '}'");
}

TEST_CASE("tiro::compiler supports throwing message callbacks", "[api]") {
    struct custom_exception : std::runtime_error {
        custom_exception()
            : runtime_error("my custom exception") {}
    };

    int called = 0;

    tiro::compiler comp("test");
    comp.set_message_callback([&](const tiro::compiler_message&) {
        ++called;
        throw custom_exception();
    });
    comp.add_file("foo", "export func bar() {a b c d f e");
    REQUIRE_THROWS_MATCHES(comp.run(), custom_exception, Catch::Message("my custom exception"));
    REQUIRE(called == 1);
}
