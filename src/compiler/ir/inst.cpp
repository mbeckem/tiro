#include "compiler/ir/inst.hpp"

namespace tiro::ir {

Inst::Inst(Value value)
    : value_(std::move(value)) {}

void Inst::format(FormatStream& stream) const {
    stream.format("Inst(name: {}, value: {})", name(), value());
}

} // namespace tiro::ir
