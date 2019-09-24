#include "hammer/core/unicode.hpp"

#include <utf8.h>

namespace hammer {

template<typename Key, typename Value>
auto sparse_map_find(
    Span<const unicode_data::MapEntry<Key, Value>> sparse_map, Key key) {

    // Find the first entry with a key greater than `key`.
    auto pos = std::upper_bound(sparse_map.begin(), sparse_map.end(), key,
        [&](const Key& key_, const auto& entry) { return key_ < entry.key; });
    HAMMER_ASSERT(pos != sparse_map.begin(),
        "The first entry must not be greater than any key.");
    --pos;

    HAMMER_ASSERT(key >= pos->key, "Must have found the lower bound.");
    return pos->value;
}

template<typename Key>
bool sparse_set_contains(
    Span<const unicode_data::Interval<Key>> sparse_set, Key key) {

    // Find the first interval that has last >= key
    auto pos = std::lower_bound(sparse_set.begin(), sparse_set.end(), key,
        [&](const auto& entry, const Key& key_) { return entry.last < key_; });
    if (pos == sparse_set.end())
        return false;

    return pos->first <= key;
}

std::string_view to_string(GeneralCategory category) {
    switch (category) {
#define HAMMER_CASE(cat)       \
    case GeneralCategory::cat: \
        return #cat;

        HAMMER_CASE(Invalid)
        HAMMER_CASE(Cc)
        HAMMER_CASE(Cf)
        HAMMER_CASE(Cn)
        HAMMER_CASE(Co)
        HAMMER_CASE(Cs)
        HAMMER_CASE(Ll)
        HAMMER_CASE(Lm)
        HAMMER_CASE(Lo)
        HAMMER_CASE(Lt)
        HAMMER_CASE(Lu)
        HAMMER_CASE(Mc)
        HAMMER_CASE(Me)
        HAMMER_CASE(Mn)
        HAMMER_CASE(Nd)
        HAMMER_CASE(Nl)
        HAMMER_CASE(No)
        HAMMER_CASE(Pc)
        HAMMER_CASE(Pd)
        HAMMER_CASE(Pe)
        HAMMER_CASE(Pf)
        HAMMER_CASE(Pi)
        HAMMER_CASE(Po)
        HAMMER_CASE(Ps)
        HAMMER_CASE(Sc)
        HAMMER_CASE(Sk)
        HAMMER_CASE(Sm)
        HAMMER_CASE(So)
        HAMMER_CASE(Zl)
        HAMMER_CASE(Zp)
        HAMMER_CASE(Zs)

#undef HAMMER_CASE
    }

    HAMMER_UNREACHABLE("Invalid category.");
}

GeneralCategory general_category(CodePoint cp) {
    return sparse_map_find(unicode_data::cps_to_cat, cp);
}

bool is_letter(CodePoint cp) {
    static constexpr GeneralCategory letter_cats[] = {
        GeneralCategory::Ll,
        GeneralCategory::Lm,
        GeneralCategory::Lo,
        GeneralCategory::Lt,
        GeneralCategory::Lu,
    };
    return contains(letter_cats, general_category(cp));
}

bool is_number(CodePoint cp) {
    static constexpr GeneralCategory number_cats[] = {
        GeneralCategory::Nd, GeneralCategory::Nl, GeneralCategory::No};
    return contains(number_cats, general_category(cp));
}

bool is_whitespace(CodePoint cp) {
    return sparse_set_contains(unicode_data::is_whitespace, cp);
}

std::tuple<CodePoint, const char*>
decode_code_point(const char* pos, const char* end) {
    if (pos == end) {
        return std::tuple(invalid_code_point, end);
    }

    try {
        auto cp = utf8::next(pos, end);
        return std::tuple(cp, pos);
    } catch (const utf8::exception&) {
        // TODO What to do with exceptions? Validate utf8 before iterating?
        HAMMER_ERROR("Invalid utf8.");
    }
}

std::string code_point_to_string(CodePoint cp) {
    std::string result;
    append_code_point(result, cp);
    return result;
}

void append_code_point(std::string& buffer, CodePoint cp) {
    utf8::append(cp, std::back_inserter(buffer));
}

} // namespace hammer
