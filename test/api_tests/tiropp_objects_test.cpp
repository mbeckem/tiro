#include <catch2/catch.hpp>

#include "tiropp/objects.hpp"
#include "tiropp/vm.hpp"

#include "helpers.hpp"
#include "matchers.hpp"

#include <variant>

static void load_test_code(tiro::vm& vm) {
    tiro::compiler compiler("test");
    compiler.add_file("main", "export func test(a, b) { return a + b; }");
    compiler.run();
    vm.load_std();
    vm.load(compiler.take_module());
}

static tiro::record make_record(tiro::vm& vm, const std::vector<std::string>& keys) {
    tiro::array array = tiro::make_array(vm, keys.size());
    for (const auto& key : keys) {
        array.push(tiro::make_string(vm, key));
    }
    return tiro::make_record(vm, array);
}

TEST_CASE("tiro::handle should throw on invalid cast", "[api]") {
    tiro::vm vm;
    tiro::handle integer = tiro::make_integer(vm, 123);

    REQUIRE_THROWS_AS(integer.as<tiro::string>(), tiro::bad_handle_cast);
}

TEST_CASE("tiro::handle should support type conversions", "[api]") {
    tiro::vm vm;
    tiro::handle integer = tiro::make_integer(vm, 123);

    tiro::integer integer1 = integer.as<tiro::integer>();
    REQUIRE(integer1.value() == 123);

    tiro::integer integer2 = tiro::integer(integer);
    REQUIRE(integer2.value() == 123);
}

TEST_CASE("copy constructing tiro::handle should duplicate the value", "[api]") {
    tiro::vm vm;
    tiro::handle source = tiro::make_integer(vm, 123);
    tiro::handle target = source;
    REQUIRE(source.raw_vm() == target.raw_vm());
    REQUIRE(source.raw_handle() != target.raw_handle());
    REQUIRE(target.as<tiro::integer>().value() == 123);
}

TEST_CASE("copy assigning tiro::handle should override the value", "[api]") {
    tiro::vm vm;

    tiro::handle source = tiro::make_integer(vm, 123);
    tiro::handle target = tiro::make_integer(vm, 456);
    tiro_handle_t raw_target = target.raw_handle();

    target = source;
    REQUIRE(target.raw_handle() == raw_target); // Unchanged
    REQUIRE(target.as<tiro::integer>().value() == 123);
}

TEST_CASE("copy assigning tiro::handle with a different vm's handle should allocate a new handle",
    "[api]") {
    tiro::vm vm1;
    tiro::vm vm2;

    tiro::handle source = tiro::make_integer(vm1, 123);
    tiro::handle target = tiro::make_integer(vm2, 456);
    tiro_handle_t raw_target = target.raw_handle();

    target = source;
    REQUIRE(target.raw_vm() == vm1.raw_vm());
    REQUIRE(target.raw_handle() != raw_target); // Unchanged
    REQUIRE(target.as<tiro::integer>().value() == 123);
}

TEST_CASE("move constructing tiro::handle should transfer the raw handle", "[api]") {
    tiro::vm vm;

    tiro::handle source = tiro::make_integer(vm, 123);
    tiro_handle_t raw_source = source.raw_handle();
    REQUIRE(source.valid());
    REQUIRE(raw_source != nullptr);

    tiro::handle target = std::move(source);
    REQUIRE(target.valid());
    REQUIRE(target.raw_handle() == raw_source);
    REQUIRE(!source.valid());
    REQUIRE(source.raw_handle() == nullptr);

    REQUIRE(source.raw_vm() == target.raw_vm());
}

TEST_CASE("move assigning tiro::handle should overwrite the handle's state", "[api]") {
    tiro::vm vm;

    tiro::handle source = tiro::make_integer(vm, 123);
    tiro_handle_t raw_source = source.raw_handle();

    tiro::handle target = tiro::make_integer(vm, 456);
    target = std::move(source);
    REQUIRE(!source.valid());
    REQUIRE(target.valid());
    REQUIRE(target.raw_handle() == raw_source);
    REQUIRE(target.as<tiro::integer>().value() == 123);
}

TEST_CASE("explicit copy of tiro::handle should copy the inner value", "[api]") {
    tiro::vm vm;
    tiro::handle source = tiro::make_integer(vm, 123);
    tiro::handle target = tiro::make_copy(vm, source.raw_handle());
    REQUIRE(target.raw_vm() == vm.raw_vm());
    REQUIRE(target.as<tiro::integer>().value() == 123);
}

TEST_CASE("tiro::null should represent nulls", "[api]") {
    tiro::vm vm;
    tiro::null null = tiro::make_null(vm);
    REQUIRE(null.kind() == tiro::value_kind::null);
}

TEST_CASE("tiro::boolean should store boolean values", "[api]") {
    tiro::vm vm;
    tiro::boolean b = tiro::make_boolean(vm, false);
    REQUIRE(b.kind() == tiro::value_kind::boolean);
    REQUIRE(!b.value());
}

TEST_CASE("tiro::integer should store integer values", "[api]") {
    tiro::vm vm;
    tiro::integer i = tiro::make_integer(vm, 123);
    REQUIRE(i.kind() == tiro::value_kind::integer);
    REQUIRE(i.value() == 123);
}

