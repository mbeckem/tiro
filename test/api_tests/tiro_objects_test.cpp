
#include <catch2/catch.hpp>

#include "tiro/api.h"

#include "./helpers.hpp"

#include <string>
#include <vector>

static void dummy_sync_func(tiro_vm_t, tiro_sync_frame_t){};

static void dummy_async_func(tiro_vm_t, tiro_async_frame_t frame) {
    tiro_async_frame_free(frame);
}

static tiro::record make_record(tiro::vm& vm, const std::vector<std::string>& keys) {
    tiro::array in_keys = tiro::make_array(vm, keys.size());
    for (const auto& key : keys) {
        in_keys.push(tiro::make_string(vm, key));
    }

    return tiro::make_record(vm, in_keys);
}

TEST_CASE("String serialization should return sensible results", "[api]") {
    tiro::vm vm;

    auto to_string = [&](const tiro::handle& value) {
        tiro::handle string(vm.raw_vm());
        tiro_value_to_string(
            vm.raw_vm(), value.raw_handle(), string.raw_handle(), tiro::error_adapter());
        return string.as<tiro::string>().value();
    };

    REQUIRE(to_string(tiro::make_null(vm)) == "null");
    REQUIRE(to_string(tiro::make_boolean(vm, true)) == "true");
    REQUIRE(to_string(tiro::make_boolean(vm, false)) == "false");
    REQUIRE(to_string(tiro::make_integer(vm, 123)) == "123");
    REQUIRE(to_string(tiro::make_float(vm, 123.4)) == "123.4");
    REQUIRE(to_string(tiro::make_string(vm, "Hello")) == "Hello");
}

TEST_CASE("Null values should be constructible", "[api]") {
    tiro::vm vm;

    // To see a difference, handles are initialzied to null by default.
    tiro::handle handle = tiro::make_integer(vm, 123);
    REQUIRE(tiro_value_kind(vm.raw_vm(), handle.raw_handle()) == TIRO_KIND_INTEGER);

    tiro_make_null(vm.raw_vm(), handle.raw_handle());
    REQUIRE(tiro_value_kind(vm.raw_vm(), handle.raw_handle()) == TIRO_KIND_NULL);
}

TEST_CASE("Boolean construction should fail if parameters are invalid", "[api]") {
    tiro::vm vm;
    tiro::handle handle = tiro::make_null(vm);

    SECTION("Invalid vm") {
        tiro_errc_t errc = TIRO_OK;
        tiro_make_boolean(nullptr, true, handle.raw_handle(), error_observer(errc));
        REQUIRE(errc == TIRO_ERROR_BAD_ARG);
    }

    SECTION("Invalid handle") {
        tiro_errc_t errc = TIRO_OK;
        tiro_make_boolean(vm.raw_vm(), true, nullptr, error_observer(errc));
        REQUIRE(errc == TIRO_ERROR_BAD_ARG);
    }
}

TEST_CASE("Boolean values should be constructible", "[api]") {
    tiro::vm vm;
    tiro::handle handle = tiro::make_null(vm);

    auto test_value = [&](bool value) {
        tiro_make_boolean(vm.raw_vm(), value, handle.raw_handle(), tiro::error_adapter());

        REQUIRE(tiro_value_kind(vm.raw_vm(), handle.raw_handle()) == TIRO_KIND_BOOLEAN);
        REQUIRE(tiro_boolean_value(vm.raw_vm(), handle.raw_handle()) == value);
    };

    SECTION("true") { test_value(true); }
    SECTION("false") { test_value(false); }
}

TEST_CASE("Boolean value retrieval should support conversions", "[api]") {
    tiro::vm vm;

    tiro::handle null = tiro::make_null(vm);
    tiro::handle zero_int = tiro::make_integer(vm, 0);
    tiro::handle zero_float = tiro::make_float(vm, 0);
    tiro::handle empty_string = tiro::make_string(vm, "");
    tiro::handle empty_tuple = tiro::make_tuple(vm, 0);
    tiro::handle empty_array = tiro::make_array(vm, 0);

    REQUIRE(tiro_boolean_value(vm.raw_vm(), null.raw_handle()) == false);
    REQUIRE(tiro_boolean_value(vm.raw_vm(), zero_int.raw_handle()) == true);
    REQUIRE(tiro_boolean_value(vm.raw_vm(), zero_float.raw_handle()) == true);
    REQUIRE(tiro_boolean_value(vm.raw_vm(), empty_string.raw_handle()) == true);
    REQUIRE(tiro_boolean_value(vm.raw_vm(), empty_tuple.raw_handle()) == true);
    REQUIRE(tiro_boolean_value(vm.raw_vm(), empty_array.raw_handle()) == true);
}

TEST_CASE("Integer construction should fail if parameters are invalid", "[api]") {
    tiro::vm vm;
    tiro::handle handle = tiro::make_null(vm);

    SECTION("Invalid vm") {
        tiro_errc_t errc = TIRO_OK;
        tiro_make_integer(nullptr, 12345, handle.raw_handle(), error_observer(errc));
        REQUIRE(errc == TIRO_ERROR_BAD_ARG);
    }

    SECTION("Invalid handle") {
        tiro_errc_t errc = TIRO_OK;
        tiro_make_integer(vm.raw_vm(), 12345, nullptr, error_observer(errc));
        REQUIRE(errc == TIRO_ERROR_BAD_ARG);
    }
}

TEST_CASE("Integer construction should succeed", "[api]") {
    tiro::vm vm;
    tiro::handle handle = tiro::make_null(vm);

    tiro_make_integer(vm.raw_vm(), 12345, handle.raw_handle(), tiro::error_adapter());
    REQUIRE(tiro_value_kind(vm.raw_vm(), handle.raw_handle()) == TIRO_KIND_INTEGER);
    REQUIRE(tiro_integer_value(vm.raw_vm(), handle.raw_handle()) == 12345);
}

TEST_CASE("tiro_integer_value should convert floating point numbers to int", "[api]") {
    tiro::vm vm;
    tiro::handle handle = tiro::make_null(vm);

    tiro_make_float(vm.raw_vm(), 123.456, handle.raw_handle(), tiro::error_adapter());
    REQUIRE(tiro_value_kind(vm.raw_vm(), handle.raw_handle()) == TIRO_KIND_FLOAT);
    REQUIRE(tiro_integer_value(vm.raw_vm(), handle.raw_handle()) == 123);
}

TEST_CASE("tiro_integer_value should return 0 if the value is not a number", "[api]") {
    tiro::vm vm;
    tiro::handle handle = tiro::make_null(vm);

    tiro_make_boolean(vm.raw_vm(), true, handle.raw_handle(), tiro::error_adapter());
    REQUIRE(tiro_value_kind(vm.raw_vm(), handle.raw_handle()) == TIRO_KIND_BOOLEAN);
    REQUIRE(tiro_integer_value(vm.raw_vm(), handle.raw_handle()) == 0);
}

TEST_CASE("Float construction should fail if parameters are invalid", "[api]") {
    tiro::vm vm;
    tiro::handle handle = tiro::make_null(vm);

    SECTION("Invalid vm") {
        tiro_errc_t errc = TIRO_OK;
        tiro_make_float(nullptr, 12345, handle.raw_handle(), error_observer(errc));
        REQUIRE(errc == TIRO_ERROR_BAD_ARG);
    }

    SECTION("Invalid handle") {
        tiro_errc_t errc = TIRO_OK;
        tiro_make_float(vm.raw_vm(), 12345, nullptr, error_observer(errc));
        REQUIRE(errc == TIRO_ERROR_BAD_ARG);
    }
}

TEST_CASE("Float construction should succeeed", "[api]") {
    tiro::vm vm;
    tiro::handle handle = tiro::make_null(vm);

    tiro_make_float(vm.raw_vm(), 123.456, handle.raw_handle(), tiro::error_adapter());
    REQUIRE(tiro_value_kind(vm.raw_vm(), handle.raw_handle()) == TIRO_KIND_FLOAT);
    REQUIRE(tiro_float_value(vm.raw_vm(), handle.raw_handle()) == 123.456);
}

TEST_CASE("tiro_float_value should convert integers to float", "[api]") {
    tiro::vm vm;
    tiro::handle handle = tiro::make_null(vm);

    tiro_make_integer(vm.raw_vm(), 123456, handle.raw_handle(), tiro::error_adapter());
    REQUIRE(tiro_value_kind(vm.raw_vm(), handle.raw_handle()) == TIRO_KIND_INTEGER);
    REQUIRE(tiro_float_value(vm.raw_vm(), handle.raw_handle()) == 123456);
}

TEST_CASE("tiro_float_value should return 0 if the value is not a float", "[api]") {
    tiro::vm vm;
    tiro::handle handle = tiro::make_boolean(vm, true);
    REQUIRE(tiro_value_kind(vm.raw_vm(), handle.raw_handle()) == TIRO_KIND_BOOLEAN);
    REQUIRE(tiro_float_value(vm.raw_vm(), handle.raw_handle()) == 0);
}

TEST_CASE("String construction should fail if parameters are invalid", "[api]") {
    tiro::vm vm;
    tiro::handle handle = tiro::make_null(vm);
    const char message[] = "Hello";

    SECTION("Invalid vm") {
        tiro_errc_t errc = TIRO_OK;
        tiro_make_string_from_data(
            nullptr, message, sizeof(message), handle.raw_handle(), error_observer(errc));
        REQUIRE(errc == TIRO_ERROR_BAD_ARG);
    }

    SECTION("Invalid handle") {
        tiro_errc_t errc = TIRO_OK;
        tiro_make_string_from_data(
            vm.raw_vm(), message, sizeof(message), nullptr, error_observer(errc));
        REQUIRE(errc == TIRO_ERROR_BAD_ARG);
    }

    SECTION("Data null with non-zero length") {
        tiro_errc_t errc = TIRO_OK;
        tiro_make_string_from_data(
            vm.raw_vm(), nullptr, sizeof(message), handle.raw_handle(), error_observer(errc));
        REQUIRE(errc == TIRO_ERROR_BAD_ARG);
    }
}

