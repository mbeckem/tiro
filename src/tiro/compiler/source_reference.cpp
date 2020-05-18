#include "tiro/compiler/source_reference.hpp"

#include <limits>

namespace tiro {

SourceReference SourceReference::from_std_offsets(
    InternedString file_name, size_t begin, size_t end) {
    TIRO_CHECK(begin <= std::numeric_limits<u32>::max(),
        "Index too large for 32 bit.");
    TIRO_CHECK(
        end <= std::numeric_limits<u32>::max(), "Index too large for 32 bit.");
    return SourceReference(
        file_name, static_cast<u32>(begin), static_cast<u32>(end));
}

SourceReference::SourceReference(InternedString file_name, u32 begin, u32 end)
    : file_name_(file_name)
    , begin_(begin)
    , end_(end) {
    TIRO_CHECK(file_name, "Invalid file name.");
    TIRO_CHECK(begin <= end, "Invalid range: 'begin' must be < 'end'.");
}

void SourceReference::format(FormatStream& stream) const {
    if (!valid()) {
        stream.format("<invalid>");
    }

    stream.format("(start: {}, end: {})", begin(), end());
}

} // namespace tiro
