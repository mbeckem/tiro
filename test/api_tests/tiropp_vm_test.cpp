#include <catch2/catch.hpp>

#include "tiropp/vm.hpp"

#include "./helpers.hpp"

static tiro::compiled_module test_compile(const char* module_name, const char* source) {
    tiro::compiler compiler(module_name);
    compiler.add_file("main", source);
    compiler.run();
    return compiler.take_module();
}

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

TEST_CASE("tiropp::vm should be able to load module objects", "[api]") {
    tiro::vm vm;
    tiro::module module = tiro::make_module(vm, "test", {{"foo", tiro::make_integer(vm, 123)}});
    tiro::load_module(vm, module);

    REQUIRE(tiro::get_export(vm, "test", "foo").as<tiro::integer>().value() == 123);
}

TEST_CASE("tiropp::vm should support stdout redirection", "[api]") {
    std::vector<std::string> messages;

    tiro::vm_settings settings;
    settings.print_stdout = [&](std::string_view view) { messages.push_back(std::string(view)); };

    tiro::vm vm(settings);
    vm.load_std();
    vm.load(test_compile("test", R"(
        import std;

        export func main() {
            std.print("Hello");
            std.print("World");
        }
    )"));

    tiro::function main = tiro::get_export(vm, "test", "main").as<tiro::function>();
    tiro::coroutine coro = tiro::make_coroutine(vm, main);
    coro.start();
    vm.run_ready();

    REQUIRE(messages.size());
    REQUIRE(messages[0] == "Hello\n");
    REQUIRE(messages[1] == "World\n");
}