TEST_CASE("String construction should succeed", "[api]") {
    tiro::vm vm;
    tiro::handle handle = tiro::make_null(vm);

    SECTION("from null c strings") {
        tiro_errc_t errc = TIRO_OK;
        tiro_make_string(vm.raw_vm(), nullptr, handle.raw_handle(), error_observer(errc));
        REQUIRE(errc == TIRO_OK);
        REQUIRE(handle.as<tiro::string>().value() == "");
    }

    SECTION("from empty c strings") {
        tiro_errc_t errc = TIRO_OK;
        tiro_make_string(vm.raw_vm(), "", handle.raw_handle(), error_observer(errc));
        REQUIRE(errc == TIRO_OK);
        REQUIRE(handle.as<tiro::string>().value() == "");
    }

    SECTION("from valid c strings") {
        tiro_errc_t errc = TIRO_OK;
        tiro_make_string(vm.raw_vm(), "Hello World!", handle.raw_handle(), error_observer(errc));
        REQUIRE(errc == TIRO_OK);
        REQUIRE(handle.as<tiro::string>().value() == "Hello World!");
    }

    SECTION("from null data") {
        tiro_errc_t errc = TIRO_OK;
        tiro_make_string_from_data(
            vm.raw_vm(), nullptr, 0, handle.raw_handle(), error_observer(errc));
        REQUIRE(errc == TIRO_OK);
        REQUIRE(handle.as<tiro::string>().value() == "");
    }

    SECTION("from empty data") {
        // Invalid address (does not matter, length is 0).
        tiro_errc_t errc = TIRO_OK;
        tiro_make_string_from_data(
            vm.raw_vm(), (char*) 0x123456, 0, handle.raw_handle(), error_observer(errc));
        REQUIRE(errc == TIRO_OK);
        REQUIRE(handle.as<tiro::string>().value() == "");
    }

    SECTION("from valid data") {
        const char data[] = "Hello World!\0after null!";
        const size_t size = sizeof(data) - 1; // Without trailing 0
        tiro_errc_t errc = TIRO_OK;
        tiro_make_string_from_data(
            vm.raw_vm(), data, size, handle.raw_handle(), error_observer(errc));
        REQUIRE(errc == TIRO_OK);

        std::string actual = handle.as<tiro::string>().value();
        REQUIRE(actual.size() == size);
        REQUIRE(std::equal(actual.begin(), actual.end(), data, data + size));
    }
}

TEST_CASE("String should be convertible to a c string for convenience", "[api]") {
    tiro::vm vm;
    tiro::string string = tiro::make_string(vm, "Hello World!");

    struct StringHolder {
        char* cstr = nullptr;

        ~StringHolder() { std::free(cstr); }
    };

    StringHolder holder;
    tiro_string_cstr(vm.raw_vm(), string.raw_handle(), &holder.cstr, tiro::error_adapter());
    REQUIRE(holder.cstr != nullptr);

    std::string data(holder.cstr);
    REQUIRE(data == "Hello World!");
}

TEST_CASE("Tuple construction should fail if parameters are invalid", "[api]") {
    tiro::vm vm;
    tiro::handle handle = tiro::make_null(vm);

    SECTION("Invalid vm") {
        tiro_errc_t errc = TIRO_OK;
        tiro_make_tuple(nullptr, 0, handle.raw_handle(), error_observer(errc));
        REQUIRE(errc == TIRO_ERROR_BAD_ARG);
    }

    SECTION("Invalid handle") {
        tiro_errc_t errc = TIRO_OK;
        tiro_make_tuple(vm.raw_vm(), 0, nullptr, error_observer(errc));
        REQUIRE(errc == TIRO_ERROR_BAD_ARG);
    }

    SECTION("Out of memory") {
        tiro_errc_t errc = TIRO_OK;
        tiro_make_tuple(vm.raw_vm(), size_t(-1), handle.raw_handle(), error_observer(errc));
        REQUIRE(errc == TIRO_ERROR_ALLOC);
    }
}

TEST_CASE("Tuple construction should succeed", "[api]") {
    tiro::vm vm;

    auto construct = [&](size_t size) {
        tiro::handle handle = tiro::make_null(vm);
        tiro_make_tuple(vm.raw_vm(), size, handle.raw_handle(), tiro::error_adapter());
        REQUIRE(tiro_value_kind(vm.raw_vm(), handle.raw_handle()) == TIRO_KIND_TUPLE);
        REQUIRE(tiro_tuple_size(vm.raw_vm(), handle.raw_handle()) == size);
    };

    SECTION("Zero size tuple") { construct(0); }
    SECTION("Normal tuple") { construct(7); }
    SECTION("Huge tuple") { construct(1 << 15); }
}

TEST_CASE("Tuple elements should be initialized to null", "[api]") {
    tiro::vm vm;
    tiro::tuple tuple = tiro::make_tuple(vm, 123);

    REQUIRE(tiro_tuple_size(vm.raw_vm(), tuple.raw_handle()) == 123);
    for (size_t i = 0; i < 123; ++i) {
        CAPTURE(i);

        tiro::handle element = tiro::make_integer(vm, 1);
        tiro_tuple_get(
            vm.raw_vm(), tuple.raw_handle(), i, element.raw_handle(), tiro::error_adapter());
        REQUIRE(tiro_value_kind(vm.raw_vm(), element.raw_handle()) == TIRO_KIND_NULL);
    }
}

TEST_CASE("Tuple element access should report type errors", "[api]") {
    tiro::vm vm;
    tiro::handle not_tuple = tiro::make_null(vm);
    tiro::handle element = tiro::make_null(vm);

    SECTION("Read access") {
        tiro_errc_t errc = TIRO_OK;
        tiro_tuple_get(
            vm.raw_vm(), not_tuple.raw_handle(), 0, element.raw_handle(), error_observer(errc));
        REQUIRE(errc == TIRO_ERROR_BAD_TYPE);
    }

    SECTION("Write access") {
        tiro_errc_t errc = TIRO_OK;
        tiro_tuple_set(
            vm.raw_vm(), not_tuple.raw_handle(), 0, element.raw_handle(), error_observer(errc));
        REQUIRE(errc == TIRO_ERROR_BAD_TYPE);
    }
}

TEST_CASE("Tuple elements should support assignment", "[api]") {
    const int64_t count = 5;

    tiro::vm vm;
    tiro::tuple tuple = tiro::make_tuple(vm, count);
    for (int64_t i = 0; i < count; ++i) {
        CAPTURE(i);
        tiro::handle input = tiro::make_integer(vm, i);
        tiro::handle output = tiro::make_null(vm);

        tiro_tuple_set(
            vm.raw_vm(), tuple.raw_handle(), i, input.raw_handle(), tiro::error_adapter());
        tiro_tuple_get(
            vm.raw_vm(), tuple.raw_handle(), i, output.raw_handle(), tiro::error_adapter());
        REQUIRE(tiro_value_kind(vm.raw_vm(), output.raw_handle()) == TIRO_KIND_INTEGER);
        REQUIRE(tiro_integer_value(vm.raw_vm(), output.raw_handle()) == i);
    }
}

TEST_CASE("Tuple element access should report out of bounds errors", "[api]") {
    tiro::vm vm;
    tiro::tuple tuple = tiro::make_tuple(vm, 4);
    tiro::integer element = tiro::make_integer(vm, 42);

    SECTION("Read access") {
        tiro_errc_t errc = TIRO_OK;
        tiro_tuple_get(
            vm.raw_vm(), tuple.raw_handle(), 4, element.raw_handle(), error_observer(errc));
        REQUIRE(errc == TIRO_ERROR_OUT_OF_BOUNDS);
        REQUIRE(element.value() == 42); // Not touched.
    }

    SECTION("Write access") {
        tiro_errc_t errc = TIRO_OK;
        tiro_tuple_set(
            vm.raw_vm(), tuple.raw_handle(), 4, element.raw_handle(), error_observer(errc));
        REQUIRE(errc == TIRO_ERROR_OUT_OF_BOUNDS);
        REQUIRE(element.value() == 42); // Not touched.
    }
}

TEST_CASE("Record construction should fail if parameters are invalid", "[api]") {
    tiro::vm vm;
    tiro::handle result = tiro::make_null(vm);
    tiro::array keys = tiro::make_array(vm);
    keys.push(tiro::make_string(vm, "foo"));
    keys.push(tiro::make_string(vm, "bar"));

    SECTION("Invalid vm") {
        tiro_errc_t errc = TIRO_OK;
        tiro_make_record(nullptr, keys.raw_handle(), result.raw_handle(), error_observer(errc));
        REQUIRE(errc == TIRO_ERROR_BAD_ARG);
    }

    SECTION("Invalid keys") {
        tiro_errc_t errc = TIRO_OK;
        tiro_make_record(vm.raw_vm(), nullptr, result.raw_handle(), error_observer(errc));
        REQUIRE(errc == TIRO_ERROR_BAD_ARG);
    }

    SECTION("Invalid result") {
        tiro_errc_t errc = TIRO_OK;
        tiro_make_record(vm.raw_vm(), keys.raw_handle(), nullptr, error_observer(errc));
        REQUIRE(errc == TIRO_ERROR_BAD_ARG);
    }

    SECTION("Keys are not an array") {
        tiro::integer integer = tiro::make_integer(vm, 123);

        tiro_errc_t errc = TIRO_OK;
        tiro_make_record(
            vm.raw_vm(), integer.raw_handle(), result.raw_handle(), error_observer(errc));
        REQUIRE(errc == TIRO_ERROR_BAD_TYPE);
    }

    SECTION("Non-string contents") {
        keys.push(tiro::make_integer(vm, 123));

        tiro_errc_t errc = TIRO_OK;
        tiro_make_record(vm.raw_vm(), keys.raw_handle(), result.raw_handle(), error_observer(errc));
        REQUIRE(errc == TIRO_ERROR_BAD_TYPE);
    }
}

