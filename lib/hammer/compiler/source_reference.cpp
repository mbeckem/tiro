#include "hammer/compiler/source_reference.hpp"

#include "hammer/core/error.hpp"

#include <limits>

namespace hammer {

SourceReference SourceReference::from_std_offsets(InternedString file_name, size_t begin,
                                                    size_t end) {
    HAMMER_CHECK(begin <= std::numeric_limits<u32>::max(), "Index too large for 32 bit.");
    HAMMER_CHECK(end <= std::numeric_limits<u32>::max(), "Index too large for 32 bit.");
    return SourceReference(file_name, static_cast<u32>(begin), static_cast<u32>(end));
}

SourceReference::SourceReference(InternedString file_name, u32 begin, u32 end)
    : file_name_(file_name)
    , begin_(begin)
    , end_(end) {
    HAMMER_CHECK(file_name, "Invalid file name.");
    HAMMER_CHECK(begin <= end, "Invalid range: 'begin' must be <='end'.");
}

} // namespace hammer
