#include "compiler/ir/record.hpp"

namespace tiro::ir {

Record::Record() {}

Record::~Record() {}

void Record::insert(InternedString name, InstId value) {
    TIRO_DEBUG_ASSERT(name, "Invalid name.");
    TIRO_DEBUG_ASSERT(value, "Invalid value.");
    entries_.emplace_back(name, value);
}

} // namespace tiro::ir