TEST_CASE("Record construction should succeed for valid parameters", "[api]") {
    tiro::vm vm;

    auto construct = [&](const std::vector<std::string>& key_strings) {
        tiro::array in_keys = tiro::make_array(vm, key_strings.size());
        for (const auto& key : key_strings) {
            in_keys.push(tiro::make_string(vm, key));
        }

        tiro::handle result = tiro::make_null(vm);
        tiro_make_record(
            vm.raw_vm(), in_keys.raw_handle(), result.raw_handle(), tiro::error_adapter());
        REQUIRE(tiro_value_kind(vm.raw_vm(), result.raw_handle()) == TIRO_KIND_RECORD);

        tiro::array out_keys = result.as<tiro::record>().keys();
        std::vector<std::string> actual_key_strings;
        for (size_t i = 0, n = out_keys.size(); i < n; ++i)
            actual_key_strings.push_back(out_keys.get(i).as<tiro::string>().value());

        REQUIRE(key_strings == actual_key_strings);
    };

    SECTION("Empty record") { construct({}); }

    SECTION("Normal record") { construct({"a", "b", "c"}); }
}

TEST_CASE("Record values should be initialized to null", "[api]") {
    tiro::vm vm;
    tiro::handle record = make_record(vm, {"foo", "bar"});

    for (const char* raw_key : {"foo", "bar"}) {
        CAPTURE(raw_key);

        tiro::string key = tiro::make_string(vm, raw_key);
        tiro::handle value = tiro::make_integer(vm, 123);
        tiro_record_get(vm.raw_vm(), record.raw_handle(), key.raw_handle(), value.raw_handle(),
            tiro::error_adapter());

        REQUIRE(tiro_value_kind(vm.raw_vm(), value.raw_handle()) == TIRO_KIND_NULL);
    }
}

TEST_CASE("Record functions should report type errors", "[api]") {
    tiro::vm vm;
    tiro::handle not_record = tiro::make_null(vm);
    tiro::handle key = tiro::make_string(vm, "foo");
    tiro::handle result = tiro::make_null(vm);

    SECTION("keys access") {
        tiro_errc_t errc = TIRO_OK;
        tiro_record_keys(
            vm.raw_vm(), not_record.raw_handle(), result.raw_handle(), error_observer(errc));
        REQUIRE(errc == TIRO_ERROR_BAD_TYPE);
    }

    SECTION("key read") {
        tiro_errc_t errc = TIRO_OK;
        tiro_record_get(vm.raw_vm(), not_record.raw_handle(), key.raw_handle(), result.raw_handle(),
            error_observer(errc));
        REQUIRE(errc == TIRO_ERROR_BAD_TYPE);
    }

    SECTION("key write") {
        tiro_errc_t errc = TIRO_OK;
        tiro_record_set(vm.raw_vm(), not_record.raw_handle(), key.raw_handle(), result.raw_handle(),
            error_observer(errc));
        REQUIRE(errc == TIRO_ERROR_BAD_TYPE);
    }
}

TEST_CASE("Record key access should report errors if parameters are invalid", "[api]") {
    tiro::vm vm;
    tiro::handle record = make_record(vm, {"foo", "bar"});
    tiro::handle result = tiro::make_null(vm);

    SECTION("Read fails for non-string keys") {
        tiro::handle integer = tiro::make_integer(vm, 123);

        tiro_errc_t errc = TIRO_OK;
        tiro_record_get(vm.raw_vm(), record.raw_handle(), integer.raw_handle(), result.raw_handle(),
            error_observer(errc));
        REQUIRE(errc == TIRO_ERROR_BAD_TYPE);
    }

    SECTION("Write fails for non-string keys") {
        tiro::handle integer = tiro::make_integer(vm, 123);

        tiro_errc_t errc = TIRO_OK;
        tiro_record_set(vm.raw_vm(), record.raw_handle(), integer.raw_handle(), result.raw_handle(),
            error_observer(errc));
        REQUIRE(errc == TIRO_ERROR_BAD_TYPE);
    }

    SECTION("Read fails for non-existant key") {
        tiro::handle invalid_key = tiro::make_string(vm, "asd");

        tiro_errc_t errc = TIRO_OK;
        tiro_record_get(vm.raw_vm(), record.raw_handle(), invalid_key.raw_handle(),
            result.raw_handle(), error_observer(errc));
        REQUIRE(errc == TIRO_ERROR_BAD_KEY);
    }

    SECTION("Write fails for non-existant key") {
        tiro::handle invalid_key = tiro::make_string(vm, "asd");

        tiro_errc_t errc = TIRO_OK;
        tiro_record_set(vm.raw_vm(), record.raw_handle(), invalid_key.raw_handle(),
            result.raw_handle(), error_observer(errc));
        REQUIRE(errc == TIRO_ERROR_BAD_KEY);
    }
}

TEST_CASE("Record elements should support assignment", "[api]") {
    tiro::vm vm;
    tiro::handle record = make_record(vm, {"foo", "bar"});

    int64_t count = 0;
    for (const char* raw_key : {"foo", "bar"}) {
        CAPTURE(raw_key);

        tiro::string key = tiro::make_string(vm, raw_key);
        tiro::integer value = tiro::make_integer(vm, ++count);

        tiro_record_set(vm.raw_vm(), record.raw_handle(), key.raw_handle(), value.raw_handle(),
            tiro::error_adapter());

        tiro::handle result = tiro::make_null(vm);
        tiro_record_get(vm.raw_vm(), record.raw_handle(), key.raw_handle(), result.raw_handle(),
            tiro::error_adapter());
        REQUIRE(result.as<tiro::integer>().value() == count);
    }
}

TEST_CASE("Array construction should fail if parameters are invalid", "[api]") {
    tiro::vm vm;
    tiro::handle handle = tiro::make_null(vm);

    SECTION("Invalid vm") {
        tiro_errc_t errc = TIRO_OK;
        tiro_make_array(nullptr, 0, handle.raw_handle(), error_observer(errc));
        REQUIRE(errc == TIRO_ERROR_BAD_ARG);
    }

    SECTION("Invalid handle") {
        tiro_errc_t errc = TIRO_OK;
        tiro_make_array(vm.raw_vm(), 0, nullptr, error_observer(errc));
        REQUIRE(errc == TIRO_ERROR_BAD_ARG);
    }

    SECTION("Out of memory") {
        tiro_errc_t errc = TIRO_OK;
        tiro_make_array(vm.raw_vm(), size_t(-1), handle.raw_handle(), error_observer(errc));
        REQUIRE(errc == TIRO_ERROR_ALLOC);
    }
}

TEST_CASE("Array construction should succeed", "[api]") {
    tiro::vm vm;
    tiro::handle value = tiro::make_null(vm);
    tiro_make_array(vm.raw_vm(), 0, value.raw_handle(), tiro::error_adapter());

    REQUIRE(tiro_value_kind(vm.raw_vm(), value.raw_handle()) == TIRO_KIND_ARRAY);
    REQUIRE(tiro_array_size(vm.raw_vm(), value.raw_handle()) == 0);
}

TEST_CASE("Array access should report type errors", "[api]") {
    tiro::vm vm;
    tiro::handle not_array = tiro::make_integer(vm, 123);
    tiro::handle element = tiro::make_null(vm);

    SECTION("Read access") {
        tiro_errc_t errc = TIRO_OK;
        tiro_array_get(
            vm.raw_vm(), not_array.raw_handle(), 0, element.raw_handle(), error_observer(errc));
        REQUIRE(errc == TIRO_ERROR_BAD_TYPE);
        REQUIRE(tiro_value_kind(vm.raw_vm(), element.raw_handle()) == TIRO_KIND_NULL);
    }

    SECTION("Write access") {
        tiro_errc_t errc = TIRO_OK;
        tiro_array_set(
            vm.raw_vm(), not_array.raw_handle(), 0, element.raw_handle(), error_observer(errc));
        REQUIRE(errc == TIRO_ERROR_BAD_TYPE);
        REQUIRE(tiro_value_kind(vm.raw_vm(), element.raw_handle()) == TIRO_KIND_NULL);
    }

    SECTION("Push") {
        tiro_errc_t errc = TIRO_OK;
        tiro_array_push(
            vm.raw_vm(), not_array.raw_handle(), element.raw_handle(), error_observer(errc));
        REQUIRE(errc == TIRO_ERROR_BAD_TYPE);
        REQUIRE(tiro_value_kind(vm.raw_vm(), element.raw_handle()) == TIRO_KIND_NULL);
    }

    SECTION("Pop") {
        tiro_errc_t errc = TIRO_OK;
        tiro_array_pop(vm.raw_vm(), not_array.raw_handle(), error_observer(errc));
        REQUIRE(errc == TIRO_ERROR_BAD_TYPE);
    }

    SECTION("Clear") {
        tiro_errc_t errc = TIRO_OK;
        tiro_array_clear(vm.raw_vm(), not_array.raw_handle(), error_observer(errc));
        REQUIRE(errc == TIRO_ERROR_BAD_TYPE);
    }
}

TEST_CASE("Array elements should support assignment", "[api]") {
    tiro::vm vm;
    tiro::array array = tiro::make_array(vm, 0);
    array.push(tiro::make_null(vm));
    array.push(tiro::make_null(vm));

    for (int64_t i = 0; i < 2; ++i) {
        CAPTURE(i);
        tiro::handle input = tiro::make_integer(vm, i);
        tiro::handle output = tiro::make_null(vm);

        tiro_array_set(
            vm.raw_vm(), array.raw_handle(), i, input.raw_handle(), tiro::error_adapter());
        tiro_array_get(
            vm.raw_vm(), array.raw_handle(), i, output.raw_handle(), tiro::error_adapter());
        REQUIRE(tiro_value_kind(vm.raw_vm(), output.raw_handle()) == TIRO_KIND_INTEGER);
        REQUIRE(tiro_integer_value(vm.raw_vm(), output.raw_handle()) == i);
    }
}

