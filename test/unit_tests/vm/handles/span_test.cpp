#include <catch2/catch.hpp>

#include "vm/handles/span.hpp"
#include "vm/objects/primitives.hpp"

#include <array>
#include <iterator>
#include <type_traits>

namespace tiro::vm::test {

TEMPLATE_TEST_CASE("HandleSpans should be empty by default", "[handle-span]", HandleSpan<Value>,
    MutHandleSpan<Value>) {
    TestType span;
    REQUIRE(span.empty());
    REQUIRE(span.size() == 0);
    REQUIRE(std::distance(span.begin(), span.end()) == 0);
}

TEMPLATE_TEST_CASE("HandleSpans should be able to reference a span of values", "[handle-span]",
    HandleSpan<Value>, MutHandleSpan<Value>) {
    std::array<Value, 17> slots;
    TestType span(slots);
    REQUIRE(!span.empty());
    REQUIRE(span.size() == 17);
    REQUIRE(std::distance(span.begin(), span.end()) == 17);
}

TEMPLATE_TEST_CASE("HandleSpans should support element access", "[handle-span]", HandleSpan<Value>,
    MutHandleSpan<Value>) {

    std::array<Value, 17> slots;
    slots[5] = SmallInteger::make(123);

    TestType span(slots);
    REQUIRE(span[5].template must_cast<SmallInteger>()->value() == 123);
}

TEMPLATE_TEST_CASE("HandleSpans should support iteration", "[handle-span]", HandleSpan<Value>,
    MutHandleSpan<Value>) {

    std::vector<Value> slots;
    for (size_t i = 0; i < 123; ++i) {
        slots.push_back(SmallInteger::make(i));
    }

    std::vector<Value> seen;
    for (auto handle : TestType(slots)) {
        seen.push_back(*handle);
    }

    REQUIRE(seen.size() == 123);
    REQUIRE(std::equal(slots.begin(), slots.end(), seen.begin(), seen.end(),
        [](Value a, Value b) { return a.same(b); }));
}

TEST_CASE("Immutable HandleSpans should be convertible to their parent types", "[handle-span]") {
    STATIC_REQUIRE(std::is_convertible_v<HandleSpan<Integer>, HandleSpan<HeapValue>>);
    STATIC_REQUIRE(std::is_convertible_v<HandleSpan<Integer>, HandleSpan<Value>>);
    STATIC_REQUIRE(!std::is_convertible_v<HandleSpan<HeapValue>, HandleSpan<Integer>>);
}

TEST_CASE(
    "MutHandleSpans should not be convertible to their immutable counterparts", "[handle-span]") {
    STATIC_REQUIRE(std::is_convertible_v<MutHandleSpan<Integer>, HandleSpan<HeapValue>>);
    STATIC_REQUIRE(std::is_convertible_v<MutHandleSpan<Integer>, HandleSpan<Value>>);
    STATIC_REQUIRE(!std::is_convertible_v<MutHandleSpan<HeapValue>, HandleSpan<Integer>>);
}

TEST_CASE("MutHandleSpans should not be convertible to their parent types", "[handle-span]") {
    STATIC_REQUIRE(!std::is_convertible_v<MutHandleSpan<Integer>, MutHandleSpan<HeapValue>>);
    STATIC_REQUIRE(!std::is_convertible_v<MutHandleSpan<Integer>, MutHandleSpan<Value>>);
    STATIC_REQUIRE(!std::is_convertible_v<MutHandleSpan<HeapValue>, MutHandleSpan<Integer>>);
}

} // namespace tiro::vm::test
