#include <catch2/catch.hpp>

#include "tiropp/functions.hpp"
#include "tiropp/vm.hpp"

#include "helpers.hpp"
#include "matchers.hpp"

static tiro::handle simple_sync_function(tiro::vm& vm, [[maybe_unused]] tiro::sync_frame& frame) {
    return tiro::make_integer(vm, 123);
};

TEST_CASE("tiro::function should store sync functions", "[api]") {
    tiro::vm vm;
    tiro::function func = tiro::make_sync_function<simple_sync_function>(
        vm, tiro::make_string(vm, "func"), 0, tiro::make_null(vm));
    REQUIRE(func.kind() == tiro::value_kind::function);

    tiro::handle value = run_sync(vm, func, tiro::make_null(vm)).value();
    REQUIRE(value.kind() == tiro::value_kind::integer);
    REQUIRE(value.as<tiro::integer>().value() == 123);
}

static tiro::handle simple_throwing_sync_function(tiro::vm&, tiro::sync_frame&) {
    throw std::runtime_error("some error message");
}

TEST_CASE("tiro::function should translate exceptions from sync functions into panics", "[api]") {
    tiro::vm vm;
    tiro::function func = tiro::make_sync_function<simple_throwing_sync_function>(
        vm, tiro::make_string(vm, "func"), 0, tiro::make_null(vm));
    REQUIRE(func.kind() == tiro::value_kind::function);

    tiro::handle error = run_sync(vm, func, tiro::make_null(vm)).error();
    REQUIRE(error.kind() == tiro::value_kind::exception);
    std::string message = error.as<tiro::exception>().message().value();
    REQUIRE(message == "some error message");
}

static void simple_async_function(tiro::vm& vm, tiro::async_frame frame) {
    frame.return_value(tiro::make_integer(vm, 456));
}

TEST_CASE("tiro::function should store async functions", "[api]") {
    tiro::vm vm;
    tiro::function func = tiro::make_async_function<simple_async_function>(
        vm, tiro::make_string(vm, "func"), 0, tiro::make_null(vm));
    REQUIRE(func.kind() == tiro::value_kind::function);

    tiro::handle value = run_sync(vm, func, tiro::make_null(vm)).value();
    REQUIRE(value.kind() == tiro::value_kind::integer);
    REQUIRE(value.as<tiro::integer>().value() == 456);
}

static void simple_panicking_async_function(tiro::vm&, tiro::async_frame frame) {
    frame.panic_msg("some error message");
}

TEST_CASE("tiro::function should support panics from async functions", "[api]") {
    tiro::vm vm;
    tiro::function func = tiro::make_async_function<simple_panicking_async_function>(
        vm, tiro::make_string(vm, "func"), 0, tiro::make_null(vm));
    REQUIRE(func.kind() == tiro::value_kind::function);

    tiro::handle error = run_sync(vm, func, tiro::make_null(vm)).error();
    REQUIRE(error.kind() == tiro::value_kind::exception);
    std::string message = error.as<tiro::exception>().message().value();
    REQUIRE(message == "some error message");
}