TEST_CASE("Array element access should report out of bounds errors", "[api]") {
    tiro::vm vm;
    tiro::array array = tiro::make_array(vm, 1);
    tiro::handle element = tiro::make_integer(vm, 42);
    array.push(element);
    REQUIRE(array.size() == 1);

    SECTION("Read access") {
        tiro_errc_t errc = TIRO_OK;
        tiro_array_get(
            vm.raw_vm(), array.raw_handle(), 1, element.raw_handle(), error_observer(errc));
        REQUIRE(errc == TIRO_ERROR_OUT_OF_BOUNDS);
        REQUIRE(tiro_integer_value(vm.raw_vm(), element.raw_handle()) == 42); // Not touched.
    }

    SECTION("Write access") {
        tiro_errc_t errc = TIRO_OK;
        tiro_array_set(
            vm.raw_vm(), array.raw_handle(), 1, element.raw_handle(), error_observer(errc));
        REQUIRE(errc == TIRO_ERROR_OUT_OF_BOUNDS);
        REQUIRE(tiro_integer_value(vm.raw_vm(), element.raw_handle()) == 42); // Not touched.
    }
}

TEST_CASE("Array should support insertion of new elements at the end", "[api]") {
    tiro::vm vm;
    tiro::array array = tiro::make_array(vm);

    const int64_t count = 5;
    for (int64_t i = 0; i < count; ++i) {
        CAPTURE(i);

        tiro::integer input = tiro::make_integer(vm, i);
        tiro_array_push(vm.raw_vm(), array.raw_handle(), input.raw_handle(), tiro::error_adapter());
    }
    REQUIRE(tiro_array_size(vm.raw_vm(), array.raw_handle()) == count);

    for (int64_t i = 0; i < count; ++i) {
        CAPTURE(i);

        tiro::handle output = tiro::make_null(vm);
        tiro_array_get(
            vm.raw_vm(), array.raw_handle(), i, output.raw_handle(), tiro::error_adapter());
        REQUIRE(output.as<tiro::integer>().value() == i);
    }
}

TEST_CASE("Array should support removal of elements at the end", "[api]") {
    tiro::vm vm;
    tiro::array array = tiro::make_array(vm);

    const int64_t count = 5;
    for (int64_t i = 0; i < count; ++i) {
        array.push(tiro::make_integer(vm, i));
    }

    for (int64_t expected = count; expected-- > 0;) {
        CAPTURE(expected);

        tiro::handle element = array.get(expected);
        REQUIRE(element.as<tiro::integer>().value() == expected);

        tiro_array_pop(vm.raw_vm(), array.raw_handle(), tiro::error_adapter());
        REQUIRE(tiro_array_size(vm.raw_vm(), array.raw_handle()) == static_cast<size_t>(expected));
    }
}

TEST_CASE("Pop on an empty array should return an error", "[api]") {
    tiro::vm vm;
    tiro::array array = tiro::make_array(vm);

    REQUIRE(tiro_array_size(vm.raw_vm(), array.raw_handle()) == 0);
    tiro_errc_t errc = TIRO_OK;
    tiro_array_pop(vm.raw_vm(), array.raw_handle(), error_observer(errc));
    REQUIRE(errc == TIRO_ERROR_OUT_OF_BOUNDS);
}

TEST_CASE("Arrays should be support removal of all elements", "[api]") {
    tiro::vm vm;
    tiro::array array = tiro::make_array(vm);

    for (int i = 0; i < 123; ++i) {
        array.push(tiro::make_integer(vm, i));
    }
    REQUIRE(tiro_array_size(vm.raw_vm(), array.raw_handle()) == 123);

    tiro_array_clear(vm.raw_vm(), array.raw_handle(), tiro::error_adapter());
    REQUIRE(tiro_array_size(vm.raw_vm(), array.raw_handle()) == 0);
}

TEST_CASE("Result construction should fail for invalid arguments", "[api]") {
    tiro::vm vm;
    tiro::handle result = tiro::make_null(vm);
    tiro::handle value = tiro::make_integer(vm, 123);

    SECTION("Success with invalid vm") {
        tiro_errc_t errc = TIRO_OK;
        tiro_make_success(nullptr, value.raw_handle(), result.raw_handle(), error_observer(errc));
        REQUIRE(errc == TIRO_ERROR_BAD_ARG);
    }

    SECTION("Failure with invalid vm") {
        tiro_errc_t errc = TIRO_OK;
        tiro_make_failure(nullptr, value.raw_handle(), result.raw_handle(), error_observer(errc));
        REQUIRE(errc == TIRO_ERROR_BAD_ARG);
    }

    SECTION("Success with invalid value") {
        tiro_errc_t errc = TIRO_OK;
        tiro_make_success(vm.raw_vm(), nullptr, result.raw_handle(), error_observer(errc));
        REQUIRE(errc == TIRO_ERROR_BAD_ARG);
    }

    SECTION("Success with invalid reason") {
        tiro_errc_t errc = TIRO_OK;
        tiro_make_failure(vm.raw_vm(), nullptr, result.raw_handle(), error_observer(errc));
        REQUIRE(errc == TIRO_ERROR_BAD_ARG);
    }

    SECTION("Success with invalid output handle") {
        tiro_errc_t errc = TIRO_OK;
        tiro_make_success(vm.raw_vm(), value.raw_handle(), nullptr, error_observer(errc));
        REQUIRE(errc == TIRO_ERROR_BAD_ARG);
    }

    SECTION("Failure with invalid output handle") {
        tiro_errc_t errc = TIRO_OK;
        tiro_make_failure(vm.raw_vm(), value.raw_handle(), nullptr, error_observer(errc));
        REQUIRE(errc == TIRO_ERROR_BAD_ARG);
    }
}

TEST_CASE("Result construction should succeed", "[api]") {
    tiro::vm vm;
    tiro::handle value = tiro::make_integer(vm, 123);
    tiro::handle result = tiro::make_null(vm);
    tiro::handle check = tiro::make_null(vm);

    SECTION("New success") {
        tiro_make_success(
            vm.raw_vm(), value.raw_handle(), result.raw_handle(), tiro::error_adapter());
        REQUIRE(tiro_value_kind(vm.raw_vm(), result.raw_handle()) == TIRO_KIND_RESULT);
        REQUIRE(tiro_result_is_success(vm.raw_vm(), result.raw_handle()));
        REQUIRE(!tiro_result_is_failure(vm.raw_vm(), result.raw_handle()));

        tiro_result_value(
            vm.raw_vm(), result.raw_handle(), check.raw_handle(), tiro::error_adapter());
        REQUIRE(check.as<tiro::integer>().value() == 123);
    }

    SECTION("New failure") {
        tiro_make_failure(
            vm.raw_vm(), value.raw_handle(), result.raw_handle(), tiro::error_adapter());
        REQUIRE(tiro_value_kind(vm.raw_vm(), result.raw_handle()) == TIRO_KIND_RESULT);
        REQUIRE(tiro_result_is_failure(vm.raw_vm(), result.raw_handle()));
        REQUIRE(!tiro_result_is_success(vm.raw_vm(), result.raw_handle()));

        tiro_result_reason(
            vm.raw_vm(), result.raw_handle(), check.raw_handle(), tiro::error_adapter());
        REQUIRE(check.as<tiro::integer>().value() == 123);
    }
}

TEST_CASE("Value retrieval should fail for invalid inputs", "[api]") {
    tiro::vm vm;
    tiro::integer integer = tiro::make_integer(vm, 123);
    tiro::result failure = tiro::make_failure(vm, integer);
    tiro::result success = tiro::make_success(vm, integer);
    tiro::handle out = tiro::make_null(vm);

    SECTION("Invalid vm") {
        tiro_errc_t errc = TIRO_OK;
        tiro_result_value(nullptr, success.raw_handle(), out.raw_handle(), error_observer(errc));
        REQUIRE(errc == TIRO_ERROR_BAD_ARG);
    }

    SECTION("Invalid instance") {
        tiro_errc_t errc = TIRO_OK;
        tiro_result_value(vm.raw_vm(), nullptr, out.raw_handle(), error_observer(errc));
        REQUIRE(errc == TIRO_ERROR_BAD_ARG);
    }

    SECTION("Invalid output handle") {
        tiro_errc_t errc = TIRO_OK;
        tiro_result_value(vm.raw_vm(), success.raw_handle(), nullptr, error_observer(errc));
        REQUIRE(errc == TIRO_ERROR_BAD_ARG);
    }

    SECTION("Not a result") {
        tiro_errc_t errc = TIRO_OK;
        tiro_result_value(
            vm.raw_vm(), integer.raw_handle(), out.raw_handle(), error_observer(errc));
        REQUIRE(errc == TIRO_ERROR_BAD_TYPE);
    }

    SECTION("Not a success") {
        tiro_errc_t errc = TIRO_OK;
        tiro_result_value(
            vm.raw_vm(), failure.raw_handle(), out.raw_handle(), error_observer(errc));
        REQUIRE(errc == TIRO_ERROR_BAD_STATE);
    }
}

