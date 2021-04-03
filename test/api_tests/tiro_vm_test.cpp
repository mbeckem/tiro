#include <catch2/catch.hpp>

#include "tiro/api.h"
#include "tiropp/api.hpp"

#include "./helpers.hpp"

static void load_program(tiro_vm_t vm, const char* module, const char* source) {
    tiro::compiler compiler;
    compiler.add_file(module, source);
    compiler.run();
    tiro::compiled_module compiled = compiler.take_module();

    tiro_vm_load_std(vm, tiro::error_adapter());
    tiro_vm_load_bytecode(vm, compiled.raw_module(), tiro::error_adapter());
}

static tiro::handle require_value(const tiro::handle& result) {
    REQUIRE(result.kind() == tiro::value_kind::result);
    return result.as<tiro::result>().value();
}

TEST_CASE("Virtual machine supports userdata", "[api]") {
    tiro_vm_settings_t settings;
    tiro_vm_settings_init(&settings);
    REQUIRE(settings.userdata == nullptr);

    struct Holder {
        tiro_vm_t vm = nullptr;
        ~Holder() { tiro_vm_free(vm); }
    };

    SECTION("Null when not set") {
        Holder holder;
        tiro_vm_t& vm = holder.vm = tiro_vm_new(&settings, tiro::error_adapter());
        REQUIRE(vm != nullptr);
        REQUIRE(tiro_vm_userdata(vm) == nullptr);
    }

    SECTION("Can be set to an arbitrary value") {
        int dummy = 1;
        settings.userdata = &dummy;

        Holder holder;
        tiro_vm_t& vm = holder.vm = tiro_vm_new(&settings, tiro::error_adapter());
        REQUIRE(vm != nullptr);
        REQUIRE(tiro_vm_userdata(vm) == &dummy);
    }
}

TEST_CASE("The virtual machine's standard output should support redirection", "[api]") {
    struct TestContext {
        std::exception_ptr caught_exception;
        std::vector<std::string> messages;
    };

    struct Holder {
        tiro_vm_t vm = nullptr;
        ~Holder() { tiro_vm_free(vm); }
    };

    TestContext ctx;

    tiro_vm_settings_t settings;
    tiro_vm_settings_init(&settings);
    settings.userdata = &ctx;
    settings.print_stdout = [](const char* message, size_t size, void* userdata) {
        auto& inner_ctx = *static_cast<TestContext*>(userdata);
        if (inner_ctx.caught_exception)
            return;

        try {
            inner_ctx.messages.push_back(std::string(message, size));
        } catch (const std::exception& e) {
            inner_ctx.caught_exception = std::current_exception();
        }
    };

    {
        Holder vm_holder;
        tiro_vm_t& vm = vm_holder.vm = tiro_vm_new(&settings, tiro::error_adapter());
        REQUIRE(vm != nullptr);
        REQUIRE(tiro_vm_userdata(vm) == &ctx);

        load_program(vm, "test", R"(
            import std;

            export func main() {
                std.print("Hello");
                std.print("World");
            }
        )");

        // TODO: Sync call api should be removed
        tiro_handle_t function = tiro_global_new(vm, tiro::error_adapter());
        tiro_vm_get_export(vm, "test", "main", function, tiro::error_adapter());
        tiro_vm_call(vm, function, NULL, NULL, tiro::error_adapter());
    }

    if (ctx.caught_exception)
        std::rethrow_exception(ctx.caught_exception);

    REQUIRE(ctx.messages.size() == 2);
    REQUIRE(ctx.messages[0] == "Hello\n");
    REQUIRE(ctx.messages[1] == "World\n");
}

TEST_CASE("Virtual machine should support loading module objects", "[api]") {
    tiro::vm vm;
    tiro::module module = tiro::make_module(vm, "test", {{"foo", tiro::make_integer(vm, 123)}});

    tiro_vm_load_module(vm.raw_vm(), module.raw_handle(), tiro::error_adapter());

    tiro::handle result = tiro::make_null(vm);
    tiro_vm_get_export(vm.raw_vm(), "test", "foo", result.raw_handle(), tiro::error_adapter());
    REQUIRE(result.as<tiro::integer>().value() == 123);
}

TEST_CASE("Attempt to load a module that already exists should result in an error", "[api]") {
    tiro::vm vm;
    tiro::module module = tiro::make_module(vm, "test", {{"foo", tiro::make_integer(vm, 123)}});

    tiro_vm_load_module(vm.raw_vm(), module.raw_handle(), tiro::error_adapter());

    tiro_errc_t errc = TIRO_OK;
    tiro_vm_load_module(vm.raw_vm(), module.raw_handle(), error_observer(errc));
    REQUIRE(errc == TIRO_ERROR_MODULE_EXISTS);
}

