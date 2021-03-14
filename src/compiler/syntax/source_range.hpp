#ifndef TIRO_COMPILER_SYNTAX_SOURCE_RANGE_HPP
#define TIRO_COMPILER_SYNTAX_SOURCE_RANGE_HPP

#include "common/defs.hpp"
#include "common/format.hpp"

namespace tiro::next {

/// References a contiguous slice of the source text.
class SourceRange final {
public:
    /// Constructs a source range from the given [begin, end) interval.
    /// Verifies that the indices fit into 32 bits.
    static SourceRange from_std_offsets(size_t begin, size_t end);

    /// Constructs an empty source range at the given position.
    static SourceRange from_std_offset(size_t offset);

    /// Constructs an empty source range at the given position.
    static SourceRange from_offset(u32 offset);

    /// Constructs an invalid instance
    SourceRange() = default;

    /// Constructs a valid source reference.
    SourceRange(u32 begin, u32 end);

    /// Start of the referenced source code, inclusive.
    u32 begin() const { return begin_; }

    /// End of the referenced source code, exclusive.
    u32 end() const { return end_; }

    /// True if this range has length 0.
    bool empty() const { return begin_ == end_; }

    /// Number of bytes in this range.
    size_t size() const { return end_ - begin_; }

    void format(FormatStream& stream) const;

private:
    // Byte offsets into the input string. Half open [begin, end).
    u32 begin_ = 0;
    u32 end_ = 0;
};

std::string_view substring(std::string_view file, const SourceRange& range);

inline bool operator==(const SourceRange& lhs, const SourceRange& rhs) {
    return lhs.begin() == rhs.begin() && lhs.end() == rhs.end();
}

inline bool operator!=(const SourceRange& lhs, const SourceRange& rhs) {
    return !(lhs == rhs);
}

} // namespace tiro::next

TIRO_ENABLE_MEMBER_FORMAT(tiro::next::SourceRange);

#endif // TIRO_COMPILER_SYNTAX_SOURCE_RANGE_HPP
