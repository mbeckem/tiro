#ifndef TIRO_COMMON_TEXT_UNICODE_HPP
#define TIRO_COMMON_TEXT_UNICODE_HPP

#include "common/adt/span.hpp"
#include "common/defs.hpp"

#include <algorithm>

namespace tiro {

using CodePoint = u32;

/// Returns true if the code point has the XID_Start property.
bool is_xid_start(CodePoint cp);

/// Returns true if the code point has the XID_Continue property.
bool is_xid_continue(CodePoint cp);

/// Returns true if `cp` is a whitespace code point.
bool is_whitespace(CodePoint cp);

/// Sentinel value for invalid code points.
inline constexpr CodePoint invalid_code_point = CodePoint(-1);

/// Returns the next code point (at "pos") and the position just after that code point
/// to continue with the iteration. An invalid code point together with "end" will be returned
/// on error.
std::tuple<CodePoint, const char*> decode_utf8(const char* pos, const char* end);

/// Converts the code point to a utf8 string.
std::string to_string_utf8(CodePoint cp);

/// Appends the code point to a utf8 string.
/// NOTE: Throws if `cp` cannot be represented as utf8 (invalid code point).
void append_utf8(std::string& buffer, CodePoint cp);

/// Appends the code point to a utf8 string.
/// Returns false (and does nothing) if `cp` is not a valid code point.
bool try_append_utf8(std::string& buffer, CodePoint cp);

struct Utf8ValidationResult {
    bool ok = false;         // True if the string was OK
    size_t error_offset = 0; // Index of the first invalid byte, if ok == false
};

/// Validates the given string as utf8. Returns whether the string is valid, and if it isn't,
/// the position of the first invalid byte.
Utf8ValidationResult validate_utf8(std::string_view str);

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

template<typename Key>
struct Interval {
    Key first{}; // Inclusive
    Key last{};  // Inclusive

    constexpr Interval() = default;

    constexpr Interval(const Key& f, const Key& l)
        : first(f)
        , last(l) {}

    constexpr Interval(const Interval&) = default;
    constexpr Interval& operator=(const Interval&) = default;
};

// Defined in unicode_data.cpp
extern const Span<const Interval<CodePoint>> is_whitespace;
extern const Span<const Interval<CodePoint>> is_xid_start;
extern const Span<const Interval<CodePoint>> is_xid_continue_without_start;

} // namespace unicode_data

} // namespace tiro

#endif // TIRO_COMMON_TEXT_UNICODE_HPP
