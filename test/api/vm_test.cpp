#include <catch2/catch.hpp>

#include "tiro/api.h"

#include "./helpers.hpp"

using namespace tiro::api;

static void load_test(tiro_vm* vm, const char* source) {
    Compiler compiler(tiro_compiler_new(nullptr));
    Error error;

    tiro_compiler_add_file(compiler, "test", source, error.out());
    error.check();

    tiro_compiler_run(compiler, error.out());
    error.check();

    REQUIRE(tiro_compiler_has_module(compiler));

    Module module;
    tiro_compiler_take_module(compiler, module.out(), error.out());
    error.check();

    tiro_vm_load_std(vm, error.out());
    error.check();

    tiro_vm_load(vm, module, error.out());
    error.check();
}

TEST_CASE("Frame construction should return null if vm is null", "[api]") {
    Frame frame(allow_null, tiro_frame_new(nullptr, 123));
    REQUIRE(frame.get() == nullptr);
}

TEST_CASE("Frames should be constructible", "[api]") {
    VM vm(tiro_vm_new(nullptr));

    Frame frame(tiro_frame_new(vm, 123));
    REQUIRE(frame.get() != nullptr);
    REQUIRE(tiro_frame_size(frame) == 123);

    tiro_handle a = tiro_frame_slot(frame, 0);
    REQUIRE(a != nullptr);

    tiro_handle b = tiro_frame_slot(frame, 1);
    REQUIRE(b != nullptr);

    REQUIRE(a != b);
}

TEST_CASE("Accessing an invalid slot returns null", "[api]") {
    VM vm(tiro_vm_new(nullptr));
    Frame frame(tiro_frame_new(vm, 1));
    tiro_handle handle = tiro_frame_slot(frame, 1);
    REQUIRE(handle == nullptr);
}

TEST_CASE("Frame slots should be null by default", "[api]") {
    VM vm(tiro_vm_new(nullptr));
    Frame frame(tiro_frame_new(vm, 1));
    tiro_handle handle = tiro_frame_slot(frame, 0);
    REQUIRE(tiro_get_kind(handle) == TIRO_KIND_NULL);
}

TEST_CASE("Exported functions should be found", "[api]") {
    VM vm(tiro_vm_new(nullptr));
    load_test(vm, "export func foo() { return 0; }");

    Frame frame(tiro_frame_new(vm, 1));
    tiro_handle handle = tiro_frame_slot(frame, 0);

    tiro_errc errc = tiro_vm_find_function(vm, "test", "foo", handle, nullptr);
    REQUIRE(errc == TIRO_OK);
    REQUIRE(tiro_get_kind(handle) == TIRO_KIND_FUNCTION);
}

TEST_CASE("Appropriate error code should be returned if module does not exist", "[api]") {
    VM vm(tiro_vm_new(nullptr));
    load_test(vm, "export func foo() { return 0; }");

    Frame frame(tiro_frame_new(vm, 1));
    tiro_handle handle = tiro_frame_slot(frame, 0);

    tiro_errc errc = tiro_vm_find_function(vm, "qux", "foo", handle, nullptr);
    REQUIRE(errc == TIRO_ERROR_MODULE_NOT_FOUND);
}

TEST_CASE("Appropriate error code should be returned if function does not exist", "[api]") {
    VM vm(tiro_vm_new(nullptr));
    load_test(vm, "export func foo() { return 0; }");

    Frame frame(tiro_frame_new(vm, 1));
    tiro_handle handle = tiro_frame_slot(frame, 0);

    tiro_errc errc = tiro_vm_find_function(vm, "test", "bar", handle, nullptr);
    REQUIRE(errc == TIRO_ERROR_FUNCTION_NOT_FOUND);
}

TEST_CASE("Functions should be callable", "[api]") {
    VM vm(tiro_vm_new(nullptr));
    load_test(vm, "export func foo() { return 123; }");

    Frame frame(tiro_frame_new(vm, 3));
    tiro_handle function = tiro_frame_slot(frame, 0);
    tiro_handle arguments = tiro_frame_slot(frame, 1);
    tiro_handle result = tiro_frame_slot(frame, 2);

    tiro_errc errc = tiro_vm_find_function(vm, "test", "foo", function, nullptr);
    REQUIRE(errc == TIRO_OK);

    SECTION("With a null handle") {
        Error error;
        tiro_vm_call(vm, function, nullptr, result, error.out());
        error.check();
        REQUIRE(tiro_get_kind(result) == TIRO_KIND_INTEGER);
        REQUIRE(tiro_integer_value(result) == 123);
    }

    SECTION("With a handle pointing to null") {
        Error error;
        tiro_vm_call(vm, function, arguments, result, error.out());
        error.check();
        REQUIRE(tiro_get_kind(result) == TIRO_KIND_INTEGER);
        REQUIRE(tiro_integer_value(result) == 123);
    }

    // TODO: Requires tuple API
    // SECTION("With a zero sized tuple") {
    //    tiro_errc call_error = tiro_vm_call(vm, function, nullptr, result, nullptr);
    //    REQUIRE(call_error == TIRO_OK);
    //    REQUIRE(tiro_get_kind(result) == TIRO_KIND_INTEGER);
    // }
}

TEST_CASE("Integer construction should fail if parameters are invalid", "[api]") {
    VM vm(tiro_vm_new(nullptr));
    Frame frame(tiro_frame_new(vm, 1));
    tiro_handle result = tiro_frame_slot(frame, 0);

    SECTION("Invalid vm") {
        tiro_errc errc = tiro_make_integer(nullptr, 12345, result, nullptr);
        REQUIRE(errc == TIRO_ERROR_BAD_ARG);
    }

    SECTION("Invalid handle") {
        tiro_errc errc = tiro_make_integer(vm, 12345, nullptr, nullptr);
        REQUIRE(errc == TIRO_ERROR_BAD_ARG);
    }
}

TEST_CASE("Integers should be constructible", "[api]") {
    VM vm(tiro_vm_new(nullptr));
    Error error;
    Frame frame(tiro_frame_new(vm, 1));

    tiro_handle result = tiro_frame_slot(frame, 0);
    tiro_errc errc = tiro_make_integer(vm, 12345, result, error.out());
    error.check();
    REQUIRE(errc == TIRO_OK);

    REQUIRE(tiro_get_kind(result) == TIRO_KIND_INTEGER);
    REQUIRE(tiro_integer_value(result) == 12345);
}

TEST_CASE("Float construction should fail if parameters are invalid", "[api]") {
    VM vm(tiro_vm_new(nullptr));
    Frame frame(tiro_frame_new(vm, 1));
    tiro_handle result = tiro_frame_slot(frame, 0);

    SECTION("Invalid vm") {
        tiro_errc errc = tiro_make_float(nullptr, 12345, result, nullptr);
        REQUIRE(errc == TIRO_ERROR_BAD_ARG);
    }

    SECTION("Invalid handle") {
        tiro_errc errc = tiro_make_float(vm, 12345, nullptr, nullptr);
        REQUIRE(errc == TIRO_ERROR_BAD_ARG);
    }
}

TEST_CASE("Floats should be constructible", "[api]") {
    VM vm(tiro_vm_new(nullptr));
    Error error;
    Frame frame(tiro_frame_new(vm, 1));

    tiro_handle result = tiro_frame_slot(frame, 0);
    tiro_errc errc = tiro_make_float(vm, 123.456, result, error.out());
    error.check();
    REQUIRE(errc == TIRO_OK);

    REQUIRE(tiro_get_kind(result) == TIRO_KIND_FLOAT);
    REQUIRE(tiro_float_value(result) == 123.456);
}
