#include <catch2/catch.hpp>

#include "common/adt/bitset.hpp"

#include <array>

namespace tiro::test {

namespace {

struct Storage {
    Span<u32> span() { return data; }
    u32 bits() { return type_bits<u32>() * 16; }

    u32 data[16]{};
};

} // namespace

TEST_CASE("bitset set and clear should modify the correct bits", "[bitset]") {
    Storage storage;
    BitsetView bitset(storage.span(), storage.bits());

    bitset.set(0);
    REQUIRE(bitset.test(0));
    REQUIRE(storage.data[0] == 1);

    bitset.clear(0);
    REQUIRE(!bitset.test(0));
    REQUIRE(storage.data[0] == 0);

    bitset.set(38);
    REQUIRE(storage.data[1] == 1 << 6);

    bitset.set(39);
    REQUIRE(storage.data[1] == ((1 << 6) | (1 << 7)));

    bitset.clear(38);
    REQUIRE(storage.data[1] == 1 << 7);
}

TEST_CASE("bitset count should return the number of set bits", "[bitset]") {
    Storage storage;
    BitsetView bitset(storage.span(), storage.bits());
    REQUIRE(bitset.count() == 0);

    bitset.set(155);
    REQUIRE(bitset.count() == 1);
    REQUIRE(bitset.count(0, 155) == 0);
    REQUIRE(bitset.count(0, 156) == 1);
    REQUIRE(bitset.count(155, 1) == 1);
    REQUIRE(bitset.count(155, 0) == 0);
    REQUIRE(bitset.count(156) == 0);

    bitset.set(300);
    REQUIRE(bitset.count() == 2);

    bitset.clear();
    REQUIRE(bitset.count() == 0);

    for (int i = 55; i < 455; i += 1) {
        bitset.set(i);
    }
    REQUIRE(bitset.count() == 400);
}

TEST_CASE("bitset find_set should find the next set bit", "[bitset]") {
    Storage storage;
    BitsetView bitset(storage.span(), storage.bits());
    REQUIRE(bitset.find_set() == bitset.npos);

    bitset.set(5);
    REQUIRE(bitset.find_set() == 5);
    REQUIRE(bitset.find_set(5) == 5);
    REQUIRE(bitset.find_set(6) == bitset.npos);

    bitset.set(444);
    REQUIRE(bitset.find_set(5) == 5);
    REQUIRE(bitset.find_set(6) == 444);

    bitset.clear();
    bitset.set(32);
    REQUIRE(bitset.find_set() == 32);
}

TEST_CASE("bitset find_unset should find the next unset bit", "[bitset]") {
    Storage storage;
    BitsetView bitset(storage.span(), storage.bits());
    REQUIRE(bitset.find_unset() == 0);

    bitset.set(5);
    REQUIRE(bitset.find_unset(4) == 4);
    REQUIRE(bitset.find_unset(5) == 6);
    REQUIRE(bitset.find_unset(6) == 6);

    bitset.set(32);
    REQUIRE(bitset.find_unset(31) == 31);
    REQUIRE(bitset.find_unset(32) == 33);
    REQUIRE(bitset.find_unset(33) == 33);

    for (size_t i = 100; i < bitset.size(); ++i)
        bitset.set(i, true);

    REQUIRE(bitset.find_unset(99) == 99);
    REQUIRE(bitset.find_unset(100) == bitset.npos);
}

TEST_CASE("bitset range clear should set bits to 0", "[bitset])") {
    Storage storage;
    auto storage_span = storage.span();
    std::fill(storage_span.begin(), storage_span.end(), ~u32(0)); // fill with ones

    BitsetView bitset(storage_span, storage.bits());
    REQUIRE(bitset.count() == storage.bits());

    auto assert_unset = [&](size_t start, size_t end) {
        CAPTURE(start, end);

        for (size_t i = start; i < end; ++i) {
            CAPTURE(i);
            REQUIRE(!bitset.test(i));
        }

        // Other bits are still 1s
        REQUIRE(bitset.count() == storage.bits() - (end - start));
    };

    SECTION("empty range") {
        bitset.clear(123, 0);
        REQUIRE(bitset.count() == storage.bits());
    }

    SECTION("same block with space left only") {
        bitset.clear(35, 29);
        assert_unset(35, 64);
    }

    SECTION("same block with space right only") {
        bitset.clear(62, 2);
        assert_unset(62, 64);
    }

    SECTION("same block with space left and right") {
        bitset.clear(35, 5);
        assert_unset(35, 40);
    }

    SECTION("different blocks (no full blocks between)") {
        bitset.clear(38, 30);
        assert_unset(38, 68);
    }

    SECTION("large number of blocks") {
        bitset.clear(33, 222);
        assert_unset(33, 255);
    }
}

TEST_CASE("dynamic bitset should compute its size correctly", "[bitset]") {
    DynamicBitset set1(128);
    REQUIRE(set1.raw_blocks().size() == 2);

    DynamicBitset set2(129);
    REQUIRE(set2.raw_blocks().size() == 3);
}

TEST_CASE("dynamic bitset should support initial size", "[bitset]") {
    DynamicBitset s(16);
    REQUIRE(s.size() == 16);
    REQUIRE(s.count() == 0);
}

TEST_CASE("dynamic bitset should support dynamic size", "[bitset]") {
    DynamicBitset s(16);

    s.resize(15);
    REQUIRE(s.size() == 15);
    REQUIRE(s.count() == 0);
}

TEST_CASE("dynamic bitset should support setting and clearing of bits", "[bitset]") {
    DynamicBitset s(16);

    s.set(15);
    REQUIRE(s.test(15));
    REQUIRE(s.count() == 1);

    s.set(3);
    REQUIRE(s.test(3));
    REQUIRE(s.count() == 2);

    s.set(3, false);
    REQUIRE_FALSE(s.test(3));
    REQUIRE(s.count() == 1);

    s.clear(15);
    REQUIRE_FALSE(s.test(15));
    REQUIRE(s.count() == 0);
}

TEST_CASE("dynamic bitset should support flipping single bits", "[bitset]") {
    DynamicBitset s(16);

    s.flip(15);
    REQUIRE(s.test(15));

    s.flip(15);
    REQUIRE_FALSE(s.test(15));
}

TEST_CASE("dynamic bitset should support flipping all bits", "[bitset]") {
    DynamicBitset s(999);

    s.flip();
    REQUIRE(s.count() == 999);

    s.flip();
    REQUIRE(s.count() == 0);
}

TEST_CASE("dynamic bitset should be able to find set bits", "[bitset]") {
    DynamicBitset s(999);
    s.set(3);
    s.set(7);
    s.set(11);
    s.set(23);
    s.set(123);
    s.set(998);

    std::vector<size_t> expected{3, 7, 11, 23, 123, 998};
    std::vector<size_t> found;
    {
        size_t start = 0;
        while (1) {
            size_t pos = s.find_set(start);
            if (pos == s.npos)
                break;

            found.push_back(pos);
            start = pos + 1;
        }
    }

    REQUIRE(found == expected);
}

TEST_CASE("dynamic bitset should be able to find unset bits", "[bitset]") {
    DynamicBitset s(999);
    s.set(3);
    s.set(7);
    s.set(11);
    s.set(23);
    s.set(123);
    s.set(998);
    s.flip();

    std::vector<size_t> expected{3, 7, 11, 23, 123, 998};
    std::vector<size_t> found;
    {
        size_t start = 0;
        while (1) {
            size_t pos = s.find_unset(start);
            if (pos == s.npos)
                break;

            found.push_back(pos);
            start = pos + 1;
        }
    }

    REQUIRE(found == expected);
}

} // namespace tiro::test
