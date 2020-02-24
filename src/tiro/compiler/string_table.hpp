#ifndef TIRO_COMPILER_STRING_TABLE_HPP
#define TIRO_COMPILER_STRING_TABLE_HPP

#include "tiro/core/arena.hpp"
#include "tiro/core/defs.hpp"
#include "tiro/core/format_stream.hpp"
#include "tiro/core/hash.hpp"

#include <optional>
#include <string_view>
#include <type_traits>
#include <unordered_map>
#include <unordered_set>

namespace tiro::compiler {

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

    StringTable(const StringTable&) = delete;
    StringTable& operator=(const StringTable&) = delete;

    /// Returns an interned string index that points to a copy of the given string.
    /// Entries are created as necessary.
    InternedString insert(std::string_view str);

    /// Returns an interned string index for the given input string if it
    /// exists in the table.
    std::optional<InternedString> find(std::string_view str) const;

    /// Returns the string value for the given string index.
    std::string_view value(const InternedString& str) const;

    /// Number of strings in the table.
    size_t size() const { return strings_by_index_.size(); }

    /// Total number of bytes used by all string instances in this table.
    size_t byte_size() const { return total_bytes_; }

private:
    struct Storage {
        size_t size;
        char data[];
    };

    // NB boost intrusive container or multiindex would be more appropriate
    // could also just be a vector
    using strings_by_index_t = std::unordered_map<u32, Storage*>;

    // string views point to the string_t
    using strings_by_content_t = std::unordered_map<std::string_view, u32>;

private:
    std::string_view view(const Storage* str) const noexcept {
        TIRO_ASSERT_NOT_NULL(str);
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

    bool operator==(const InternedString& other) const {
        return value_ == other.value_;
    }
    bool operator!=(const InternedString& other) const {
        return !(*this == other);
    }
    bool operator<(const InternedString& other) const {
        return value_ < other.value_;
    }

    void build_hash(Hasher& h) const { h.append(value_); }

    void format(FormatStream& stream) const;

private:
    u32 value_ = 0; // 0 -> invalid string
};

} // namespace tiro::compiler

namespace std {

template<>
struct hash<tiro::compiler::InternedString> : public tiro::UseHasher {};

} // namespace std

TIRO_FORMAT_MEMBER(tiro::compiler::InternedString)

#endif // TIRO_COMPILER_STRING_TABLE_HPP