TEST_CASE("Failure retrieval should fail for invalid inputs", "[api]") {
    tiro::vm vm;
    tiro::integer integer = tiro::make_integer(vm, 123);
    tiro::result failure = tiro::make_failure(vm, integer);
    tiro::result success = tiro::make_success(vm, integer);
    tiro::handle out = tiro::make_null(vm);

    SECTION("Invalid vm") {
        tiro_errc_t errc = TIRO_OK;
        tiro_result_reason(nullptr, failure.raw_handle(), out.raw_handle(), error_observer(errc));
        REQUIRE(errc == TIRO_ERROR_BAD_ARG);
    }

    SECTION("Invalid instance") {
        tiro_errc_t errc = TIRO_OK;
        tiro_result_reason(vm.raw_vm(), nullptr, out.raw_handle(), error_observer(errc));
        REQUIRE(errc == TIRO_ERROR_BAD_ARG);
    }

    SECTION("Invalid output handle") {
        tiro_errc_t errc = TIRO_OK;
        tiro_result_reason(vm.raw_vm(), failure.raw_handle(), nullptr, error_observer(errc));
        REQUIRE(errc == TIRO_ERROR_BAD_ARG);
    }

    SECTION("Not a result") {
        tiro_errc_t errc = TIRO_OK;
        tiro_result_reason(
            vm.raw_vm(), integer.raw_handle(), out.raw_handle(), error_observer(errc));
        REQUIRE(errc == TIRO_ERROR_BAD_TYPE);
    }

    SECTION("Not a failure") {
        tiro_errc_t errc = TIRO_OK;
        tiro_result_reason(
            vm.raw_vm(), success.raw_handle(), out.raw_handle(), error_observer(errc));
        REQUIRE(errc == TIRO_ERROR_BAD_STATE);
    }
}

TEST_CASE("Coroutine construction should succeed", "[api]") {
    tiro::vm vm;
    load_test(vm, R"(
        export func foo() {}
    )");

    tiro::function func = tiro::get_export(vm, "test", "foo").as<tiro::function>();
    tiro::handle coroutine = tiro::make_null(vm);

    tiro_make_coroutine(
        vm.raw_vm(), func.raw_handle(), nullptr, coroutine.raw_handle(), tiro::error_adapter());

    REQUIRE(tiro_value_kind(vm.raw_vm(), coroutine.raw_handle()) == TIRO_KIND_COROUTINE);
    REQUIRE(!tiro_coroutine_started(vm.raw_vm(), coroutine.raw_handle()));
    REQUIRE(!tiro_coroutine_completed(vm.raw_vm(), coroutine.raw_handle()));
}

TEST_CASE("Coroutine construction should fail when invalid arguments are provided", "[api]") {
    tiro::vm vm;
    load_test(vm, R"(
            export func foo() {}
        )");

    tiro::handle func = tiro::get_export(vm, "test", "foo");
    tiro::handle args = tiro::make_null(vm);
    tiro::handle result = tiro::make_null(vm);

    SECTION("Invalid vm") {
        tiro_errc_t errc = TIRO_OK;
        tiro_make_coroutine(nullptr, func.raw_handle(), args.raw_handle(), result.raw_handle(),
            error_observer(errc));
        REQUIRE(errc == TIRO_ERROR_BAD_ARG);
    }

    SECTION("Invalid function") {
        tiro_errc_t errc = TIRO_OK;
        tiro_make_coroutine(
            vm.raw_vm(), nullptr, nullptr, result.raw_handle(), error_observer(errc));
        REQUIRE(errc == TIRO_ERROR_BAD_ARG);
    }

    SECTION("Non-function argument") {
        func = tiro::make_integer(vm, 123);
        tiro_errc_t errc = TIRO_OK;
        tiro_make_coroutine(
            vm.raw_vm(), func.raw_handle(), nullptr, result.raw_handle(), error_observer(errc));
        REQUIRE(errc == TIRO_ERROR_BAD_TYPE);
    }

    SECTION("Non-tuple function arguments") {
        args = tiro::make_integer(vm, 123);
        tiro_errc_t errc = TIRO_OK;
        tiro_make_coroutine(vm.raw_vm(), func.raw_handle(), args.raw_handle(), result.raw_handle(),
            error_observer(errc));
        REQUIRE(errc == TIRO_ERROR_BAD_TYPE);
    }
}

TEST_CASE("Coroutines should be executable with a native callback", "[api]") {
    struct Context {
        int callback_called = 0;
        int cleanup_called = 0;
        std::exception_ptr callback_error;
    } context;

    {
        tiro::vm vm;
        load_test(vm, R"(
            export func double(x) = x * 2;
        )");

        tiro::function func = tiro::get_export(vm, "test", "double").as<tiro::function>();
        tiro::tuple args = tiro::make_tuple(vm, 1);
        args.set(0, tiro::make_integer(vm, 123));

        tiro::coroutine coro = tiro::make_coroutine(vm, func, args);

        tiro_coroutine_set_callback(
            vm.raw_vm(), coro.raw_handle(),
            [](tiro_vm_t cb_vm, tiro_handle_t cb_coro, void* userdata) {
                Context* ctx = static_cast<Context*>(userdata);
                ctx->callback_called += 1;
                try {
                    REQUIRE(tiro_value_kind(cb_vm, cb_coro) == TIRO_KIND_COROUTINE);
                    REQUIRE(tiro_coroutine_completed(cb_vm, cb_coro));

                    tiro::handle result(cb_vm);
                    tiro_coroutine_result(
                        cb_vm, cb_coro, result.raw_handle(), tiro::error_adapter());
                    REQUIRE(tiro_value_kind(cb_vm, result.raw_handle()) == TIRO_KIND_RESULT);

                    tiro::handle value(cb_vm);
                    tiro_result_value(
                        cb_vm, result.raw_handle(), value.raw_handle(), tiro::error_adapter());
                    REQUIRE(tiro_value_kind(cb_vm, value.raw_handle()) == TIRO_KIND_INTEGER);
                    REQUIRE(tiro_integer_value(cb_vm, value.raw_handle()) == 246);
                } catch (...) {
                    ctx->callback_error = std::current_exception();
                }
            },
            [](void* userdata) {
                Context* ctx = static_cast<Context*>(userdata);
                ctx->cleanup_called += 1;
            },
            &context, tiro::error_adapter());

        REQUIRE(!tiro_coroutine_started(vm.raw_vm(), coro.raw_handle()));
        tiro_coroutine_start(vm.raw_vm(), coro.raw_handle(), tiro::error_adapter());
        REQUIRE(tiro_coroutine_started(vm.raw_vm(), coro.raw_handle()));

        REQUIRE(tiro_vm_has_ready(vm.raw_vm()));
        tiro_vm_run_ready(vm.raw_vm(), tiro::error_adapter());
        REQUIRE(!tiro_vm_has_ready(vm.raw_vm()));

        // No async code here - the coroutine should resolve without yielding.
        if (context.callback_error)
            std::rethrow_exception(context.callback_error);
        REQUIRE(context.callback_called == 1);
        REQUIRE(context.cleanup_called == 1);
    }

    // Not altered during vm shutdown
    REQUIRE(context.callback_called == 1);
    REQUIRE(context.cleanup_called == 1);
}

TEST_CASE("Coroutine callback cleanup should be invoked during vm shutdown", "[api]") {
    struct Context {
        int callback_called = 0;
        int cleanup_called = 0;
    } context;

    {
        tiro::vm vm;
        load_test(vm, R"(
            export func double(x) = x * 2;
        )");

        tiro::function func = tiro::get_export(vm, "test", "double").as<tiro::function>();
        tiro::tuple args = tiro::make_tuple(vm, 1);
        args.set(0, tiro::make_integer(vm, 123));

        tiro::coroutine coro = tiro::make_coroutine(vm, func, args);

        tiro_coroutine_set_callback(
            vm.raw_vm(), coro.raw_handle(),
            [](tiro_vm_t, tiro_handle_t, void* userdata) {
                Context* ctx = static_cast<Context*>(userdata);
                ctx->callback_called += 1;
            },
            [](void* userdata) {
                Context* ctx = static_cast<Context*>(userdata);
                ctx->cleanup_called += 1;
            },
            &context, tiro::error_adapter());

        SECTION("never started") {}

        SECTION("with start") {
            tiro_coroutine_start(vm.raw_vm(), coro.raw_handle(), tiro::error_adapter());
        }

        REQUIRE(context.callback_called == 0);
        REQUIRE(context.cleanup_called == 0);
    }

    // vm shutdown before completion triggers cleanup execution.
    REQUIRE(context.callback_called == 0);
    REQUIRE(context.cleanup_called == 1);
}

TEST_CASE("Module construction should fail for invalid arguments", "[api]") {
    tiro::vm vm;
    tiro::handle result = tiro::make_null(vm);
    tiro::handle foo_value = tiro::make_integer(vm, 123);

    tiro_module_member_t members[] = {
        {"foo", foo_value.raw_handle()},
    };

    SECTION("Invalid vm") {
        tiro_errc_t errc = TIRO_OK;
        tiro_make_module(nullptr, "test", members, std::size(members), result.raw_handle(),
            error_observer(errc));
        REQUIRE(errc == TIRO_ERROR_BAD_ARG);
    }

    SECTION("Invalid name") {
        tiro_errc_t errc = TIRO_OK;
        tiro_make_module(vm.raw_vm(), nullptr, members, std::size(members), result.raw_handle(),
            error_observer(errc));
        REQUIRE(errc == TIRO_ERROR_BAD_ARG);
    }

    SECTION("Empty name") {
        tiro_errc_t errc = TIRO_OK;
        tiro_make_module(vm.raw_vm(), "", members, std::size(members), result.raw_handle(),
            error_observer(errc));
        REQUIRE(errc == TIRO_ERROR_BAD_ARG);
    }

    SECTION("Null members with nonzero length") {
        tiro_errc_t errc = TIRO_OK;
        tiro_make_module(
            vm.raw_vm(), "test", nullptr, 123, result.raw_handle(), error_observer(errc));
        REQUIRE(errc == TIRO_ERROR_BAD_ARG);
    }

    SECTION("Invalid result") {
        tiro_errc_t errc = TIRO_OK;
        tiro_make_module(
            vm.raw_vm(), "test", members, std::size(members), nullptr, error_observer(errc));
        REQUIRE(errc == TIRO_ERROR_BAD_ARG);
    }

    SECTION("Invalid handle in members list") {
        tiro_module_member_t invalid_members[] = {{"foo", nullptr}};

        tiro_errc_t errc = TIRO_OK;
        tiro_make_module(vm.raw_vm(), "test", invalid_members, std::size(invalid_members),
            result.raw_handle(), error_observer(errc));
        REQUIRE(errc == TIRO_ERROR_BAD_ARG);
    }

    SECTION("Invalid name in members list") {
        tiro_module_member_t invalid_members[] = {{nullptr, foo_value.raw_handle()}};

        tiro_errc_t errc = TIRO_OK;
        tiro_make_module(vm.raw_vm(), "test", invalid_members, std::size(invalid_members),
            result.raw_handle(), error_observer(errc));
        REQUIRE(errc == TIRO_ERROR_BAD_ARG);
    }

    SECTION("Empty name in members list") {
        tiro_module_member_t invalid_members[] = {{"", foo_value.raw_handle()}};

        tiro_errc_t errc = TIRO_OK;
        tiro_make_module(vm.raw_vm(), "test", invalid_members, std::size(invalid_members),
            result.raw_handle(), error_observer(errc));
        REQUIRE(errc == TIRO_ERROR_BAD_ARG);
    }
}

