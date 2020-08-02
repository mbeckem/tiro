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

static void fill_array(tiro_vm* vm, tiro_handle array, size_t n) {
    Error error;
    Frame frame(tiro_frame_new(vm, 1));
    tiro_handle value = tiro_frame_slot(frame, 0);
    for (size_t i = 0; i < n; ++i) {
        tiro_make_integer(vm, i, value, error.out());
        error.check();

        tiro_array_push(vm, array, value, error.out());
        error.check();
    }

    REQUIRE(tiro_array_size(vm, array) == n);
}

TEST_CASE("Exported functions should be found", "[api]") {
    VM vm(tiro_vm_new(nullptr));
    load_test(vm, "export func foo() { return 0; }");

    Frame frame(tiro_frame_new(vm, 1));
    tiro_handle handle = tiro_frame_slot(frame, 0);

    tiro_errc errc = tiro_vm_find_function(vm, "test", "foo", handle, nullptr);
    REQUIRE(errc == TIRO_OK);
    REQUIRE(tiro_value_kind(vm, handle) == TIRO_KIND_FUNCTION);
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
        REQUIRE(tiro_value_kind(vm, result) == TIRO_KIND_INTEGER);
        REQUIRE(tiro_integer_value(vm, result) == 123);
    }

    SECTION("With a handle pointing to null") {
        Error error;
        tiro_vm_call(vm, function, arguments, result, error.out());
        error.check();
        REQUIRE(tiro_value_kind(vm, result) == TIRO_KIND_INTEGER);
        REQUIRE(tiro_integer_value(vm, result) == 123);
    }

    SECTION("With a zero sized tuple") {
        Error error;

        tiro_make_tuple(vm, 0, arguments, error.out());
        error.check();
        REQUIRE(tiro_value_kind(vm, arguments) == TIRO_KIND_TUPLE);

        tiro_errc call_error = tiro_vm_call(vm, function, arguments, result, nullptr);
        REQUIRE(call_error == TIRO_OK);
        REQUIRE(tiro_value_kind(vm, result) == TIRO_KIND_INTEGER);
        REQUIRE(tiro_integer_value(vm, result) == 123);
    }
}

TEST_CASE("Functions calls should support tuples as call arguments", "[api]") {
    Error error;
    VM vm(tiro_vm_new(nullptr));
    load_test(vm, "export func foo(a, b, c) = a * b + c");

    Frame frame(tiro_frame_new(vm, 4));
    tiro_handle function = tiro_frame_slot(frame, 0);
    tiro_handle arguments = tiro_frame_slot(frame, 1);
    tiro_handle result = tiro_frame_slot(frame, 2);

    auto set_integer = [&](size_t index, int64_t value) {
        tiro_handle handle = tiro_frame_slot(frame, 3);

        tiro_make_integer(vm, value, handle, error.out());
        error.check();

        tiro_tuple_set(vm, arguments, index, handle, error.out());
        error.check();
    };

    tiro_make_tuple(vm, 3, arguments, error.out());
    error.check();

    set_integer(0, 5);
    set_integer(1, 2);
    set_integer(2, 7);

    tiro_vm_find_function(vm, "test", "foo", function, error.out());
    error.check();

    tiro_vm_call(vm, function, arguments, result, error.out());
    error.check();

    REQUIRE(tiro_value_kind(vm, result) == TIRO_KIND_INTEGER);
    REQUIRE(tiro_integer_value(vm, result) == 17);
}

TEST_CASE("Null values should be constructible", "[api]") {
    Error error;
    VM vm(tiro_vm_new(nullptr));
    Frame frame(tiro_frame_new(vm, 1));
    tiro_handle result = tiro_frame_slot(frame, 0);

    tiro_make_integer(vm, 123, result, error.out());
    error.check();
    REQUIRE(tiro_value_kind(vm, result) == TIRO_KIND_INTEGER);

    tiro_make_null(vm, result);
    REQUIRE(tiro_value_kind(vm, result) == TIRO_KIND_NULL);
}

TEST_CASE("Boolean values should be constructible", "[api]") {
    Error error;
    VM vm(tiro_vm_new(nullptr));
    Frame frame(tiro_frame_new(vm, 1));
    tiro_handle result = tiro_frame_slot(frame, 0);

    auto test_value = [&](bool value) {
        tiro_make_boolean(vm, value, result, error.out());
        error.check();

        REQUIRE(tiro_value_kind(vm, result) == TIRO_KIND_BOOLEAN);
        REQUIRE(tiro_boolean_value(vm, result) == value);
    };

    SECTION("true") { test_value(true); }
    SECTION("false") { test_value(false); }
}

