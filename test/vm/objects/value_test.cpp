#include <catch.hpp>

#include "vm/objects/all.hpp"
#include "vm/objects/value.hpp"

#include <type_traits>

using namespace tiro::vm;

TEST_CASE("Nullpointer representation should be an actual 0", "[value]") {
    REQUIRE(reinterpret_cast<uintptr_t>(nullptr) == 0);
}

TEST_CASE("Only expected types should be able to contain references", "[value]") {
    struct Test {
        ValueType type;
        bool expected_may_contain_references;
    };

    Test tests[] = {
        {ValueType::Boolean, false},
        {ValueType::Buffer, false},
        {ValueType::Float, false},
        {ValueType::Integer, false},
        {ValueType::NativeObject, false},
        {ValueType::NativePointer, false},
        {ValueType::Null, false},
        {ValueType::SmallInteger, false},
        {ValueType::String, false},
        {ValueType::Undefined, false},
        {ValueType::Code, false},

        {ValueType::Array, true},
        {ValueType::ArrayStorage, true},
        {ValueType::BoundMethod, true},
        {ValueType::Coroutine, true},
        {ValueType::CoroutineStack, true},
        {ValueType::DynamicObject, true},
        {ValueType::Environment, true},
        {ValueType::Function, true},
        {ValueType::FunctionTemplate, true},
        {ValueType::HashTable, true},
        {ValueType::HashTableIterator, true},
        {ValueType::HashTableStorage, true},
        {ValueType::Method, true},
        {ValueType::Module, true},
        {ValueType::NativeFunction, true},
        {ValueType::StringBuilder, true},
        {ValueType::Symbol, true},
        {ValueType::Tuple, true},
        {ValueType::Type, true},
    };

    for (const auto& test : tests) {
        CAPTURE(to_string(test.type));
        REQUIRE(may_contain_references(test.type) == test.expected_may_contain_references);
    }
}

TEST_CASE("Nullable should be implicitly constructible from T", "[value]") {
    STATIC_REQUIRE(std::is_convertible_v<Value, Nullable<Value>>);
    STATIC_REQUIRE(std::is_convertible_v<Integer, Nullable<Integer>>);
    STATIC_REQUIRE(std::is_convertible_v<SmallInteger, Nullable<SmallInteger>>);
    STATIC_REQUIRE(std::is_convertible_v<Undefined, Nullable<Undefined>>);
    STATIC_REQUIRE(std::is_convertible_v<HashTable, Nullable<HashTable>>);
}

TEST_CASE("Default constructed nullable should be null.", "[value]") {
    Nullable<Value> optional;
    REQUIRE(!optional);
    REQUIRE(optional.is_null());
    REQUIRE(!optional.has_value());
}

TEST_CASE("Nullable should return the original value", "[value]") {
    Nullable<Value> optional = SmallInteger::make(1234);
    REQUIRE(optional);
    REQUIRE(!optional.is_null());
    REQUIRE(optional.has_value());

    REQUIRE(optional.value().is<SmallInteger>());
    REQUIRE(optional.value().must_cast<SmallInteger>().value());
}
