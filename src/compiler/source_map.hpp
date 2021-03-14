#ifndef TIRO_COMPILER_SOURCE_MAP_HPP
#define TIRO_COMPILER_SOURCE_MAP_HPP

#include "common/text/string_table.hpp"
#include "compiler/source_range.hpp"

namespace tiro {

/// Represents the position of a cursor (line and column) in a source text.
/// Note that the line and column numbers refer to unicode code points.
class CursorPosition final {
public:
    /// Constructs an invalid instance.
    CursorPosition() = default;

    /// Constructs a valid instance (line and column > 0).
    CursorPosition(u32 line, u32 column);

    /// 1 based
    u32 line() const { return line_; }

    /// 1 based
    u32 column() const { return column_; }

    /// True iff valid.
    explicit operator bool() const { return line_ != 0; }

private:
    u32 line_ = 0;
    u32 column_ = 0;
};

class SourceMap final {
public:
    // Note: source_text is stored by reference!
    explicit SourceMap(InternedString file_name, std::string_view source_text);

    // Computes the cursor position for the given source range.
    CursorPosition cursor_pos(const SourceRange& range) const;

private:
    static std::vector<size_t> compute_line_starts(std::string_view source_text);

private:
    InternedString file_name_;
    std::string_view source_text_;
    size_t file_size_ = 0;

    // Contains the indices of newlines within the source string,
    // in ascending order.
    std::vector<size_t> line_starts_;
};

} // namespace tiro

#endif // TIRO_COMPILER_SOURCE_MAP_HPP
