#include "compiler/source_range.hpp"

#include "common/assert.hpp"

#include <limits>

namespace tiro {

SourceRange SourceRange::from_std_offsets(size_t begin, size_t end) {
    TIRO_CHECK(begin <= std::numeric_limits<u32>::max(), "Index too large for 32 bit.");
    TIRO_CHECK(end <= std::numeric_limits<u32>::max(), "Index too large for 32 bit.");
    return SourceRange(static_cast<u32>(begin), static_cast<u32>(end));
}

SourceRange SourceRange::from_std_offset(size_t offset) {
    TIRO_CHECK(offset <= std::numeric_limits<u32>::max(), "Index too large for 32 bit.");
    return from_offset(static_cast<u32>(offset));
}

SourceRange SourceRange::from_offset(u32 offset) {
    return SourceRange(offset, offset);
}

SourceRange::SourceRange(u32 begin, u32 end)
    : begin_(begin)
    , end_(end) {
    TIRO_CHECK(begin <= end, "Invalid range: 'begin' must be <= 'end'.");
}

void SourceRange::format(FormatStream& stream) const {
    if (empty()) {
        stream.format("[{}, empty]", begin());
    }

    stream.format("[{}, {}]", begin(), end());
}

std::string_view substring(std::string_view file, const SourceRange& ref) {
    TIRO_CHECK(ref.end() <= file.size(),
        "Source file range is out of bounds for the given source content.");
    return file.substr(ref.begin(), ref.end() - ref.begin());
}

} // namespace tiro
