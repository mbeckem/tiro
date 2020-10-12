#include <catch2/catch.hpp>

#include "common/memory/byte_order.hpp"

using namespace tiro;

TEST_CASE("Byte swaps should be executed correctly", "[byte-order]") {
    u16 v1 = 0xff00;
    REQUIRE(byteswap(v1) == 0x00ff);

    u32 v2 = 0xf0f1f2f3;
    REQUIRE(byteswap(v2) == 0xf3f2f1f0);

    u64 v3 = 0xf0f1f2f3f4f5f6f7;
    REQUIRE(byteswap(v3) == 0xf7f6f5f4f3f2f1f0);
}

TEST_CASE("Host to host conversion should not modify the value", "[byte-order]") {
    auto h2h = [](auto v) { return convert_byte_order<ByteOrder::Host, ByteOrder::Host>(v); };

    REQUIRE(h2h(u8(0xf0)) == 0xf0);
    REQUIRE(h2h(u16(0xff00)) == 0xff00);
    REQUIRE(h2h(u32(0xf0f1f2f3)) == 0xf0f1f2f3);
    REQUIRE(h2h(u64(0xf0f1f2f3f4f5f6f7)) == 0xf0f1f2f3f4f5f6f7);
}

TEST_CASE("Conversion between byte orders swaps the bytes", "[byte-order]") {
    auto b2l = [](auto v) {
        return convert_byte_order<ByteOrder::BigEndian, ByteOrder::LittleEndian>(v);
    };

    REQUIRE(b2l(u8(0xf0)) == 0xf0);
    REQUIRE(b2l(u16(0xff00)) == 0x00ff);
    REQUIRE(b2l(u32(0xf0f1f2f3)) == 0xf3f2f1f0);
    REQUIRE(b2l(u64(0xf0f1f2f3f4f5f6f7)) == 0xf7f6f5f4f3f2f1f0);

    auto l2b = [](auto v) {
        return convert_byte_order<ByteOrder::LittleEndian, ByteOrder::BigEndian>(v);
    };

    REQUIRE(l2b(u8(0xf0)) == 0xf0);
    REQUIRE(l2b(u16(0xff00)) == 0x00ff);
    REQUIRE(l2b(u32(0xf0f1f2f3)) == 0xf3f2f1f0);
    REQUIRE(l2b(u64(0xf0f1f2f3f4f5f6f7)) == 0xf7f6f5f4f3f2f1f0);
};
