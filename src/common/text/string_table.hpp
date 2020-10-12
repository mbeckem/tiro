#ifndef TIRO_COMMON_TEXT_STRING_TABLE_HPP
#define TIRO_COMMON_TEXT_STRING_TABLE_HPP

#include "common/defs.hpp"
#include "common/format.hpp"
#include "common/hash.hpp"
#include "common/memory/arena.hpp"

#include <absl/container/flat_hash_map.h>

#include <optional>
#include <string_view>
#include <type_traits>

namespace tiro {

class InternedString;
class StringTable;

/// Stores interned string instances. Strings can be looked up by content
/// and by index. Only one string copy is stored for every distinct string.
///
/// Interned strings are represented as simple integers (intally: indices into the string table)
/// which makes comparison of interned strings extremely fast.
class StringTable final {
public:
    StringTable();
    ~StringTable();

    StringTable(StringTable&&) noexcept = default;

    StringTable(const StringTable&) = delete;
    StringTable& operator=(const StringTable&) = delete;

    /// Returns an interned string index that points to a copy of the given string.
    /// Entries are created as necessary.
    InternedString insert(std::string_view str);

    /// Returns an interned string index for the given input string if it
    /// exists in the table.
    std::optional<InternedString> find(std::string_view str) const;

    /// Returns the string value for the given string index.
    /// Throws if the string is invalid.
    std::string_view value(const InternedString& str) const;

    /// Returns the string value for the given interned string, or the provided
    /// default string if the string is invalid.
    std::string_view value_or(const InternedString& str, std::string_view def) const;

    /// Returns the string value for the given interned string, or the empty string
    /// if the string is invalid.
    std::string_view value_or_empty(const InternedString& str) const { return value_or(str, ""); }

    /// Returns a simple string representation for the given string.
    /// Returns a placeholder string if the string is invalid.
    std::string_view dump(const InternedString& str) const;

    /// Number of strings in the table.
    size_t size() const { return strings_by_index_.size(); }

    /// Total number of bytes used by all string instances in this table.
    size_t byte_size() const { return total_bytes_; }

private:
    struct Storage {
        size_t size;
        char data[];
    };

    using strings_by_index_t = absl::flat_hash_map<u32, Storage*>;
    using strings_by_content_t = absl::flat_hash_map<std::string_view, u32>;

private:
    std::string_view view(const Storage* str) const noexcept {
        TIRO_DEBUG_NOT_NULL(str);
        return {str->data, str->size};
    }

private:
    Arena arena_;
    strings_by_index_t strings_by_index_;
    strings_by_content_t strings_by_content_;
    u32 next_index_ = 1;
    size_t total_bytes_ = 0;
};

/// An interned string points into the string table. The associated string value
/// can be retrieved using string_table.value(interned_string).
///
class InternedString final {
public:
    InternedString() = default;
    explicit InternedString(u32 value)
        : value_(value) {}

    u32 value() const { return value_; }
    bool valid() const { return value_ != 0; }
    explicit operator bool() const { return value_ != 0; }

    bool operator==(const InternedString& other) const { return value_ == other.value_; }
    bool operator!=(const InternedString& other) const { return !(*this == other); }
    bool operator<(const InternedString& other) const { return value_ < other.value_; }

    void hash(Hasher& h) const { h.append(value_); }

    void format(FormatStream& stream) const;

private:
    u32 value_ = 0; // 0 -> invalid string
};

} // namespace tiro

TIRO_ENABLE_MEMBER_FORMAT(tiro::InternedString)
TIRO_ENABLE_MEMBER_HASH(tiro::InternedString)

#endif // TIRO_COMMON_TEXT_STRING_TABLE_HPP
