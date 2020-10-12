#ifndef TIRO_COMPILER_SOURCE_REFERENCE_HPP
#define TIRO_COMPILER_SOURCE_REFERENCE_HPP

#include "common/defs.hpp"
#include "common/format.hpp"
#include "common/text/string_table.hpp"

namespace tiro {

/// References a substring of the source code.
class SourceReference final {
public:
    /// Constructs a source reference from the given [begin, end) interval.
    /// Verifies that the indices fit into 32 bits.
    static SourceReference from_std_offsets(InternedString file_name, size_t begin, size_t end);

    /// Constructs an invalid instance
    SourceReference() = default;

    /// Constructs a valid source reference.
    SourceReference(InternedString file_name, u32 begin, u32 end);

    /// File that contains the source text interval.
    InternedString file_name() const { return file_name_; }

    /// Start of the referenced source code, inclusive.
    u32 begin() const { return begin_; }

    /// End of the referenced source code, exclusive.
    u32 end() const { return end_; }

    /// True if this reference is valid.
    bool valid() const { return file_name_.valid(); }
    explicit operator bool() const { return valid(); }

    void format(FormatStream& stream) const;

private:
    /* Source file name, points into the parser's string table. */
    InternedString file_name_;

    /* Byte offsets into the input string. Half open [begin, end) */
    u32 begin_ = 0;
    u32 end_ = 0;
};

std::string_view substring(std::string_view file, const SourceReference& ref);

} // namespace tiro

TIRO_ENABLE_MEMBER_FORMAT(tiro::SourceReference);

#endif // TIRO_COMPILER_SOURCE_REFERENCE_HPP
