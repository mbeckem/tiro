#include <catch2/catch.hpp>

#include "tiropp/functions.hpp"
#include "tiropp/vm.hpp"

#include "helpers.hpp"
#include "matchers.hpp"

static tiro::handle simple_sync_function(tiro::vm& vm, [[maybe_unused]] tiro::sync_frame& frame) {
    return tiro::make_integer(vm, 123);
};

static tiro::handle simple_throwing_sync_function(tiro::vm&, tiro::sync_frame&) {
    throw std::runtime_error("some error message");
}

static void simple_async_function(tiro::vm& vm, tiro::async_frame& frame) {
    frame.return_value(tiro::make_integer(vm, 456));
}

static void simple_panicking_async_function(tiro::vm&, tiro::async_frame& frame) {
    frame.panic_msg("some error message");
}

static void simple_resumable_function(tiro::vm& vm, tiro::resumable_frame& frame) {
    switch (frame.state()) {
    case tiro::resumable_frame::start:
        return frame.set_state(1);
    case 1: {
        auto value = tiro::make_string(vm, "hello world");
        return frame.return_value(value);
    }
    default:
        return frame.panic_msg("unexpected state");
    }
}

static void simple_resumable_function_with_locals(tiro::vm& vm, tiro::resumable_frame& frame) {
    switch (frame.state()) {
    case tiro::resumable_frame::start:
        frame.set_local(0, tiro::make_string(vm, "hello world"));
        return frame.set_state(1);
    case 1:
        return frame.return_value(frame.local(0));
    default:
        return frame.panic_msg("unexpected state");
    }
}

static void simple_panicking_resumable_function(tiro::vm&, tiro::resumable_frame& frame) {
    switch (frame.state()) {
    case tiro::resumable_frame::start:
        return frame.panic_msg("some error message");
    default:
        return frame.panic_msg("unexpected state");
    }
}

TEST_CASE("tiro::function should store sync functions", "[api]") {
    tiro::vm vm;
    tiro::function func = tiro::make_sync_function<simple_sync_function>(
        vm, tiro::make_string(vm, "func"), 0, tiro::make_null(vm));
    REQUIRE(func.kind() == tiro::value_kind::function);

    tiro::handle value = run_sync(vm, func, tiro::make_null(vm)).value();
    REQUIRE(value.kind() == tiro::value_kind::integer);
    REQUIRE(value.as<tiro::integer>().value() == 123);
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

TEST_CASE("tiro::function should store async functions", "[api]") {
    tiro::vm vm;
    tiro::function func = tiro::make_async_function<simple_async_function>(
        vm, tiro::make_string(vm, "func"), 0, tiro::make_null(vm));
    REQUIRE(func.kind() == tiro::value_kind::function);

    tiro::handle value = run_sync(vm, func, tiro::make_null(vm)).value();
    REQUIRE(value.kind() == tiro::value_kind::integer);
    REQUIRE(value.as<tiro::integer>().value() == 456);
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

TEST_CASE("tiro::function should store resumable functions", "[api]") {
    tiro::vm vm;
    tiro::function func = tiro::make_resumable_function<simple_resumable_function>(
        vm, tiro::make_string(vm, "func"), 0, 0, tiro::make_null(vm));
    REQUIRE(func.kind() == tiro::value_kind::function);

    tiro::handle value = run_sync(vm, func, tiro::make_null(vm)).value();
    REQUIRE(value.kind() == tiro::value_kind::string);
    REQUIRE(value.as<tiro::string>().value() == "hello world");
}

TEST_CASE("tiro::function should support panics from resumable functions", "[api]") {
    tiro::vm vm;
    tiro::function func = tiro::make_resumable_function<simple_panicking_resumable_function>(
        vm, tiro::make_string(vm, "func"), 0, 0, tiro::make_null(vm));
    REQUIRE(func.kind() == tiro::value_kind::function);

    tiro::handle error = run_sync(vm, func, tiro::make_null(vm)).error();
    REQUIRE(error.kind() == tiro::value_kind::exception);
    std::string message = error.as<tiro::exception>().message().value();
    REQUIRE(message == "some error message");
}

TEST_CASE("tiro::function should support access to locals", "[api]") {
    tiro::vm vm;
    tiro::function func = tiro::make_resumable_function<simple_resumable_function_with_locals>(
        vm, tiro::make_string(vm, "func"), 0, 1, tiro::make_null(vm));
    REQUIRE(func.kind() == tiro::value_kind::function);

    tiro::handle value = run_sync(vm, func, tiro::make_null(vm)).value();
    REQUIRE(value.kind() == tiro::value_kind::string);
    REQUIRE(value.as<tiro::string>().value() == "hello world");
}
