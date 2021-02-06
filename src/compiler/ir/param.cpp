#include "compiler/ir/param.hpp"

namespace tiro::ir {

Param::Param(InternedString name)
    : name_(name) {
    TIRO_DEBUG_ASSERT(name, "Parameters must have valid names.");
}

InternedString Param::name() const {
    return name_;
}

void Param::format(FormatStream& stream) const {
    stream.format("Param({})", name());
}

} // namespace tiro::ir
