#include <catch2/catch.hpp>

#include "common/ranges/generator_range.hpp"

namespace tiro::test {

template<typename Range>
static std::vector<typename Range::value_type> gather(Range& r) {
    return std::vector(r.begin(), r.end());
}

TEST_CASE("generator range should infer its value type from the function signature",
    "[generator-range]") {
    auto int_gen = []() -> std::optional<int> { return {}; };

    using IntRange = GeneratorRange<decltype(int_gen)>;
    STATIC_REQUIRE(std::is_same_v<IntRange::value_type, int>);
}

TEST_CASE("empty generator range contains no elements", "[generator-range]") {
    auto gen = []() -> std::optional<int> { return {}; };
    auto range = GeneratorRange(gen);
    auto items = gather(range);
    REQUIRE(items.empty());
}

TEST_CASE("generator range should return all generated items", "[generator-range]") {
    auto gen = [i = 0]() mutable -> std::optional<int> {
        if (i >= 5)
            return {};

        return i++;
    };
    auto range = GeneratorRange(gen);

    std::vector<int> expected{0, 1, 2, 3, 4};
    std::vector<int> actual = gather(range);
    REQUIRE(actual == expected);
}

TEST_CASE("generator range works with move only types", "[generator-range]") {
    struct MoveOnly {
        int value = 0;

        MoveOnly(int value_)
            : value(value_) {}

        MoveOnly(MoveOnly&& other)
            : value(other.value) {
            other.value = -1;
        }

        MoveOnly& operator=(MoveOnly&& other) noexcept {
            value = std::exchange(other.value, -1);
            return *this;
        }
    };

    auto gen = [i = 0]() mutable -> std::optional<MoveOnly> {
        if (i >= 2)
            return {};

        return MoveOnly(i++);
    };

    std::vector<int> seen;
    for (auto&& v : GeneratorRange(gen)) {
        MoveOnly m = std::move(v);
        REQUIRE(v.value == -1);
        seen.push_back(m.value);
    }

    std::vector<int> expected{0, 1};
    REQUIRE(seen == expected);
}

} // namespace tiro::test
