#include <catch2/catch.hpp>

#include "vm/context.hpp"
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
        {ValueType::Code, false},
        {ValueType::Float, false},
        {ValueType::Integer, false},
        {ValueType::NativeObject, false},
        {ValueType::NativePointer, false},
        {ValueType::Null, false},
        {ValueType::SmallInteger, false},
        {ValueType::String, false},
        {ValueType::Undefined, false},

        {ValueType::Array, true},
        {ValueType::ArrayStorage, true},
        {ValueType::BoundMethod, true},
        {ValueType::Coroutine, true},
        {ValueType::CoroutineStack, true},
        {ValueType::CoroutineToken, true},
        {ValueType::Environment, true},
        {ValueType::Function, true},
        {ValueType::FunctionTemplate, true},
        {ValueType::HashTable, true},
        {ValueType::HashTableIterator, true},
        {ValueType::HashTableKeyIterator, true},
        {ValueType::HashTableKeyView, true},
        {ValueType::HashTableStorage, true},
        {ValueType::HashTableValueIterator, true},
        {ValueType::HashTableValueView, true},
        {ValueType::Method, true},
        {ValueType::Module, true},
        {ValueType::NativeFunction, true},
        {ValueType::Record, true},
        {ValueType::RecordTemplate, true},
        {ValueType::Result, true},
        {ValueType::Set, true},
        {ValueType::SetIterator, true},
        {ValueType::StringBuilder, true},
        {ValueType::StringIterator, true},
        {ValueType::StringSlice, true},
        {ValueType::Symbol, true},
        {ValueType::Tuple, true},
        {ValueType::Type, true},
        {ValueType::UnresolvedImport, true},
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

TEST_CASE("Equality of numbers should be implemented correctly", "[value]") {
    Context ctx;
    Scope sc(ctx);

    Local i_1 = sc.local(Integer::make(ctx, 1));
    Local i_2 = sc.local(Integer::make(ctx, 2));
    Local si_1 = sc.local(SmallInteger::make(1));
    Local si_2 = sc.local(SmallInteger::make(2));
    Local f_1 = sc.local(Float::make(ctx, 1));
    Local f_1_5 = sc.local(Float::make(ctx, 1.5));
    Local f_nan = sc.local(Float::make(ctx, std::nan("")));

    struct Test {
        Handle<Value> lhs;
        Handle<Value> rhs;
        bool expected_equal;
    };

    Test tests[] = {
        // Reflexive properties
        {i_1, i_1, true},
        {si_1, si_1, true},
        {f_1, f_1, true},
        {f_nan, f_nan, false},

        // Comparison to values of the same type
        {i_2, i_1, false},
        {i_1, i_2, false},
        {si_1, si_2, false},
        {si_2, si_1, false},
        {f_1, f_1_5, false},
        {f_1_5, f_1, false},

        // Comparison int <-> small int
        {i_1, si_1, true},
        {si_1, i_1, true},
        {i_1, si_2, false},
        {si_2, i_1, false},

        // Comparison int <-> float
        {i_1, f_1, true},
        {f_1, i_1, true},
        {i_2, f_1_5, false},
        {f_1_5, i_2, false},

        // Comparison small int <-> float
        {si_1, f_1, true},
        {f_1, si_1, true},
        {si_2, f_1_5, false},
        {f_1_5, si_2, false},
    };

    for (const auto& test : tests) {
        INFO("lhs = " << to_string(*test.lhs) << " rhs = " << to_string(*test.rhs) << ", expected "
                      << (test.expected_equal ? "true" : "false"));

        bool is_equal = equal(*test.lhs, *test.rhs);
        REQUIRE(test.expected_equal == is_equal);
    }
}