TEST_CASE("Exported functions should be found", "[api]") {
    tiro::vm vm;
    load_test(vm, "export func foo() { return 0; }");

    tiro::handle handle(vm.raw_vm());
    tiro_vm_get_export(vm.raw_vm(), "test", "foo", handle.raw_handle(), tiro::error_adapter());
    REQUIRE(tiro_value_kind(vm.raw_vm(), handle.raw_handle()) == TIRO_KIND_FUNCTION);
}

TEST_CASE("Appropriate error code should be returned if module does not exist", "[api]") {
    tiro::vm vm;
    load_test(vm, "export func foo() { return 0; }");

    tiro::handle handle(vm.raw_vm());
    tiro_errc_t errc = TIRO_OK;
    tiro_vm_get_export(vm.raw_vm(), "qux", "foo", handle.raw_handle(), error_observer(errc));
    REQUIRE(errc == TIRO_ERROR_MODULE_NOT_FOUND);
}

TEST_CASE("Appropriate error code should be returned if function does not exist", "[api]") {
    tiro::vm vm;
    load_test(vm, "export func foo() { return 0; }");

    tiro::handle handle(vm.raw_vm());
    tiro_errc_t errc = TIRO_OK;
    tiro_vm_get_export(vm.raw_vm(), "test", "bar", handle.raw_handle(), error_observer(errc));
    REQUIRE(errc == TIRO_ERROR_EXPORT_NOT_FOUND);
}

TEST_CASE("Functions should be callable", "[api]") {
    tiro::vm vm;
    load_test(vm, "export func foo() { return 123; }");

    tiro::function function = tiro::get_export(vm, "test", "foo").as<tiro::function>();
    tiro::handle result(vm.raw_vm());

    SECTION("With a null handle") {
        tiro_vm_call(vm.raw_vm(), function.raw_handle(), nullptr, result.raw_handle(),
            tiro::error_adapter());
        REQUIRE(require_value(result).as<tiro::integer>().value() == 123);
    }

    SECTION("With a handle pointing to null") {
        tiro::handle argument(vm.raw_vm());
        REQUIRE(tiro_value_kind(vm.raw_vm(), argument.raw_handle()) == TIRO_KIND_NULL);

        tiro_vm_call(vm.raw_vm(), function.raw_handle(), argument.raw_handle(), result.raw_handle(),
            tiro::error_adapter());
        REQUIRE(require_value(result).as<tiro::integer>().value() == 123);
    }

    SECTION("With a zero sized tuple") {
        tiro::tuple arguments = tiro::make_tuple(vm, 0);
        REQUIRE(tiro_value_kind(vm.raw_vm(), arguments.raw_handle()) == TIRO_KIND_TUPLE);

        tiro_vm_call(vm.raw_vm(), function.raw_handle(), arguments.raw_handle(),
            result.raw_handle(), tiro::error_adapter());
        REQUIRE(require_value(result).as<tiro::integer>().value() == 123);
    }
}

TEST_CASE("Function calls should support tuples as call arguments", "[api]") {
    tiro::vm vm;
    load_test(vm, "export func foo(a, b, c) = a * b + c;");

    tiro::handle function = tiro::get_export(vm, "test", "foo");

    tiro::tuple arguments = tiro::make_tuple(vm, 3);
    arguments.set(0, tiro::make_integer(vm, 5));
    arguments.set(1, tiro::make_integer(vm, 2));
    arguments.set(2, tiro::make_integer(vm, 7));

    tiro::handle result(vm.raw_vm());
    tiro_vm_call(vm.raw_vm(), function.raw_handle(), arguments.raw_handle(), result.raw_handle(),
        tiro::error_adapter());

    REQUIRE(require_value(result).as<tiro::integer>().value() == 17);
}

TEST_CASE("Allocation of global handles should succeed", "[api]") {
    tiro::vm vm;

    struct Holder {
        tiro_vm_t vm = nullptr;
        tiro_handle_t global = nullptr;

        ~Holder() { tiro_global_free(vm, global); }
    } h;
    h.vm = vm.raw_vm();
    h.global = tiro_global_new(vm.raw_vm(), tiro::error_adapter());

    tiro_handle_t& global = h.global;
    REQUIRE(global != nullptr);
    REQUIRE(tiro_value_kind(vm.raw_vm(), global) == TIRO_KIND_NULL);

    tiro_make_integer(vm.raw_vm(), 123, global, tiro::error_adapter());
    REQUIRE(tiro_value_kind(vm.raw_vm(), global) == TIRO_KIND_INTEGER);
    REQUIRE(tiro_integer_value(vm.raw_vm(), global) == 123);
}
