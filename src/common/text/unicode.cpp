#include "common/text/unicode.hpp"

#include "common/error.hpp"
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

bool is_xid_start(CodePoint cp) {
    return sparse_set_contains(unicode_data::is_xid_start, cp);
}

bool is_xid_continue(CodePoint cp) {
    return is_xid_start(cp) || sparse_set_contains(unicode_data::is_xid_continue_without_start, cp);
}

bool is_whitespace(CodePoint cp) {
    return sparse_set_contains(unicode_data::is_whitespace, cp);
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

bool try_append_utf8(std::string& buffer, CodePoint cp) {
    // Unfortunately utfcpp does not expose this publicly...
    if (!utf8::internal::is_code_point_valid(cp))
        return false;

    utf8::unchecked::append(cp, std::back_inserter(buffer));
    return true;
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
