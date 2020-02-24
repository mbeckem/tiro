#ifndef TIRO_CORE_HASH_HPP
#define TIRO_CORE_HASH_HPP

#include "tiro/core/defs.hpp"

#include <functional>

namespace tiro {

template<typename T, typename Enable = void>
struct EnableBuildHash : std::false_type {};

#define TIRO_ENABLE_BUILD_HASH(T) \
    template<>                    \
    struct tiro::EnableBuildHash<T> : ::std::true_type {};

/// A stateful hash builder. Hashable objects or raw hash values can be passed
/// to `append()` or `append_raw`, which will combine the given hash value with
/// the existing one.
///
/// The current hash value can be retrived with `hash()`.
struct Hasher final {
public:
    /// Constructs a hasher.
    Hasher() = default;

    /// Constructs a hasher with `seed` as the initial hash value.
    explicit Hasher(size_t seed)
        : hash_(seed) {}

    /// Appends the hash (computed via std::hash<T>) of "v" to this builder.
    template<typename T>
    Hasher& append(const T& value) noexcept {
        if constexpr (EnableBuildHash<T>::value) {
            value.build_hash(*this);
        } else {
            default_hash(value);
        }
        return *this;
    }

    /// Appends the raw hash value t this builder.
    Hasher& append_raw(size_t raw_hash) noexcept {
        // Default impl from boost::hash_combine.
        // Could probably need improvement / specialization for 32/64 bit.
        hash_ ^= raw_hash + 0x9e3779b9 + (hash_ << 6) + (hash_ >> 2);
        return *this;
    }

    /// Returns the current hash value.
    size_t hash() const noexcept { return hash_; }

    // Copy disabled to prevent silly misakes.
    Hasher(const Hasher&) = delete;
    Hasher& operator=(const Hasher&) = delete;

private:
    template<typename... T>
    void default_hash(const std::tuple<T...>& tuple) {
        // call append() for every tuple element.
        std::apply(
            [this](const auto&... args) { (this->append(args), ...); }, tuple);
    }

    template<typename T>
    void default_hash(const T& value) {
        append_raw(std::hash<T>()(value));
    }

private:
    size_t hash_ = 0;
};

/// Hash function object for containers.
/// The value type must implement the `void build_hash(hash_builder&) const` member function
/// or support the normal the hasher's default hash algorithm based on `std::hash<T>`.
struct UseHasher {
    template<typename T>
    size_t operator()(const T& value) const noexcept {
        Hasher h;
        h.append(value);
        return h.hash();
    }
};

} // namespace tiro

#endif // TIRO_CORE_HASH_HPP
