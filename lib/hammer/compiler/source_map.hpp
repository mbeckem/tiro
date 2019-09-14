#ifndef HAMMER_COMPILER_SOURCE_MAP_HPP
#define HAMMER_COMPILER_SOURCE_MAP_HPP

#include "hammer/compiler/source_reference.hpp"

namespace hammer {

class CursorPosition {
public:
    // Constructs an invalid instance.
    CursorPosition() = default;

    // Constructs a valid instance (line and column > 0).
    CursorPosition(u32 line, u32 column);

    // 1 based
    u32 line() const { return line_; }

    // 1 based
    u32 column() const { return column_; }

    explicit operator bool() const { return line_ != 0; }

private:
    u32 line_ = 0;
    u32 column_ = 0;
};

class SourceMap {
public:
    SourceMap(InternedString file_name, std::string_view source_text);

    // Computes the cursor position for the given source reference.
    CursorPosition cursor_pos(const SourceReference& ref) const;

private:
    static std::vector<size_t>
    compute_line_starts(std::string_view source_text);

private:
    InternedString file_name_;
    size_t file_size_ = 0;

    // Contains the indices of newlines within the source string,
    // in ascending order.
    std::vector<size_t> line_starts_;
};

} // namespace hammer

#endif // HAMMER_COMPILER_SOURCE_MAP_HPP