TEST_CASE("Boolean value retrieval should support conversions", "[api]") {
    Error error;
    VM vm(tiro_vm_new(nullptr));
    Frame frame(tiro_frame_new(vm, 10));

    tiro_handle null = tiro_frame_slot(frame, 0);
    tiro_make_null(vm, null);

    tiro_handle zero_int = tiro_frame_slot(frame, 1);
    tiro_make_integer(vm, 0, zero_int, error.out());
    error.check();

    tiro_handle zero_float = tiro_frame_slot(frame, 2);
    tiro_make_float(vm, 0, zero_float, error.out());
    error.check();

    REQUIRE(tiro_boolean_value(vm, null) == false);
    REQUIRE(tiro_boolean_value(vm, zero_int) == true);
    REQUIRE(tiro_boolean_value(vm, zero_float) == true);
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

TEST_CASE("Integer construction should succeed", "[api]") {
    VM vm(tiro_vm_new(nullptr));
    Error error;
    Frame frame(tiro_frame_new(vm, 1));
    tiro_handle result = tiro_frame_slot(frame, 0);
    tiro_errc errc = tiro_make_integer(vm, 12345, result, error.out());
    error.check();
    REQUIRE(errc == TIRO_OK);

    REQUIRE(tiro_value_kind(vm, result) == TIRO_KIND_INTEGER);
    REQUIRE(tiro_integer_value(vm, result) == 12345);
}

TEST_CASE("tiro_integer_value should convert floating point numbers to int", "[api]") {
    VM vm(tiro_vm_new(nullptr));
    Error error;
    Frame frame(tiro_frame_new(vm, 1));
    tiro_handle value = tiro_frame_slot(frame, 0);

    tiro_make_float(vm, 123.456, value, error.out());
    error.check();

    REQUIRE(tiro_value_kind(vm, value) == TIRO_KIND_FLOAT);
    REQUIRE(tiro_integer_value(vm, value) == 123);
}

TEST_CASE("tiro_integer_value should return 0 if the value is not a number", "[api]") {
    VM vm(tiro_vm_new(nullptr));
    Error error;
    Frame frame(tiro_frame_new(vm, 1));
    tiro_handle value = tiro_frame_slot(frame, 0);

    tiro_make_boolean(vm, true, value, error.out());
    error.check();

    REQUIRE(tiro_value_kind(vm, value) == TIRO_KIND_BOOLEAN);
    REQUIRE(tiro_integer_value(vm, value) == 0);
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

TEST_CASE("Float construction should succeeed", "[api]") {
    VM vm(tiro_vm_new(nullptr));
    Error error;
    Frame frame(tiro_frame_new(vm, 1));

    tiro_handle result = tiro_frame_slot(frame, 0);
    tiro_errc errc = tiro_make_float(vm, 123.456, result, error.out());
    error.check();
    REQUIRE(errc == TIRO_OK);

    REQUIRE(tiro_value_kind(vm, result) == TIRO_KIND_FLOAT);
    REQUIRE(tiro_float_value(vm, result) == 123.456);
}

TEST_CASE("tiro_float_value should convert integers to float", "[api]") {
    VM vm(tiro_vm_new(nullptr));
    Error error;
    Frame frame(tiro_frame_new(vm, 1));
    tiro_handle value = tiro_frame_slot(frame, 0);

    tiro_make_integer(vm, 123456, value, error.out());
    error.check();

    REQUIRE(tiro_value_kind(vm, value) == TIRO_KIND_INTEGER);
    REQUIRE(tiro_float_value(vm, value) == 123456);
}

TEST_CASE("tiro_float_value should return 0 if the value is not a float", "[api]") {
    VM vm(tiro_vm_new(nullptr));
    Error error;
    Frame frame(tiro_frame_new(vm, 1));
    tiro_handle value = tiro_frame_slot(frame, 0);

    tiro_make_boolean(vm, true, value, error.out());
    error.check();

    REQUIRE(tiro_value_kind(vm, value) == TIRO_KIND_BOOLEAN);
    REQUIRE(tiro_float_value(vm, value) == 0);
}

TEST_CASE("Tuple construction should succeed", "[api]") {
    VM vm(tiro_vm_new(nullptr));
    Error error;
    Frame frame(tiro_frame_new(vm, 1));

    auto construct = [&](size_t size) {
        tiro_handle result = tiro_frame_slot(frame, 0);
        tiro_errc err = tiro_make_tuple(vm, size, result, error.out());
        REQUIRE(err == TIRO_OK);
        error.check();

        REQUIRE(tiro_value_kind(vm, result) == TIRO_KIND_TUPLE);
        REQUIRE(tiro_tuple_size(vm, result) == size);
    };

    SECTION("Zero size tuple") { construct(0); }
    SECTION("Normal tuple") { construct(7); }
    SECTION("Huge tuple") { construct(1 << 15); }
}

TEST_CASE("Tuple elements should be initialized to null", "[api]") {
    VM vm(tiro_vm_new(nullptr));
    Error error;
    Frame frame(tiro_frame_new(vm, 2));
    tiro_handle tuple = tiro_frame_slot(frame, 0);
    tiro_handle element = tiro_frame_slot(frame, 1);

    tiro_make_tuple(vm, 123, tuple, error.out());
    error.check();

    REQUIRE(tiro_tuple_size(vm, tuple) == 123);
    for (size_t i = 0; i < 123; ++i) {
        CAPTURE(i);

        tiro_tuple_get(vm, tuple, i, element, error.out());
        error.check();

        REQUIRE(tiro_value_kind(vm, element) == TIRO_KIND_NULL);
    }
}

TEST_CASE("Tuple element access should report type errors", "[api]") {
    VM vm(tiro_vm_new(nullptr));
    Frame frame(tiro_frame_new(vm, 2));
    tiro_handle tuple = tiro_frame_slot(frame, 0);
    tiro_handle element = tiro_frame_slot(frame, 1);

    SECTION("Read access") {
        tiro_errc errc = tiro_tuple_get(vm, tuple, 0, element, nullptr);
        REQUIRE(errc == TIRO_ERROR_BAD_TYPE);
    }

    SECTION("Read access") {
        tiro_errc errc = tiro_tuple_set(vm, tuple, 0, element, nullptr);
        REQUIRE(errc == TIRO_ERROR_BAD_TYPE);
    }
}

TEST_CASE("Tuple element access should report out of bounds errors", "[api]") {
    VM vm(tiro_vm_new(nullptr));
    Error error;
    Frame frame(tiro_frame_new(vm, 2));
    tiro_handle tuple = tiro_frame_slot(frame, 0);
    tiro_handle element = tiro_frame_slot(frame, 1);

    tiro_make_tuple(vm, 4, tuple, error.out());
    error.check();

    tiro_make_integer(vm, 42, element, error.out());
    error.check();

    SECTION("Read access") {
        tiro_errc errc = tiro_tuple_get(vm, tuple, 4, element, nullptr);
        REQUIRE(errc == TIRO_ERROR_OUT_OF_BOUNDS);
        REQUIRE(tiro_integer_value(vm, element) == 42); // Not touched.
    }

    SECTION("Write access") {
        tiro_errc errc = tiro_tuple_set(vm, tuple, 4, element, nullptr);
        REQUIRE(errc == TIRO_ERROR_OUT_OF_BOUNDS);
        REQUIRE(tiro_integer_value(vm, element) == 42); // Not touched.
    }
}

TEST_CASE("Tuple elements should support assignment", "[api]") {
    VM vm(tiro_vm_new(nullptr));
    Error error;
    Frame frame(tiro_frame_new(vm, 3));
    tiro_handle tuple = tiro_frame_slot(frame, 0);
    tiro_handle input = tiro_frame_slot(frame, 1);
    tiro_handle output = tiro_frame_slot(frame, 2);

    const int64_t count = 5;
    tiro_make_tuple(vm, count, tuple, error.out());
    error.check();

    for (int64_t i = 0; i < count; ++i) {
        CAPTURE(i);

        tiro_make_integer(vm, i, input, error.out());
        error.check();

        tiro_tuple_set(vm, tuple, i, input, error.out());
        error.check();

        tiro_tuple_get(vm, tuple, i, output, error.out());
        REQUIRE(tiro_value_kind(vm, output) == TIRO_KIND_INTEGER);
        REQUIRE(tiro_integer_value(vm, output) == i);
    }
}

TEST_CASE("Array construction should succeed", "[api]") {
    VM vm(tiro_vm_new(nullptr));
    Error error;
    Frame frame(tiro_frame_new(vm, 1));

    tiro_handle result = tiro_frame_slot(frame, 0);
    tiro_errc err = tiro_make_array(vm, 0, result, error.out());
    REQUIRE(err == TIRO_OK);
    error.check();

    REQUIRE(tiro_value_kind(vm, result) == TIRO_KIND_ARRAY);
    REQUIRE(tiro_array_size(vm, result) == 0);
}

TEST_CASE("Array access should report type errors", "[api]") {
    VM vm(tiro_vm_new(nullptr));
    Frame frame(tiro_frame_new(vm, 2));
    tiro_handle array = tiro_frame_slot(frame, 0);
    tiro_handle element = tiro_frame_slot(frame, 1);

    SECTION("Read access") {
        tiro_errc errc = tiro_array_get(vm, array, 0, element, nullptr);
        REQUIRE(errc == TIRO_ERROR_BAD_TYPE);
    }

    SECTION("Read access") {
        tiro_errc errc = tiro_array_set(vm, array, 0, element, nullptr);
        REQUIRE(errc == TIRO_ERROR_BAD_TYPE);
    }

    SECTION("Push") {
        tiro_errc errc = tiro_array_push(vm, array, element, nullptr);
        REQUIRE(errc == TIRO_ERROR_BAD_TYPE);
    }

    SECTION("Pop") {
        tiro_errc errc = tiro_array_pop(vm, array, nullptr);
        REQUIRE(errc == TIRO_ERROR_BAD_TYPE);
    }
}

TEST_CASE("Array element access should report out of bounds errors", "[api]") {
    VM vm(tiro_vm_new(nullptr));
    Error error;
    Frame frame(tiro_frame_new(vm, 2));
    tiro_handle array = tiro_frame_slot(frame, 0);
    tiro_handle element = tiro_frame_slot(frame, 1);

    tiro_make_array(vm, 1, array, error.out());
    error.check();

    tiro_make_integer(vm, 42, element, error.out());
    error.check();

    tiro_array_push(vm, array, element, error.out());
    error.check();

    REQUIRE(tiro_array_size(vm, array) == 1);

    SECTION("Read access") {
        tiro_errc errc = tiro_array_get(vm, array, 1, element, nullptr);
        REQUIRE(errc == TIRO_ERROR_OUT_OF_BOUNDS);
        REQUIRE(tiro_integer_value(vm, element) == 42); // Not touched.
    }

    SECTION("Write access") {
        tiro_errc errc = tiro_array_set(vm, array, 1, element, nullptr);
        REQUIRE(errc == TIRO_ERROR_OUT_OF_BOUNDS);
        REQUIRE(tiro_integer_value(vm, element) == 42); // Not touched.
    }
}

TEST_CASE("Array should support insertion of new elements at the end", "[api]") {
    VM vm(tiro_vm_new(nullptr));
    Error error;
    Frame frame(tiro_frame_new(vm, 3));
    tiro_handle array = tiro_frame_slot(frame, 0);
    tiro_handle input = tiro_frame_slot(frame, 1);
    tiro_handle output = tiro_frame_slot(frame, 2);

    tiro_make_array(vm, 0, array, error.out());
    error.check();

    const int64_t count = 5;
    for (int64_t i = 0; i < count; ++i) {
        CAPTURE(i);

        tiro_make_integer(vm, i, input, error.out());
        error.check();

        tiro_array_push(vm, array, input, error.out());
        error.check();
    }

    REQUIRE(tiro_array_size(vm, array) == count);
    for (int64_t i = 0; i < count; ++i) {
        CAPTURE(i);

        tiro_array_get(vm, array, i, output, error.out());
        error.check();

        REQUIRE(tiro_value_kind(vm, output) == TIRO_KIND_INTEGER);
        REQUIRE(tiro_integer_value(vm, output) == i);
    }
}

TEST_CASE("Array should support removal of elements at the end", "[api]") {
    VM vm(tiro_vm_new(nullptr));
    Error error;
    Frame frame(tiro_frame_new(vm, 2));
    tiro_handle array = tiro_frame_slot(frame, 0);
    tiro_handle value = tiro_frame_slot(frame, 1);

    tiro_make_array(vm, 0, array, error.out());
    error.check();

    const int64_t count = 5;
    fill_array(vm, array, count);

    for (int64_t expected = count; expected-- > 0;) {
        CAPTURE(expected);

        tiro_array_get(vm, array, expected, value, error.out());
        error.check();

        REQUIRE(tiro_value_kind(vm, value) == TIRO_KIND_INTEGER);
        REQUIRE(tiro_integer_value(vm, value) == expected);

        tiro_errc errc = tiro_array_pop(vm, array, error.out());
        REQUIRE(errc == TIRO_OK);
        error.check();

        REQUIRE(tiro_array_size(vm, array) == static_cast<size_t>(expected));
    }
}

TEST_CASE("Pop on an empty array should return an error", "[api]") {
    VM vm(tiro_vm_new(nullptr));
    Error error;
    Frame frame(tiro_frame_new(vm, 2));
    tiro_handle array = tiro_frame_slot(frame, 0);

    tiro_make_array(vm, 0, array, error.out());
    error.check();

    REQUIRE(tiro_array_size(vm, array) == 0);
    tiro_errc errc = tiro_array_pop(vm, array, nullptr);
    REQUIRE(errc == TIRO_ERROR_OUT_OF_BOUNDS);
}

TEST_CASE("Arrays should be support removal of all elements", "[api]") {
    VM vm(tiro_vm_new(nullptr));
    Error error;
    Frame frame(tiro_frame_new(vm, 2));
    tiro_handle array = tiro_frame_slot(frame, 0);

    tiro_make_array(vm, 0, array, error.out());
    error.check();

    fill_array(vm, array, 123);

    tiro_array_clear(vm, array, error.out());
    error.check();

    REQUIRE(tiro_array_size(vm, array) == 0);
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
    REQUIRE(tiro_value_kind(vm, handle) == TIRO_KIND_NULL);
}