TEST_CASE("Module construction should succeed", "[api]") {
    tiro::vm vm;
    tiro::handle module = tiro::make_null(vm);
    tiro::handle foo_value = tiro::make_integer(vm, 123);
    tiro::handle foo_retrieved = tiro::make_null(vm);

    tiro_module_member_t members[] = {
        {"foo", foo_value.raw_handle()},
    };

    tiro_make_module(vm.raw_vm(), "test", members, std::size(members), module.raw_handle(),
        tiro::error_adapter());
    REQUIRE(tiro_value_kind(vm.raw_vm(), module.raw_handle()) == TIRO_KIND_MODULE);

    tiro_module_get_export(
        vm.raw_vm(), module.raw_handle(), "foo", foo_retrieved.raw_handle(), tiro::error_adapter());
    REQUIRE(foo_retrieved.as<tiro::integer>().value() == 123);
}

TEST_CASE("Retrieving module members should fail when given invalid arguments", "[api]") {
    tiro::vm vm;
    tiro::handle module = tiro::make_null(vm);
    tiro::handle foo_value = tiro::make_integer(vm, 123);
    tiro::handle foo_retrieved = tiro::make_null(vm);

    tiro_module_member_t members[] = {
        {"foo", foo_value.raw_handle()},
    };

    tiro_make_module(vm.raw_vm(), "test", members, std::size(members), module.raw_handle(),
        tiro::error_adapter());

    SECTION("Invalid module") {
        tiro_errc_t errc = TIRO_OK;
        tiro_module_get_export(
            nullptr, module.raw_handle(), "foo", foo_retrieved.raw_handle(), error_observer(errc));
        REQUIRE(errc == TIRO_ERROR_BAD_ARG);
    }

    SECTION("Invalid module") {
        tiro_errc_t errc = TIRO_OK;
        tiro_module_get_export(
            vm.raw_vm(), nullptr, "foo", foo_retrieved.raw_handle(), error_observer(errc));
        REQUIRE(errc == TIRO_ERROR_BAD_ARG);
    }

    SECTION("Invalid name") {
        tiro_errc_t errc = TIRO_OK;
        tiro_module_get_export(vm.raw_vm(), module.raw_handle(), nullptr,
            foo_retrieved.raw_handle(), error_observer(errc));
        REQUIRE(errc == TIRO_ERROR_BAD_ARG);
    }

    SECTION("Empty name") {
        tiro_errc_t errc = TIRO_OK;
        tiro_module_get_export(
            vm.raw_vm(), module.raw_handle(), "", foo_retrieved.raw_handle(), error_observer(errc));
        REQUIRE(errc == TIRO_ERROR_BAD_ARG);
    }

    SECTION("Invalid result") {
        tiro_errc_t errc = TIRO_OK;
        tiro_module_get_export(
            vm.raw_vm(), module.raw_handle(), "foo", nullptr, error_observer(errc));
        REQUIRE(errc == TIRO_ERROR_BAD_ARG);
    }

    SECTION("Not a module") {
        tiro_errc_t errc = TIRO_OK;
        tiro_module_get_export(vm.raw_vm(), foo_value.raw_handle(), "foo",
            foo_retrieved.raw_handle(), error_observer(errc));
        REQUIRE(errc == TIRO_ERROR_BAD_TYPE);
    }

    SECTION("Export not found") {
        tiro_errc_t errc = TIRO_OK;
        tiro_module_get_export(vm.raw_vm(), module.raw_handle(), "bar", foo_retrieved.raw_handle(),
            error_observer(errc));
        REQUIRE(errc == TIRO_ERROR_EXPORT_NOT_FOUND);
    }
}

TEST_CASE("Native object construction should fail when invalid arguments are passed", "[api]") {
    tiro::vm vm;
    tiro::handle result = tiro::make_null(vm);

    tiro_native_type_t descriptor{};
    descriptor.name = "Test type";
    descriptor.finalizer = nullptr;

    SECTION("Invalid vm") {
        tiro_errc_t errc = TIRO_OK;
        tiro_make_native(nullptr, &descriptor, 123, result.raw_handle(), error_observer(errc));
        REQUIRE(errc == TIRO_ERROR_BAD_ARG);
    }

    SECTION("Invalid type descriptor") {
        tiro_errc_t errc = TIRO_OK;
        tiro_make_native(vm.raw_vm(), nullptr, 123, result.raw_handle(), error_observer(errc));
        REQUIRE(errc == TIRO_ERROR_BAD_ARG);
    }

    SECTION("Invalid result handle") {
        tiro_errc_t errc = TIRO_OK;
        tiro_make_native(vm.raw_vm(), &descriptor, 123, nullptr, error_observer(errc));
        REQUIRE(errc == TIRO_ERROR_BAD_ARG);
    }

    SECTION("Zero size allocation") {
        tiro_errc_t errc = TIRO_OK;
        tiro_make_native(vm.raw_vm(), &descriptor, 0, result.raw_handle(), error_observer(errc));
        REQUIRE(errc == TIRO_ERROR_BAD_ARG);
    }
}

TEST_CASE("Native object construction should be successful", "[api]") {
    tiro_native_type_t descriptor{};
    descriptor.name = "Test type";
    descriptor.finalizer = nullptr;

    tiro::vm vm;
    tiro::handle result = tiro::make_null(vm);

    tiro_make_native(vm.raw_vm(), &descriptor, 123, result.raw_handle(), tiro::error_adapter());
    REQUIRE(tiro_value_kind(vm.raw_vm(), result.raw_handle()) == TIRO_KIND_NATIVE);
    REQUIRE(tiro_native_type_descriptor(vm.raw_vm(), result.raw_handle()) == &descriptor);
    REQUIRE(tiro_native_data(vm.raw_vm(), result.raw_handle()) != nullptr);
    REQUIRE(tiro_native_size(vm.raw_vm(), result.raw_handle()) == 123);
}

TEST_CASE("Native object finalizer is invoked on garbage collection", "[api]") {
    struct context {
        int finalizer_called = 0;
        std::exception_ptr error;
    };

    context ctx;
    {
        tiro_native_type_t descriptor{};
        descriptor.name = "Test type";
        descriptor.finalizer = [](void* data, size_t size) {
            context* fin_ctx = *static_cast<context**>(data);

            fin_ctx->finalizer_called += 1;
            try {
                REQUIRE(data != nullptr);
                REQUIRE(size == sizeof(void*));
            } catch (...) {
                fin_ctx->error = std::current_exception();
            }
        };

        tiro::vm vm;
        tiro::handle result = tiro::make_null(vm);

        context* arg_ctx = &ctx;
        tiro_make_native(
            vm.raw_vm(), &descriptor, sizeof(void*), result.raw_handle(), tiro::error_adapter());
        std::memcpy(tiro_native_data(vm.raw_vm(), result.raw_handle()), &arg_ctx, sizeof(void*));
    }

    REQUIRE(ctx.finalizer_called == 1);
    if (ctx.error)
        std::rethrow_exception(ctx.error);
}

TEST_CASE("Type access for invalid kind values should fail", "[api]") {
    tiro::vm vm;
    tiro::handle result = tiro::make_null(vm);

    SECTION("Internal kind") {
        tiro_errc_t errc = TIRO_OK;
        tiro_kind_type(vm.raw_vm(), TIRO_KIND_INTERNAL, result.raw_handle(), error_observer(errc));
        REQUIRE(errc == TIRO_ERROR_BAD_ARG);
    }

    SECTION("Invalid kind") {
        tiro_errc_t errc = TIRO_OK;
        tiro_kind_type(vm.raw_vm(), TIRO_KIND_INVALID, result.raw_handle(), error_observer(errc));
        REQUIRE(errc == TIRO_ERROR_BAD_ARG);
    }
}

TEST_CASE("Types should return their name", "[api]") {
    tiro::vm vm;
    tiro::handle type = tiro::make_null(vm);
    tiro::handle name = tiro::make_null(vm);

    tiro_kind_type(vm.raw_vm(), TIRO_KIND_TUPLE, type.raw_handle(), tiro::error_adapter());
    REQUIRE(tiro_value_kind(vm.raw_vm(), type.raw_handle()) == TIRO_KIND_TYPE);

    tiro_type_name(vm.raw_vm(), type.raw_handle(), name.raw_handle(), tiro::error_adapter());
    REQUIRE(name.as<tiro::string>().value() == "Tuple");
}

TEST_CASE("Copying values should fail for invalid inputs", "[api]") {
    tiro::vm vm;
    tiro::handle source = tiro::make_integer(vm, 123);
    tiro::handle target = tiro::make_null(vm);

    SECTION("Invalid vm") {
        tiro_errc_t errc = TIRO_OK;
        tiro_value_copy(nullptr, source.raw_handle(), target.raw_handle(), error_observer(errc));
        REQUIRE(errc == TIRO_ERROR_BAD_ARG);
    }

    SECTION("Invalid source") {
        tiro_errc_t errc = TIRO_OK;
        tiro_value_copy(vm.raw_vm(), nullptr, target.raw_handle(), error_observer(errc));
        REQUIRE(errc == TIRO_ERROR_BAD_ARG);
    }

    SECTION("Invalid target") {
        tiro_errc_t errc = TIRO_OK;
        tiro_value_copy(vm.raw_vm(), source.raw_handle(), nullptr, error_observer(errc));
        REQUIRE(errc == TIRO_ERROR_BAD_ARG);
    }
}

