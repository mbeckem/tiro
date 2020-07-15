#include <catch2/catch.hpp>

#include "common/safe_int.hpp"

using namespace tiro;

template<typename T>
struct TypeWrapper {
    using type = T;
};

TEST_CASE("SafeInt should throw on overflow", "[safe-int]") {
    auto tests = [&](auto type) {
        using T = typename decltype(type)::type;
        using limits = std::numeric_limits<T>;

        {
            SafeInt<T> v(limits::max());
            REQUIRE_THROWS(v + 1);
        }

        {
            SafeInt<T> v(limits::min());
            REQUIRE_THROWS(v - 1);
        }

        {
            SafeInt<T> v(limits::max());
            REQUIRE_THROWS(v * 2);
        }

        if constexpr (std::is_signed_v<T>) {
            SafeInt<T> v(limits::min());
            REQUIRE_THROWS(v / T(-1));
        }

        {
            SafeInt<T> v(5);
            REQUIRE_THROWS(v / 0);
        }

        {
            SafeInt<T> v(5);
            REQUIRE_THROWS(v % 0);
        }
    };

    tests(TypeWrapper<byte>());
    tests(TypeWrapper<i32>());
    tests(TypeWrapper<u32>());
    tests(TypeWrapper<i64>());
    tests(TypeWrapper<u64>());
}
