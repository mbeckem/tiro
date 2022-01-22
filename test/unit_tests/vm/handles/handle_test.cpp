#include <catch2/catch.hpp>

#include "vm/handles/handle.hpp"
#include "vm/objects/nullable.hpp"
#include "vm/objects/primitives.hpp"

namespace tiro::vm::test {

TEMPLATE_TEST_CASE("Handles should refer to the contents of their slot", "[handle]",
    Handle<SmallInteger>, MutHandle<SmallInteger>) {
    SmallInteger slot = SmallInteger::make(123);

    TestType handle(&slot);
    REQUIRE(handle->value() == 123);
    REQUIRE(handle.get().value() == 123);
    REQUIRE((*handle).value() == 123);
    REQUIRE(handle->template is<SmallInteger>());
}

TEST_CASE("Handles should be implicitly convertible to their parent types", "[handle]") {
    STATIC_REQUIRE(std::is_convertible_v<Handle<HeapInteger>, Handle<HeapValue>>);
    STATIC_REQUIRE(std::is_convertible_v<Handle<HeapInteger>, Handle<Value>>);
    STATIC_REQUIRE(!std::is_convertible_v<Handle<Value>, Handle<HeapInteger>>);
}

TEMPLATE_TEST_CASE(
    "Handles should be able to cast to a child type", "[handle]", Handle<Value>, MutHandle<Value>) {
    SmallInteger slot = SmallInteger::make(123);

    TestType value_handle(&slot);

    auto try_result_ok = value_handle.template try_cast<SmallInteger>();
    REQUIRE(try_result_ok.valid());

    auto try_result_fail = value_handle.template try_cast<HeapInteger>();
    REQUIRE_FALSE(try_result_fail.valid());

    auto must_result = value_handle.template must_cast<SmallInteger>();
    REQUIRE(must_result->value() == 123);
}

TEST_CASE("MutHandles should provide write access to the slot", "[handle]") {
    SmallInteger slot = SmallInteger::make(123);

    MutHandle<SmallInteger> handle(&slot);
    handle.set(SmallInteger::make(456));
    REQUIRE(handle->value() == 456);
    REQUIRE(slot.value() == 456);
}

TEST_CASE("MutHandles should be convertible to their immutable counterparts", "[handle]") {
    STATIC_REQUIRE(std::is_convertible_v<MutHandle<HeapInteger>, Handle<HeapInteger>>);
    STATIC_REQUIRE(std::is_convertible_v<MutHandle<HeapInteger>, Handle<HeapValue>>);
    STATIC_REQUIRE(std::is_convertible_v<MutHandle<HeapInteger>, Handle<Value>>);
    STATIC_REQUIRE(!std::is_convertible_v<MutHandle<Value>, Handle<HeapInteger>>);
}

TEST_CASE("MutHandles should not be convertible to their parent types", "[handle]") {
    STATIC_REQUIRE(!std::is_convertible_v<MutHandle<HeapInteger>, MutHandle<HeapValue>>);
    STATIC_REQUIRE(!std::is_convertible_v<MutHandle<HeapInteger>, MutHandle<Value>>);
    STATIC_REQUIRE(!std::is_convertible_v<MutHandle<Value>, MutHandle<HeapInteger>>);
}

TEST_CASE("MutHandles should be convertible to their derived output counterparts", "[handle]") {
    STATIC_REQUIRE(std::is_convertible_v<MutHandle<HeapInteger>, OutHandle<HeapInteger>>);
    STATIC_REQUIRE(std::is_convertible_v<MutHandle<Value>, OutHandle<HeapValue>>);
    STATIC_REQUIRE(std::is_convertible_v<MutHandle<Value>, OutHandle<HeapInteger>>);
    STATIC_REQUIRE(!std::is_convertible_v<MutHandle<HeapInteger>, OutHandle<Value>>);
}

TEST_CASE("OutHandles should be convertible to their derived types", "[handle]") {
    STATIC_REQUIRE(std::is_convertible_v<OutHandle<HeapInteger>, OutHandle<HeapInteger>>);
    STATIC_REQUIRE(std::is_convertible_v<OutHandle<Value>, OutHandle<HeapValue>>);
    STATIC_REQUIRE(std::is_convertible_v<OutHandle<Value>, OutHandle<HeapInteger>>);
    STATIC_REQUIRE(!std::is_convertible_v<OutHandle<HeapInteger>, OutHandle<Value>>);
}

TEMPLATE_TEST_CASE("Default constructed MaybeHandles should be invalid", "[handle]",
    MaybeHandle<Value>, MaybeMutHandle<Value>) {
    TestType handle;
    REQUIRE_FALSE(handle);
    REQUIRE_FALSE(handle.valid());
}

TEMPLATE_TEST_CASE("MaybeHandles that refer to a slot should be convertible to a real handle",
    "[handle]", MaybeHandle<Value>, MaybeMutHandle<Value>) {
    Value slot = SmallInteger::make(123);

    TestType maybe(&slot);
    REQUIRE(maybe);
    REQUIRE(maybe.valid());

    auto handle = maybe.handle();
    REQUIRE(handle->template is<SmallInteger>());
    REQUIRE(handle.template must_cast<SmallInteger>()->value() == 123);
}

TEST_CASE("MaybeHandles that refer to a slot should be convertible to a valid, nullable handle",
    "[handle]") {
    SmallInteger si = SmallInteger::make(123);

    MaybeHandle<SmallInteger> maybe(&si);
    REQUIRE(maybe);

    Handle<Nullable<SmallInteger>> nullable = maybe.to_nullable();
    REQUIRE(!nullable->is_null());
    REQUIRE(nullable->value().value() == 123);
}

TEST_CASE("Empty MaybeHandles should be convertible to a null handle", "[handle]") {
    MaybeHandle<SmallInteger> maybe;
    REQUIRE_FALSE(maybe);

    Handle<Nullable<SmallInteger>> nullable = maybe.to_nullable();
    REQUIRE(nullable->is_null());
}

TEST_CASE(
    "MaybeHandle instances should be implicitly convertible to their parent types", "[handle]") {
    STATIC_REQUIRE(std::is_convertible_v<MaybeHandle<HeapInteger>, MaybeHandle<HeapValue>>);
    STATIC_REQUIRE(std::is_convertible_v<MaybeHandle<HeapInteger>, MaybeHandle<Value>>);
    STATIC_REQUIRE(!std::is_convertible_v<MaybeHandle<Value>, MaybeHandle<HeapInteger>>);
}

TEST_CASE(
    "MaybeMutHandle instances should be implicitly convertible to their immutable counterpart",
    "[handle]") {
    STATIC_REQUIRE(std::is_convertible_v<MaybeMutHandle<HeapInteger>, MaybeHandle<HeapValue>>);
    STATIC_REQUIRE(std::is_convertible_v<MaybeMutHandle<HeapInteger>, MaybeHandle<Value>>);
    STATIC_REQUIRE(!std::is_convertible_v<MaybeMutHandle<Value>, MaybeHandle<HeapInteger>>);
}

TEST_CASE("MaybeMutHandles should not be convertible to their parent types", "[handle]") {
    STATIC_REQUIRE(!std::is_convertible_v<MaybeMutHandle<HeapInteger>, MaybeMutHandle<HeapValue>>);
    STATIC_REQUIRE(!std::is_convertible_v<MaybeMutHandle<HeapInteger>, MaybeMutHandle<Value>>);
    STATIC_REQUIRE(!std::is_convertible_v<MaybeMutHandle<Value>, MaybeMutHandle<HeapInteger>>);
}

TEST_CASE(
    "MaybeMutHandles should be convertible to their derived output counterparts", "[handle]") {
    STATIC_REQUIRE(std::is_convertible_v<MaybeMutHandle<HeapInteger>, MaybeOutHandle<HeapInteger>>);
    STATIC_REQUIRE(std::is_convertible_v<MaybeMutHandle<Value>, MaybeOutHandle<HeapValue>>);
    STATIC_REQUIRE(std::is_convertible_v<MaybeMutHandle<Value>, MaybeOutHandle<HeapInteger>>);
    STATIC_REQUIRE(!std::is_convertible_v<MaybeMutHandle<HeapInteger>, MaybeOutHandle<Value>>);
}

TEST_CASE("MaybeOutHandles should be convertible to their derived types", "[handle]") {
    STATIC_REQUIRE(std::is_convertible_v<MaybeOutHandle<HeapInteger>, MaybeOutHandle<HeapInteger>>);
    STATIC_REQUIRE(std::is_convertible_v<MaybeOutHandle<Value>, MaybeOutHandle<HeapValue>>);
    STATIC_REQUIRE(std::is_convertible_v<MaybeOutHandle<Value>, MaybeOutHandle<HeapInteger>>);
    STATIC_REQUIRE(!std::is_convertible_v<MaybeOutHandle<HeapInteger>, MaybeOutHandle<Value>>);
}

TEST_CASE("Handles should be implicitly convertible to their maybe counterparts", "[handle]") {
    // Handle only upcasts
    STATIC_REQUIRE(std::is_convertible_v<Handle<Value>, MaybeHandle<Value>>);
    STATIC_REQUIRE(std::is_convertible_v<Handle<HeapInteger>, MaybeHandle<Value>>);
    STATIC_REQUIRE(!std::is_convertible_v<Handle<Value>, MaybeHandle<HeapInteger>>);

    // MutHandle upcasts to MaybeHandle
    STATIC_REQUIRE(std::is_convertible_v<MutHandle<Value>, MaybeHandle<Value>>);
    STATIC_REQUIRE(std::is_convertible_v<MutHandle<HeapInteger>, MaybeHandle<Value>>);
    STATIC_REQUIRE(!std::is_convertible_v<MutHandle<Value>, MaybeHandle<HeapInteger>>);

    // MutHandle does not down or upcast to MaybeMutHandle
    STATIC_REQUIRE(std::is_convertible_v<MutHandle<HeapInteger>, MaybeMutHandle<HeapInteger>>);
    STATIC_REQUIRE(!std::is_convertible_v<MutHandle<Value>, MaybeMutHandle<HeapInteger>>);
    STATIC_REQUIRE(!std::is_convertible_v<MutHandle<HeapInteger>, MaybeMutHandle<Value>>);

    // MutHandle downcasts to MaybeOutHandle
    STATIC_REQUIRE(std::is_convertible_v<MutHandle<Value>, MaybeOutHandle<Value>>);
    STATIC_REQUIRE(std::is_convertible_v<MutHandle<Value>, MaybeOutHandle<HeapInteger>>);
    STATIC_REQUIRE(!std::is_convertible_v<MutHandle<HeapInteger>, MaybeOutHandle<Value>>);

    // OutHandle downcasts to MaybeOutHandle
    STATIC_REQUIRE(std::is_convertible_v<OutHandle<Value>, MaybeOutHandle<Value>>);
    STATIC_REQUIRE(std::is_convertible_v<OutHandle<Value>, MaybeOutHandle<HeapInteger>>);
    STATIC_REQUIRE(!std::is_convertible_v<OutHandle<HeapInteger>, MaybeOutHandle<Value>>);
}

TEST_CASE("Handle types should have pointer size",
    "[handle]"
#ifdef _MSC_VER
    "[!shouldfail]" // EBO support on msvc?
#endif
) {
    REQUIRE(sizeof(Handle<Value>) == sizeof(void*));
    REQUIRE(sizeof(MutHandle<Value>) == sizeof(void*));
    REQUIRE(sizeof(OutHandle<Value>) == sizeof(void*));
    REQUIRE(sizeof(MaybeHandle<Value>) == sizeof(void*));
    REQUIRE(sizeof(MaybeMutHandle<Value>) == sizeof(void*));
    REQUIRE(sizeof(MaybeOutHandle<Value>) == sizeof(void*));
}

} // namespace tiro::vm::test