TEST_CASE("Copying values should work", "[api]") {
    tiro::vm vm;
    tiro::handle source = tiro::make_integer(vm, 123);
    tiro::handle target = tiro::make_null(vm);
    REQUIRE(source.raw_handle() != target.raw_handle());

    tiro_value_copy(vm.raw_vm(), source.raw_handle(), target.raw_handle(), tiro::error_adapter());
    REQUIRE(source.as<tiro::integer>().value() == 123);
    REQUIRE(target.as<tiro::integer>().value() == 123);
}

TEST_CASE("Native sync function construction should fail if invalid arguments are used", "[api]") {
    tiro::vm vm;
    tiro::handle result = tiro::make_null(vm);
    tiro::handle name = tiro::make_string(vm, "func");
    tiro::handle closure = tiro::make_null(vm);

    SECTION("Invalid vm") {
        tiro_errc_t errc = TIRO_OK;
        tiro_make_sync_function(nullptr, name.raw_handle(), dummy_sync_func, 0,
            closure.raw_handle(), result.raw_handle(), error_observer(errc));
        REQUIRE(errc == TIRO_ERROR_BAD_ARG);
    }

    SECTION("Invalid name (null handle)") {
        tiro_errc_t errc = TIRO_OK;
        tiro_make_sync_function(vm.raw_vm(), nullptr, dummy_sync_func, 0, closure.raw_handle(),
            result.raw_handle(), error_observer(errc));
        REQUIRE(errc == TIRO_ERROR_BAD_ARG);
    }

    SECTION("Invalid name (not a string)") {
        tiro::handle number = tiro::make_integer(vm, 123);
        tiro_errc_t errc = TIRO_OK;
        tiro_make_sync_function(vm.raw_vm(), number.raw_handle(), dummy_sync_func, 0,
            closure.raw_handle(), result.raw_handle(), error_observer(errc));
        REQUIRE(errc == TIRO_ERROR_BAD_TYPE);
    }

    SECTION("Invalid result handle") {
        tiro_errc_t errc = TIRO_OK;
        tiro_make_sync_function(vm.raw_vm(), name.raw_handle(), dummy_sync_func, 0,
            closure.raw_handle(), nullptr, error_observer(errc));
        REQUIRE(errc == TIRO_ERROR_BAD_ARG);
    }

    SECTION("Invalid function pointer") {
        tiro_errc_t errc = TIRO_OK;
        tiro_make_sync_function(vm.raw_vm(), name.raw_handle(), nullptr, 0, closure.raw_handle(),
            result.raw_handle(), error_observer(errc));
        REQUIRE(errc == TIRO_ERROR_BAD_ARG);
    }

    SECTION("Parameter count too large") {
        tiro_errc_t errc = TIRO_OK;
        tiro_make_sync_function(vm.raw_vm(), name.raw_handle(), dummy_sync_func, 1025,
            closure.raw_handle(), result.raw_handle(), error_observer(errc));
        REQUIRE(errc == TIRO_ERROR_BAD_ARG);
    }
}

TEST_CASE("Native sync function construction should succeed", "[api]") {
    tiro::vm vm;
    tiro::handle name = tiro::make_string(vm, "func");
    tiro::handle closure = tiro::make_tuple(vm, 3);
    tiro::handle result = tiro::make_null(vm);

    SECTION("With closure") {
        tiro_make_sync_function(vm.raw_vm(), name.raw_handle(), dummy_sync_func, 0,
            closure.raw_handle(), result.raw_handle(), tiro::error_adapter());
        REQUIRE(tiro_value_kind(vm.raw_vm(), result.raw_handle()) == TIRO_KIND_FUNCTION);
    }

    SECTION("Closure is optional") {
        tiro_make_sync_function(vm.raw_vm(), name.raw_handle(), dummy_sync_func, 0, nullptr,
            result.raw_handle(), tiro::error_adapter());
        REQUIRE(tiro_value_kind(vm.raw_vm(), result.raw_handle()) == TIRO_KIND_FUNCTION);
    }
}

TEST_CASE("Native sync function invocation should succeed", "[api]") {
    struct Context {
        int called = 0;
        std::exception_ptr error;
    };

    constexpr tiro_sync_function_t native_func = [](tiro_vm_t raw_vm, tiro_sync_frame_t frame) {
        try {
            tiro::vm& vm = tiro::vm::unsafe_from_raw_vm(raw_vm);
            Context* context = std::any_cast<Context*>(vm.userdata());
            try {
                context->called++;

                // Retrieve and verify function arguments.
                REQUIRE(tiro_sync_frame_argc(frame) == 2);
                tiro::handle arg_1 = tiro::make_null(vm);
                tiro::handle arg_2 = tiro::make_null(vm);
                tiro_sync_frame_arg(frame, 0, arg_1.raw_handle(), tiro::error_adapter());
                tiro_sync_frame_arg(frame, 1, arg_2.raw_handle(), tiro::error_adapter());
                REQUIRE(tiro_value_kind(raw_vm, arg_1.raw_handle()) == TIRO_KIND_INTEGER);
                REQUIRE(tiro_value_kind(raw_vm, arg_2.raw_handle()) == TIRO_KIND_FLOAT);

                // Out of bounds errors:
                {
                    tiro::handle result = tiro::make_null(vm);
                    tiro_errc_t errc = TIRO_OK;
                    tiro_sync_frame_arg(frame, 2, result.raw_handle(), error_observer(errc));
                    REQUIRE(errc == TIRO_ERROR_OUT_OF_BOUNDS);
                }

                // Retrieve captured integer from tuple.
                tiro::handle closure = tiro::make_null(vm);
                tiro_sync_frame_closure(frame, closure.raw_handle(), tiro::error_adapter());
                REQUIRE(tiro_value_kind(raw_vm, closure.raw_handle()) == TIRO_KIND_TUPLE);

                // Perform the actual work (a * b + c).
                tiro::integer closure_value = closure.as<tiro::tuple>().get(0).as<tiro::integer>();
                tiro::float_ result = tiro::make_float(
                    vm, arg_1.as<tiro::integer>().value() * arg_2.as<tiro::float_>().value()
                            + closure_value.value());
                tiro_sync_frame_return_value(frame, result.raw_handle(), tiro::error_adapter());
            } catch (...) {
                context->error = std::current_exception();
            }
        } catch (...) {
            // Some kind of memory corruption.
            std::terminate();
        }
    };

    Context context;
    {
        tiro::vm vm;
        vm.userdata() = &context;

        tiro::string name = tiro::make_string(vm, "func");
        tiro::tuple closure = tiro::make_tuple(vm, 1);
        closure.set(0, tiro::make_integer(vm, 7));
        tiro::handle func = tiro::make_null(vm);

        tiro_make_sync_function(vm.raw_vm(), name.raw_handle(), native_func, 2,
            closure.raw_handle(), func.raw_handle(), tiro::error_adapter());

        tiro::tuple args = tiro::make_tuple(vm, 2);
        args.set(0, tiro::make_integer(vm, 10));
        args.set(1, tiro::make_float(vm, 2.5));

        Context coro_context;
        tiro::coroutine coro = tiro::make_coroutine(vm, func.as<tiro::function>(), args);
        coro.set_callback([&coro_context](tiro::vm&, tiro::coroutine inner_coro) {
            coro_context.called += 1;
            try {
                REQUIRE(inner_coro.completed());

                tiro::handle result = inner_coro.result();
                REQUIRE(result.kind() == tiro::value_kind::result);

                tiro::handle value = result.as<tiro::result>().value();
                REQUIRE(value.as<tiro::float_>().value() == 32);
            } catch (...) {
                coro_context.error = std::current_exception();
            }
        });
        coro.start();
        vm.run_ready();

        REQUIRE(coro_context.called == 1);
        if (coro_context.error)
            std::rethrow_exception(context.error);
    }

    REQUIRE(context.called == 1);
    if (context.error)
        std::rethrow_exception(context.error);
}

TEST_CASE("Sync frame functions should fail for invalid input.", "[api]") {
    tiro::vm vm;

    SECTION("Invalid frame for argc") {
        size_t argc = tiro_sync_frame_argc(nullptr);
        REQUIRE(argc == 0);
    }

    SECTION("Invalid frame for arg") {
        tiro::handle result = tiro::make_null(vm);
        tiro_errc_t errc = TIRO_OK;
        tiro_sync_frame_arg(nullptr, 0, result.raw_handle(), error_observer(errc));
        REQUIRE(errc == TIRO_ERROR_BAD_ARG);
    }

    SECTION("Invalid frame for closure") {
        tiro::handle result = tiro::make_null(vm);
        tiro_errc_t errc = TIRO_OK;
        tiro_sync_frame_closure(nullptr, result.raw_handle(), error_observer(errc));
        REQUIRE(errc == TIRO_ERROR_BAD_ARG);
    }

    SECTION("Invalid frame for result") {
        tiro::handle value = tiro::make_null(vm);
        tiro_errc_t errc = TIRO_OK;
        tiro_sync_frame_return_value(nullptr, value.raw_handle(), error_observer(errc));
        REQUIRE(errc == TIRO_ERROR_BAD_ARG);
    }
}