TEST_CASE("tiro::float_ should store floating point values", "[api]") {
    tiro::vm vm;
    tiro::float_ f = tiro::make_float(vm, 1234.5);
    REQUIRE(f.kind() == tiro::value_kind::float_);
    REQUIRE(f.value() == 1234.5);
}

TEST_CASE("tiro::string should store strings", "[api]") {
    tiro::vm vm;
    tiro::string s = tiro::make_string(vm, "hello world");
    REQUIRE(s.kind() == tiro::value_kind::string);
    REQUIRE(s.value() == "hello world");
}

TEST_CASE("tiro::tuple should store tuples", "[api]") {
    tiro::vm vm;
    tiro::tuple tuple = tiro::make_tuple(vm, 3);
    REQUIRE(tuple.kind() == tiro::value_kind::tuple);
    REQUIRE(tuple.size() == 3);
}

TEST_CASE("tiro::tuple should support element access", "[api]") {
    tiro::vm vm;
    tiro::tuple tuple = tiro::make_tuple(vm, 3);

    // Null by default
    REQUIRE(tuple.get(2).kind() == tiro::value_kind::null);

    // Values can be altered
    tuple.set(2, tiro::make_integer(vm, 123));
    REQUIRE(tuple.get(2).as<tiro::integer>().value() == 123);
}

TEST_CASE("tiro::tuple should throw on out of bounds access", "[api]") {
    tiro::vm vm;
    tiro::tuple tuple = tiro::make_tuple(vm, 3);
    REQUIRE(tuple.size() == 3);
    REQUIRE_THROWS_MATCHES(
        tuple.get(3), tiro::api_error, throws_code(tiro::api_errc::out_of_bounds));
    REQUIRE_THROWS_MATCHES(tuple.set(3, tiro::make_null(vm)), tiro::api_error,
        throws_code(tiro::api_errc::out_of_bounds));
}

TEST_CASE("tiro::record should store records", "[api]") {
    tiro::vm vm;
    tiro::record record = make_record(vm, {"foo", "bar"});
    REQUIRE(record.kind() == tiro::value_kind::record);

    tiro::array keys = record.keys();
    REQUIRE(keys.size() == 2);
    REQUIRE(keys.get(0).as<tiro::string>().view() == "foo");
    REQUIRE(keys.get(1).as<tiro::string>().view() == "bar");
}

TEST_CASE("tiro::record should support element access", "[api]") {
    tiro::vm vm;
    tiro::record record = make_record(vm, {"foo", "bar"});
    tiro::string foo = tiro::make_string(vm, "foo");

    // Null by default
    REQUIRE(record.get(foo).kind() == tiro::value_kind::null);

    record.set(foo, tiro::make_integer(vm, 123));
    REQUIRE(record.get(foo).as<tiro::integer>().value() == 123);
}

TEST_CASE("tiro::record should throw for invalid keys", "[api]") {
    tiro::vm vm;
    tiro::record record = make_record(vm, {"foo", "bar"});
    tiro::string invalid = tiro::make_string(vm, "invalid");
    REQUIRE_THROWS_MATCHES(
        record.get(invalid), tiro::api_error, throws_code(tiro::api_errc::bad_key));
    REQUIRE_THROWS_MATCHES(record.set(invalid, tiro::make_null(vm)), tiro::api_error,
        throws_code(tiro::api_errc::bad_key));
}

TEST_CASE("tiro::array should store arrays", "[api]") {
    tiro::vm vm;
    tiro::array array = tiro::make_array(vm);
    REQUIRE(array.kind() == tiro::value_kind::array);
    REQUIRE(array.size() == 0);
}

TEST_CASE("tiro::array should support modifications", "[api]") {
    tiro::vm vm;
    tiro::array array = tiro::make_array(vm);
    array.push(tiro::make_integer(vm, 123));
    array.push(tiro::make_integer(vm, 456));
    REQUIRE(array.size() == 2);

    SECTION("pop removes the last element") {
        array.pop();
        REQUIRE(array.size() == 1);
        REQUIRE(array.get(0).as<tiro::integer>().value() == 123);
    }

    SECTION("push appends at then end") {
        array.push(tiro::make_integer(vm, 789));
        REQUIRE(array.size() == 3);
        REQUIRE(array.get(2).as<tiro::integer>().value() == 789);
    }

    SECTION("clear removes all elements") {
        array.clear();
        REQUIRE(array.size() == 0);
    }

    SECTION("set overrides the element") {
        array.set(0, tiro::make_integer(vm, -1));
        REQUIRE(array.get(0).as<tiro::integer>().value() == -1);
    }
}

TEST_CASE("tiro::array should throw on out of bounds access", "[api") {
    tiro::vm vm;
    tiro::array array = tiro::make_array(vm);
    array.push(tiro::make_integer(vm, 123));
    REQUIRE(array.size() == 1);
    REQUIRE_THROWS_MATCHES(
        array.get(1), tiro::api_error, throws_code(tiro::api_errc::out_of_bounds));
    REQUIRE_THROWS_MATCHES(array.set(1, tiro::make_null(vm)), tiro::api_error,
        throws_code(tiro::api_errc::out_of_bounds));
}

