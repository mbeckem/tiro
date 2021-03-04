#include "compiler/source_map.hpp"

#include "common/text/unicode.hpp"

#include <algorithm>

namespace tiro {

static size_t count_code_points(const char* begin, const char* end) {
    size_t count = 0;

    const char* next = begin;
    while (next != end) {
        [[maybe_unused]] CodePoint cp;
        std::tie(cp, next) = decode_utf8(next, end);
        ++count;
    }

    return count;
}

CursorPosition::CursorPosition(u32 line, u32 column)
    : line_(line)
    , column_(column) {
    TIRO_DEBUG_ASSERT(line_ > 0, "Invalid line.");
    TIRO_DEBUG_ASSERT(column_ > 0, "Invalid column.");
}

SourceMap::SourceMap(InternedString file_name, std::string_view source_text)
    : file_name_(file_name)
    , source_text_(source_text)
    , file_size_(source_text.size())
    , line_starts_(compute_line_starts(source_text)) {
    TIRO_DEBUG_ASSERT(file_name_.valid(), "Invalid file name.");
}

CursorPosition SourceMap::cursor_pos(const SourceReference& ref) const {
    TIRO_DEBUG_ASSERT(ref.end() <= file_size_, "Source reference is out of bounds.");

    // Find the start of the current line.
    const auto line_start_pos = [&] {
        // First one greater than ref.begin()
        auto pos = std::upper_bound(line_starts_.begin(), line_starts_.end(), ref.begin());
        TIRO_DEBUG_ASSERT(pos != line_starts_.begin(),
            "Invariant error."); // 0 is part of the vector

        // Last one <= ref.begin()
        return pos - 1;
    }();

    // 0-based index of the current line within the source text.
    const size_t line_index = static_cast<size_t>(line_start_pos - line_starts_.begin());

    // 0-based byte offset of the start of the current line within the source text.
    const size_t line_start_offset = *line_start_pos;
    TIRO_DEBUG_ASSERT(line_start_offset <= ref.begin(),
        "Start of the line must preceed the start of the source ref.");

    // Count the number of code points for the column value.
    // This is not 100% correct (complex glyphs can consist of multiple unicode code points),
    // but it will be fine for now.
    const size_t code_points = count_code_points(
        source_text_.data() + line_start_offset, source_text_.data() + ref.begin());

    return CursorPosition(static_cast<u32>(line_index + 1), static_cast<u32>(code_points + 1));
}

std::vector<size_t> SourceMap::compute_line_starts(std::string_view source_text) {
    std::vector<size_t> line_starts{0};

    const size_t size = source_text.size();
    for (size_t i = 0; i < size; ++i) {
        if (source_text[i] == '\n')
            line_starts.push_back(i + 1);
    }

    return line_starts;
}

} // namespace tiro
