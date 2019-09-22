#ifndef HAMMER_CORE_UNICODE_HPP
#define HAMMER_CORE_UNICODE_HPP

#include "hammer/core/defs.hpp"
#include "hammer/core/span.hpp"

#include <algorithm>

namespace hammer {

using CodePoint = u32;

enum class GeneralCategory {
    Invalid,
    Cc,
    Cf,
    Cn,
    Co,
    Cs,
    Ll,
    Lm,
    Lo,
    Lt,
    Lu,
    Mc,
    Me,
    Mn,
    Nd,
    Nl,
    No,
    Pc,
    Pd,
    Pe,
    Pf,
    Pi,
    Po,
    Ps,
    Sc,
    Sk,
    Sm,
    So,
    Zl,
    Zp,
    Zs,
};

std::string_view to_string(GeneralCategory category);

namespace unicode_data {

template<typename Key, typename Value>
struct MapEntry {
    Key key{};
    Value value{};

    constexpr MapEntry() = default;

    constexpr MapEntry(const Key& k, const Value& v)
        : key(k)
        , value(v) {}

    constexpr MapEntry(const MapEntry&) = default;
    constexpr MapEntry& operator=(const MapEntry&) = default;
};

// Defined in unicode_data.cpp
extern const Span<const MapEntry<CodePoint, GeneralCategory>> cps_to_cat;

template<typename Key, typename Value>
auto find_value(Span<const MapEntry<Key, Value>> sorted_span, Key key) {
    auto pos = std::upper_bound(sorted_span.begin(), sorted_span.end(), key,
        [&](const Key& key_, const auto& entry) { return key_ < entry.key; });
    HAMMER_ASSERT(pos != sorted_span.begin(),
        "The first entry must not be greater than any key.");
    --pos;

    HAMMER_ASSERT(key >= pos->key, "Must have found the lower bound.");
    return pos->value;
}

} // namespace unicode_data

GeneralCategory general_category(CodePoint point);

} // namespace hammer

#endif // HAMMER_CORE_UNICODE_HPP
