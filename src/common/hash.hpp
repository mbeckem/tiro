#ifndef TIRO_COMMON_HASH_HPP
#define TIRO_COMMON_HASH_HPP

#include "common/defs.hpp"

#include "absl/hash/hash.h"

#include <functional>

namespace tiro {

template<typename T, typename Enable = void>
struct EnableMemberHash : std::false_type {};

/// Opt into the `value.hash(Hasher&)` syntax.
#define TIRO_ENABLE_MEMBER_HASH(T) \
    template<>                     \
    struct tiro::EnableMemberHash<T> : ::std::true_type {};

/// A stateful hash builder. Hashable objects or raw hash values can be passed
/// to `append()` or `append_raw`, which will combine the given hash value with
/// the existing one.
///
/// The current hash value can be retrived with `hash()`.
struct Hasher final {
public:
    explicit Hasher(absl::HashState state)
        : state_(std::move(state)) {}

    /// Appends the hash of all arguments to this builder.
    template<typename... Args>
    Hasher& append(const Args&... args) noexcept {
        (append_one(args), ...);
        return *this;
    }

    // Copy disabled to prevent silly misakes.
    Hasher(const Hasher&) = delete;
    Hasher& operator=(const Hasher&) = delete;

private:
    template<typename T>
    void append_one(const T& value) noexcept {
        if constexpr (EnableMemberHash<T>::value) {
            value.hash(*this);
        } else {
            default_hash(value);
        }
    }

    template<typename T1, typename T2>
    void default_hash(const std::pair<T1, T2>& pair) {
        append(pair.first, pair.second);
    }

    template<typename... T>
    void default_hash(const std::tuple<T...>& tuple) {
        // call append() for every tuple element.
        std::apply([this](const auto&... args) { this->append(args...); }, tuple);
    }

    template<typename T>
    void default_hash(const T& value) {
        state_ = absl::HashState::combine(std::move(state_), value);
    }

private:
    absl::HashState state_;
};

namespace detail {

template<typename T>
struct UseHasherAbslWrapper {
    const T& value;

    template<typename H>
    friend H AbslHashValue(H state, const UseHasherAbslWrapper<T>& wrapper) {
        Hasher hasher(absl::HashState::Create(&state));
        hasher.append(wrapper.value);
        return state;
    }
};

} // namespace detail

/// Hash function object for containers.
/// The value type must implement the `void hash(Hasher&) const` member function or
/// support the default hash implemention provided by abseil.
struct UseHasher {
    template<typename T>
    size_t operator()(const T& value) const noexcept {
        // Wrap the reference into a type that implements the abseil extension point AbslHashValue.
        // Abseil's hash value is type erased and then forwarded to all callees through the Hasher instance.
        // TODO: Investigate the performance impact of this. The alternative would be to have every `value.hash()`
        // call be a template.
        using wrapper_t = detail::UseHasherAbslWrapper<T>;
        return absl::Hash<wrapper_t>()(wrapper_t{value});
    }
};

} // namespace tiro

#endif // TIRO_COMMON_HASH_HPP
