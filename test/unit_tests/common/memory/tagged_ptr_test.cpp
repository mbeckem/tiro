#include <catch2/catch.hpp>

#include "common/memory/tagged_ptr.hpp"

using namespace tiro;

static constexpr size_t align_bits = 4;
static constexpr size_t align = 1 << 4;

using Ptr = TaggedPtr<align_bits>;

namespace {

struct alignas(align) TestValue {
    int foo = 0;
};

} // namespace

TEST_CASE("Null pointer must have zero value", "[tagged-ptr]") {
    REQUIRE(uintptr_t(static_cast<void*>(nullptr)) == 0);
}

TEST_CASE("Tagged pointer should have the expected constant values", "[tagged-ptr]") {
    STATIC_REQUIRE(Ptr::total_bits == sizeof(void*) * CHAR_BIT);
    STATIC_REQUIRE(Ptr::tag_bits == align_bits);
    STATIC_REQUIRE(Ptr::pointer_bits == sizeof(void*) * CHAR_BIT - align_bits);
    STATIC_REQUIRE(Ptr::max_tag_value == align);
    STATIC_REQUIRE(Ptr::pointer_alignment == align);
}

TEST_CASE("Default constructed tagged pointer should be empty", "[tagged-ptr]") {
    Ptr ptr;
    REQUIRE(ptr.pointer() == nullptr);
    REQUIRE(ptr.tag() == 0);
}

TEST_CASE("Tagged pointer can be initialized with a valid pointer", "[tagged-ptr]") {
    TestValue value;
    uintptr_t tag = align - 1; // max tag value
    Ptr ptr(&value, tag);
    REQUIRE(ptr.pointer() == &value);
    REQUIRE(ptr.tag() == tag);
}

TEST_CASE("Tagged pointer allows modification of the current pointer and tag", "[tagged-ptr]") {
    TestValue value1;
    TestValue value2;

    Ptr ptr(&value1);
    REQUIRE(ptr.pointer() == &value1);
    REQUIRE(ptr.tag() == 0);

    ptr.pointer(&value2);
    ptr.tag(12);
    REQUIRE(ptr.pointer() == &value2);
    REQUIRE(ptr.tag() == 12);
}

TEST_CASE("Tagged pointer allows access to individual tag bits", "[tagged-ptr]") {
    Ptr ptr;
    REQUIRE_FALSE(ptr.tag_bit<3>());

    ptr.tag_bit<3>(true);
    REQUIRE(ptr.tag_bit<3>());
    REQUIRE(ptr.tag() == 8);

    ptr.tag_bit<2>(true);
    REQUIRE(ptr.tag_bit<2>());
    REQUIRE(ptr.tag() == 12);

    REQUIRE(ptr.pointer() == nullptr); // unmodified
}
