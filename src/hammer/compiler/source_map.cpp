#include "hammer/compiler/source_map.hpp"

#include <algorithm>

namespace hammer {

CursorPosition::CursorPosition(u32 line, u32 column)
    : line_(line)
    , column_(column) {
    HAMMER_ASSERT(line_ > 0, "Invalid line.");
    HAMMER_ASSERT(column_ > 0, "Invalid column.");
}

SourceMap::SourceMap(InternedString file_name, std::string_view source_text)
    : file_name_(file_name)
    , file_size_(source_text.size())
    , line_starts_(compute_line_starts(source_text)) {
    HAMMER_ASSERT(file_name_.valid(), "Invalid file name.");
}

CursorPosition SourceMap::cursor_pos(const SourceReference& ref) const {
    if (!ref)
        return {};

    HAMMER_ASSERT(ref.file_name() == file_name_,
        "Source reference belongs to a different file.");
    HAMMER_ASSERT(
        ref.end() <= file_size_, "Source reference is out of bounds.");

    // Find the start of the current line.
    const auto line_start_pos = [&] {
        // First one greater than ref.begin()
        auto pos = std::upper_bound(
            line_starts_.begin(), line_starts_.end(), ref.begin());
        HAMMER_ASSERT(pos != line_starts_.begin(),
            "Invariant error."); // 0 is part of the vector

        // Last one <= ref.begin()
        return pos - 1;
    }();

    // 0-based index of the current line.
    const size_t line_index = static_cast<size_t>(
        line_start_pos - line_starts_.begin());

    // 0-based byte offset of that start of the current line.
    const size_t line_start_offset = *line_start_pos;
    HAMMER_ASSERT(line_start_offset <= ref.begin(),
        "Start of the line must preceed the start of the source ref.");

    return CursorPosition(
        line_index + 1, (ref.begin() - line_start_offset) + 1);
}

std::vector<size_t>
SourceMap::compute_line_starts(std::string_view source_text) {
    std::vector<size_t> line_starts{0};

    const size_t size = source_text.size();
    for (size_t i = 0; i < size; ++i) {
        if (source_text[i] == '\n')
            line_starts.push_back(i + 1);
    }

    return line_starts;
}

} // namespace hammer