TEST_CASE("tiro::result should represent success", "[api]") {
    tiro::vm vm;
    tiro::result result = tiro::make_success(vm, tiro::make_integer(vm, 123));
    REQUIRE(result.is_success());
    REQUIRE(result.value().as<tiro::integer>().value() == 123);
    REQUIRE(!result.is_error());
    REQUIRE_THROWS_MATCHES(result.error(), tiro::api_error, throws_code(tiro::api_errc::bad_state));
}

TEST_CASE("tiro::result should represent failure", "[api]") {
    tiro::vm vm;
    tiro::result result = tiro::make_error(vm, tiro::make_integer(vm, 123));
    REQUIRE(result.is_error());
    REQUIRE(result.error().as<tiro::integer>().value() == 123);
    REQUIRE(!result.is_success());
    REQUIRE_THROWS_MATCHES(result.value(), tiro::api_error, throws_code(tiro::api_errc::bad_state));
}

TEST_CASE("tiro::exception should represent exceptions", "[api]") {
    tiro::vm vm;

    // There is currently no way to construct an exception from the c/c++ api
    load_test(vm, R"(
        import std;

        export func foo() {
            std.panic("nope!");
        }
    )");

    auto func = tiro::get_export(vm, "test", "foo").as<tiro::function>();
    auto exception = run_sync(vm, func, tiro::make_null(vm)).error().as<tiro::exception>();
    REQUIRE(exception.message().view() == "nope!");
}

TEST_CASE("tiro::coroutine should store coroutines", "[api]") {
    tiro::vm vm;
    load_test_code(vm);
    tiro::function func = tiro::get_export(vm, "test", "test").as<tiro::function>();

    SECTION("without arguments") {
        tiro::coroutine coro = tiro::make_coroutine(vm, func);
        REQUIRE(coro.kind() == tiro::value_kind::coroutine);
    }

    SECTION("with arguments") {
        tiro::tuple args = tiro::make_tuple(vm, 2);
        tiro::coroutine coro = tiro::make_coroutine(vm, func, args);
        REQUIRE(coro.kind() == tiro::value_kind::coroutine);
    }
}

TEST_CASE("tiro::coroutine should call callbacks", "[api]") {
    tiro::vm vm;
    load_test_code(vm);

    tiro::function func = tiro::get_export(vm, "test", "test").as<tiro::function>();
    tiro::tuple args = tiro::make_tuple(vm, 2);
    args.set(0, tiro::make_integer(vm, 4));
    args.set(1, tiro::make_integer(vm, 5));
    tiro::coroutine coro = tiro::make_coroutine(vm, func, args);

    std::variant<std::monostate, int64_t, std::exception_ptr> cb_result;
    coro.set_callback([&](tiro::vm& cb_vm, const tiro::coroutine& cb_coro) {
        try {
            REQUIRE(&cb_vm == &vm);
            REQUIRE(cb_coro.completed());

            auto result = cb_coro.result().as<tiro::result>();
            cb_result = result.value().as<tiro::integer>().value();
        } catch (...) {
            cb_result = std::current_exception();
        }
    });
    coro.start();
    REQUIRE(coro.started());

    vm.run_ready();
    if (auto ptr = std::get_if<std::exception_ptr>(&cb_result)) {
        std::rethrow_exception(*ptr);
    }
    REQUIRE(coro.completed());
    REQUIRE(std::holds_alternative<int64_t>(cb_result));
    REQUIRE(std::get<int64_t>(cb_result) == 9);
}

TEST_CASE("tiro::module should store modules", "[api]") {
    tiro::vm vm;
    tiro::module module = tiro::make_module(vm, "my_module",
        {
            {"foo", tiro::make_float(vm, 1234.5)},
        });
    REQUIRE(module.kind() == tiro::value_kind::module);
}

TEST_CASE("tiro::module should return exported members", "[api]") {
    tiro::vm vm;
    tiro::module module = tiro::make_module(vm, "my_module",
        {
            {"foo", tiro::make_float(vm, 1234.5)},
        });
    tiro::handle foo = module.get_export("foo");
    REQUIRE(foo.as<tiro::float_>().value() == 1234.5);
}

TEST_CASE("tiro::module should report non-existing module members", "[api]") {
    tiro::vm vm;
    tiro::module module = tiro::make_module(vm, "my_module",
        {
            {"foo", tiro::make_float(vm, 1234.5)},
        });
    REQUIRE_THROWS_MATCHES(
        module.get_export("bar"), tiro::api_error, throws_code(tiro::api_errc::export_not_found));
}

// TODO: Native objects.

TEST_CASE("tiro::tuple should return its name", "[api]") {
    tiro::vm vm;
    tiro::handle i = tiro::make_integer(vm, 123);
    tiro::handle b = tiro::make_boolean(vm, false);
    REQUIRE(i.type_of().name().value() == "Integer");
    REQUIRE(b.type_of().name().value() == "Boolean");
}
