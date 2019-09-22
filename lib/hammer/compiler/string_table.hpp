#ifndef HAMMER_COMPILER_STRING_TABLE_HPP
#define HAMMER_COMPILER_STRING_TABLE_HPP

#include "hammer/core/arena.hpp"
#include "hammer/core/defs.hpp"
#include "hammer/core/hash.hpp"

#include <string_view>
#include <type_traits>
#include <unordered_map>
#include <unordered_set>

namespace hammer {

class InternedString;
class StringTable;

/**
 * Stores interned string instances. Strings can be looked up by content
 * and by index. Only one string copy is stored for every distinct string.
 *
 * Interned strings are represented as simple integers (intally: indices into the string table)
 * which makes comparison of interned strings extremely fast.
 */
class StringTable {
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
        char str[];
    };

    // NB boost intrusive container or multiindex would be more appropriate
    // could also just be a vector
    using strings_by_index_t = std::unordered_map<u32, Storage*>;

    // string views point to the string_t
    using strings_by_content_t = std::unordered_map<std::string_view, u32>;

private:
    std::string_view view(const Storage* str) const noexcept {
        HAMMER_ASSERT_NOT_NULL(str);
        return {str->str, str->size};
    }

private:
    Arena arena_;
    strings_by_index_t strings_by_index_;
    strings_by_content_t strings_by_content_;
    u32 next_index_ = 1;
    size_t total_bytes_ = 0;
};

/**
 * An interned string points into the string table. The associated string value
 * can be retrieved using string_table.value(interned_string).
 */
class InternedString {
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

private:
    u32 value_ = 0; // 0 -> invalid string
};

} // namespace hammer

namespace std {

template<>
struct hash<hammer::InternedString> : public hammer::UseHasher {};

} // namespace std

#endif // HAMMER_COMPILER_STRING_TABLE_HPP