TEST_CASE("Native async function construction should fail if invalid arguments are used", "[api]") {
    tiro::vm vm;
    tiro::handle result = tiro::make_null(vm);
    tiro::handle name = tiro::make_string(vm, "func");
    tiro::handle closure = tiro::make_null(vm);

    SECTION("Invalid vm") {
        tiro_errc_t errc = TIRO_OK;
        tiro_make_async_function(nullptr, name.raw_handle(), dummy_async_func, 0,
            closure.raw_handle(), result.raw_handle(), error_observer(errc));
        REQUIRE(errc == TIRO_ERROR_BAD_ARG);
    }

    SECTION("Invalid name (null handle)") {
        tiro_errc_t errc = TIRO_OK;
        tiro_make_async_function(vm.raw_vm(), nullptr, dummy_async_func, 0, closure.raw_handle(),
            result.raw_handle(), error_observer(errc));
        REQUIRE(errc == TIRO_ERROR_BAD_ARG);
    }

    SECTION("Invalid name (not a string)") {
        tiro::handle number = tiro::make_integer(vm, 123);
        tiro_errc_t errc = TIRO_OK;
        tiro_make_async_function(vm.raw_vm(), number.raw_handle(), dummy_async_func, 0,
            closure.raw_handle(), result.raw_handle(), error_observer(errc));
        REQUIRE(errc == TIRO_ERROR_BAD_TYPE);
    }

    SECTION("Invalid result handle") {
        tiro_errc_t errc = TIRO_OK;
        tiro_make_async_function(vm.raw_vm(), name.raw_handle(), dummy_async_func, 0,
            closure.raw_handle(), nullptr, error_observer(errc));
        REQUIRE(errc == TIRO_ERROR_BAD_ARG);
    }

    SECTION("Invalid function pointer") {
        tiro_errc_t errc = TIRO_OK;
        tiro_make_async_function(vm.raw_vm(), name.raw_handle(), nullptr, 0, closure.raw_handle(),
            result.raw_handle(), error_observer(errc));
        REQUIRE(errc == TIRO_ERROR_BAD_ARG);
    }

    SECTION("Parameter count too large") {
        tiro_errc_t errc = TIRO_OK;
        tiro_make_async_function(vm.raw_vm(), name.raw_handle(), dummy_async_func, 1025,
            closure.raw_handle(), result.raw_handle(), error_observer(errc));
        REQUIRE(errc == TIRO_ERROR_BAD_ARG);
    }
}

TEST_CASE("Native async function construction should succeed", "[api]") {
    tiro::vm vm;
    tiro::handle name = tiro::make_string(vm, "func");
    tiro::handle closure = tiro::make_tuple(vm, 3);
    tiro::handle result = tiro::make_null(vm);

    SECTION("With closure") {
        tiro_make_async_function(vm.raw_vm(), name.raw_handle(), dummy_async_func, 0,
            closure.raw_handle(), result.raw_handle(), tiro::error_adapter());
        REQUIRE(tiro_value_kind(vm.raw_vm(), result.raw_handle()) == TIRO_KIND_FUNCTION);
    }

    SECTION("Closure is optional") {
        tiro_make_async_function(vm.raw_vm(), name.raw_handle(), dummy_async_func, 0, nullptr,
            result.raw_handle(), tiro::error_adapter());
        REQUIRE(tiro_value_kind(vm.raw_vm(), result.raw_handle()) == TIRO_KIND_FUNCTION);
    }
}

TEST_CASE("Native async function invocation should succeed", "[api]") {
    struct Task {
        virtual ~Task() = default;
        virtual void run(tiro::vm& vm) = 0;
    };

    struct AsyncContext {
        int called = 0;
        std::exception_ptr error;
        std::vector<std::unique_ptr<Task>> queue;
    };

    struct AsyncTask : Task {
        tiro_async_frame_t frame_;
        double result_;

        AsyncTask(tiro_async_frame_t frame, double result)
            : frame_(frame)
            , result_(result) {}

        ~AsyncTask() { tiro_async_frame_free(frame_); }

        void run(tiro::vm& vm) override {
            tiro::handle result = tiro::make_float(vm, result_);
            tiro_async_frame_return_value(frame_, result.raw_handle(), tiro::error_adapter());
        }
    };

    struct CoroContext {
        int called = 0;
        std::exception_ptr error;
    };

    constexpr tiro_async_function_t native_func = [](tiro_vm_t raw_vm, tiro_async_frame_t frame) {
        try {
            tiro::vm& vm = tiro::vm::unsafe_from_raw_vm(raw_vm);
            AsyncContext* context = std::any_cast<AsyncContext*>(vm.userdata());
            try {
                context->called++;

                // Retrieve and verify function arguments.
                REQUIRE(tiro_async_frame_argc(frame) == 2);
                tiro::handle arg_1 = tiro::make_null(vm);
                tiro::handle arg_2 = tiro::make_null(vm);
                tiro_async_frame_arg(frame, 0, arg_1.raw_handle(), tiro::error_adapter());
                tiro_async_frame_arg(frame, 1, arg_2.raw_handle(), tiro::error_adapter());
                REQUIRE(tiro_value_kind(raw_vm, arg_1.raw_handle()) == TIRO_KIND_INTEGER);
                REQUIRE(tiro_value_kind(raw_vm, arg_2.raw_handle()) == TIRO_KIND_FLOAT);

                // Out of bounds errors:
                {
                    tiro::handle result = tiro::make_null(vm);
                    tiro_errc_t errc = TIRO_OK;
                    tiro_async_frame_arg(frame, 2, result.raw_handle(), error_observer(errc));
                    REQUIRE(errc == TIRO_ERROR_OUT_OF_BOUNDS);
                }

                // Retrieve captured integer from tuple.
                tiro::handle closure = tiro::make_null(vm);
                tiro_async_frame_closure(frame, closure.raw_handle(), tiro::error_adapter());
                REQUIRE(tiro_value_kind(raw_vm, closure.raw_handle()) == TIRO_KIND_TUPLE);

                // Perform the actual work (a * b + c).
                double result = arg_1.as<tiro::integer>().value() * arg_2.as<tiro::float_>().value()
                                + closure.as<tiro::tuple>().get(0).as<tiro::integer>().value();

                // Enqueue a task to resume the coroutine.
                auto task = std::make_unique<AsyncTask>(frame, result);
                frame = nullptr;
                context->queue.push_back(std::move(task));
            } catch (...) {
                context->error = std::current_exception();
            }
        } catch (...) {
            // Some kind of memory corruption.
            std::terminate();
        }

        if (frame)
            tiro_async_frame_free(frame);
    };

    AsyncContext async_context;
    {
        tiro::vm vm;
        vm.userdata() = &async_context;

        // Pointers to frames must not survive the vm!
        struct CleanupContext {
            ~CleanupContext() { ctx.queue.clear(); }

            AsyncContext& ctx;
        } cleanup_context{async_context};

        tiro::string name = tiro::make_string(vm, "func");
        tiro::tuple closure = tiro::make_tuple(vm, 1);
        closure.set(0, tiro::make_integer(vm, 7));
        tiro::handle func = tiro::make_null(vm);

        tiro_make_async_function(vm.raw_vm(), name.raw_handle(), native_func, 2,
            closure.raw_handle(), func.raw_handle(), tiro::error_adapter());

        tiro::tuple args = tiro::make_tuple(vm, 2);
        args.set(0, tiro::make_integer(vm, 10));
        args.set(1, tiro::make_float(vm, 2.5));

        CoroContext coro_context;
        tiro::coroutine coro = tiro::make_coroutine(vm, func.as<tiro::function>(), args);
        coro.set_callback([&coro_context](tiro::vm&, tiro::coroutine inner_coro) {
            coro_context.called += 1;
            try {
                REQUIRE(inner_coro.completed());

                tiro::handle result = inner_coro.result();
                REQUIRE(result.kind() == tiro::value_kind::result);

                tiro::handle value = result.as<tiro::result>().value();
                REQUIRE(value.as<tiro::float_>().value() == 32);
            } catch (...) {
                coro_context.error = std::current_exception();
            }
        });
        coro.start();

        // Async work is started but not finished. Instead, a task is placed into the queue.
        vm.run_ready();
        REQUIRE(coro_context.called == 0);
        REQUIRE(!vm.has_ready());
        REQUIRE(async_context.called == 1);
        if (async_context.error)
            std::rethrow_exception(async_context.error);

        // Execute the task - this will resume the coroutine.
        REQUIRE(async_context.queue.size() == 1);
        async_context.queue[0]->run(vm);
        async_context.queue.pop_back();

        // The coroutine callback must be executed now.
        REQUIRE(vm.has_ready());
        vm.run_ready();
        REQUIRE(coro_context.called == 1);
        if (coro_context.error)
            std::rethrow_exception(coro_context.error);
    }

    REQUIRE(async_context.called == 1);
    if (async_context.error)
        std::rethrow_exception(async_context.error);
}

TEST_CASE("Async frame functions should fail for invalid input.", "[api]") {
    tiro::vm vm;

    SECTION("Invalid frame for vm") {
        tiro_vm_t raw_vm = tiro_async_frame_vm(nullptr);
        REQUIRE(raw_vm == nullptr);
    }

    SECTION("Invalid frame for argc") {
        size_t argc = tiro_async_frame_argc(nullptr);
        REQUIRE(argc == 0);
    }

    SECTION("Invalid frame for arg") {
        tiro::handle result = tiro::make_null(vm);
        tiro_errc_t errc = TIRO_OK;
        tiro_async_frame_arg(nullptr, 0, result.raw_handle(), error_observer(errc));
        REQUIRE(errc == TIRO_ERROR_BAD_ARG);
    }

    SECTION("Invalid frame for closure") {
        tiro::handle result = tiro::make_null(vm);
        tiro_errc_t errc = TIRO_OK;
        tiro_async_frame_closure(nullptr, result.raw_handle(), error_observer(errc));
        REQUIRE(errc == TIRO_ERROR_BAD_ARG);
    }

    SECTION("Invalid frame for result") {
        tiro::handle value = tiro::make_null(vm);
        tiro_errc_t errc = TIRO_OK;
        tiro_async_frame_return_value(nullptr, value.raw_handle(), error_observer(errc));
        REQUIRE(errc == TIRO_ERROR_BAD_ARG);
    }
}
