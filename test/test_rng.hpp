#ifndef TIRO_TEST_RNG_HPP
#define TIRO_TEST_RNG_HPP

#include "tiro/core/defs.hpp"

#include <memory>
#include <random>

namespace tiro {

class TestRng {
public:
    explicit TestRng(uint64_t seed)
        : rng_(std::make_unique<std::mt19937_64>(seed)) {}

private:
    template<typename T>
    T generate_int(T min, T max) const {
        return std::uniform_int_distribution<T>(min, max)(*rng_);
    }

public:
    i32 next_i32() const {
        return generate_int(
            std::numeric_limits<i32>::min(), std::numeric_limits<i32>::max());
    }

    i32 next_i32(i32 min, i32 max) const { return generate_int(min, max); }

    i64 next_i64() const {
        return generate_int(
            std::numeric_limits<i64>::min(), std::numeric_limits<i64>::max());
    }

    i64 next_i64(i64 min, i64 max) const { return generate_int(min, max); }

private:
    std::unique_ptr<std::mt19937_64> rng_;
};

}; // namespace tiro

#endif // TIRO_TEST_RNG_HPP
