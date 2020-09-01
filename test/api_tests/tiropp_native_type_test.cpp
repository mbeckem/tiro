#include <catch2/catch.hpp>

#include "tiropp/native_type.hpp"

namespace {

struct native_data {
    static size_t finalizer_count;

    native_data() noexcept = default;

    native_data(native_data&& other) noexcept
        : moved(std::exchange(other.moved, true))
        , value(std::move(other.value)) {}

    ~native_data() {
        if (!moved)
            finalizer_count += 1;
    }

    native_data& operator=(native_data&&) = delete;

    bool moved = false;
    std::string value;
};

} // namespace

size_t native_data::finalizer_count = 0;

static const char* test_string =
    "This is a longer string to ensure that the std::string has to allocate.";

TEST_CASE("tiropp::native_type should be constructible", "[api]") {
    tiro::native_type<native_data> type("native_data");
    REQUIRE(type.valid());
    REQUIRE(type.name() == "native_data");
}

TEST_CASE("tiropp::native_type should be move constructible", "[api]") {
    tiro::native_type<native_data> source("native_data");
    tiro::native_type<native_data> target = std::move(source);

    REQUIRE(!source.valid());
    REQUIRE(target.valid());
    REQUIRE(target.name() == "native_data");
}

TEST_CASE("tiropp::native_type should be move assignable", "[api]") {
    tiro::native_type<native_data> source("native_data");
    tiro::native_type<native_data> target("other_data");
    target = std::move(source);

    REQUIRE(!source.valid());
    REQUIRE(target.valid());
    REQUIRE(target.name() == "native_data");
}

TEST_CASE("tiropp::native_type should support object construction", "[api]") {
    native_data::finalizer_count = 0;

    tiro::native_type<native_data> type("native_data");
    {
        tiro::vm vm;

        tiro::native object = [&]() {
            native_data data;
            data.value = test_string;
            return type.make(vm, std::move(data));
        }();

        REQUIRE(object.kind() == tiro::value_kind::native);
        REQUIRE(type.is_instance(object));

        native_data* raw_object = type.access(object);
        REQUIRE(raw_object != nullptr);
        REQUIRE(raw_object->value == test_string);
    }

    REQUIRE(native_data::finalizer_count == 1);
}

TEST_CASE("tiropp::native_type should support manual object destruction", "[api]") {
    native_data::finalizer_count = 0;

    tiro::native_type<native_data> type("native_data");
    {
        tiro::vm vm;

        tiro::native object = [&]() {
            native_data data;
            data.value = test_string;
            return type.make(vm, std::move(data));
        }();
        REQUIRE(!type.is_destroyed(object));

        type.destroy(object);
        REQUIRE(type.is_destroyed(object));
        REQUIRE_THROWS_MATCHES(
            type.access(object), tiro::error, Catch::Message("The object was already destroyed."));
        REQUIRE(native_data::finalizer_count == 1);
    }

    REQUIRE(native_data::finalizer_count == 1);
}
