#include "common/text/unicode.hpp"

#include "common/ranges/iter_tools.hpp"

#include <utf8.h>

namespace tiro {

template<typename Key, typename Value>
auto sparse_map_find(Span<const unicode_data::MapEntry<Key, Value>> sparse_map, Key key) {

    // Find the first entry with a key greater than `key`.
    auto pos = std::upper_bound(sparse_map.begin(), sparse_map.end(), key,
        [&](const Key& key_, const auto& entry) { return key_ < entry.key; });
    TIRO_DEBUG_ASSERT(
        pos != sparse_map.begin(), "The first entry must not be greater than any key.");
    --pos;

    TIRO_DEBUG_ASSERT(key >= pos->key, "Must have found the lower bound.");
    return pos->value;
}

template<typename Key>
bool sparse_set_contains(Span<const unicode_data::Interval<Key>> sparse_set, Key key) {

    // Find the first interval that has last >= key
    auto pos = std::lower_bound(sparse_set.begin(), sparse_set.end(), key,
        [&](const auto& entry, const Key& key_) { return entry.last < key_; });
    if (pos == sparse_set.end())
        return false;

    return pos->first <= key;
}

std::string_view to_string(GeneralCategory category) {
    switch (category) {
#define TIRO_CASE(cat)         \
    case GeneralCategory::cat: \
        return #cat;

        TIRO_CASE(Invalid)
        TIRO_CASE(Cc)
        TIRO_CASE(Cf)
        TIRO_CASE(Cn)
        TIRO_CASE(Co)
        TIRO_CASE(Cs)
        TIRO_CASE(Ll)
        TIRO_CASE(Lm)
        TIRO_CASE(Lo)
        TIRO_CASE(Lt)
        TIRO_CASE(Lu)
        TIRO_CASE(Mc)
        TIRO_CASE(Me)
        TIRO_CASE(Mn)
        TIRO_CASE(Nd)
        TIRO_CASE(Nl)
        TIRO_CASE(No)
        TIRO_CASE(Pc)
        TIRO_CASE(Pd)
        TIRO_CASE(Pe)
        TIRO_CASE(Pf)
        TIRO_CASE(Pi)
        TIRO_CASE(Po)
        TIRO_CASE(Ps)
        TIRO_CASE(Sc)
        TIRO_CASE(Sk)
        TIRO_CASE(Sm)
        TIRO_CASE(So)
        TIRO_CASE(Zl)
        TIRO_CASE(Zp)
        TIRO_CASE(Zs)

#undef TIRO_CASE
    }

    TIRO_UNREACHABLE("Invalid category.");
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

bool is_printable(CodePoint cp) {
    if (cp == ' ')
        return true;

    switch (general_category(cp)) {
    case GeneralCategory::Cc:
    case GeneralCategory::Cf:
    case GeneralCategory::Cn:
    case GeneralCategory::Co:
    case GeneralCategory::Cs:
    case GeneralCategory::Zl:
    case GeneralCategory::Zp:
    case GeneralCategory::Zs:
        return false;
    default:
        return true;
    }
}

std::tuple<CodePoint, const char*> decode_utf8(const char* pos, const char* end) {
    if (pos == end) {
        return std::tuple(invalid_code_point, end);
    }

    try {
        auto cp = utf8::next(pos, end);
        return std::tuple(cp, pos);
    } catch (const utf8::exception&) {
        TIRO_ERROR("Invalid utf8.");
    }
}

std::string to_string_utf8(CodePoint cp) {
    std::string result;
    append_utf8(result, cp);
    return result;
}

void append_utf8(std::string& buffer, CodePoint cp) {
    utf8::append(cp, std::back_inserter(buffer));
}

Utf8ValidationResult validate_utf8(std::string_view str) {
    Utf8ValidationResult result;

    auto invalid = utf8::find_invalid(str.begin(), str.end());
    if (invalid == str.end()) {
        result.ok = true;
        return result;
    }

    result.ok = false;
    result.error_offset = static_cast<size_t>(invalid - str.begin());
    return result;
}

} // namespace tiro
