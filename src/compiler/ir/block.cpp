#include "compiler/ir/block.hpp"

#include "compiler/ir/function.hpp"

namespace tiro::ir {

Block::Block(InternedString label)
    : label_(label) {
    TIRO_DEBUG_ASSERT(label, "Basic blocks must have a valid label.");
}

Block::~Block() {}

Block::Block(Block&&) noexcept = default;

Block& Block::operator=(Block&&) = default;

BlockId Block::predecessor(size_t index) const {
    TIRO_DEBUG_ASSERT(index < predecessors_.size(), "Index out of bounds.");
    return predecessors_[index];
}

size_t Block::predecessor_count() const {
    return predecessors_.size();
}

void Block::append_predecessor(BlockId predecessor) {
    predecessors_.push_back(predecessor);
}

void Block::replace_predecessor(BlockId old_pred, BlockId new_pred) {
    // TODO: Keep in mind that this will cause problems if the same source block
    // can have multiple edges to the same target. This could happen with
    // more advanced optimizations.
    auto pos = std::find(predecessors_.begin(), predecessors_.end(), old_pred);
    if (pos != predecessors_.end())
        *pos = new_pred;
}

InstId Block::inst(size_t index) const {
    TIRO_DEBUG_ASSERT(index < insts_.size(), "Index out of bounds.");
    return insts_[index];
}

size_t Block::inst_count() const {
    return insts_.size();
}

void Block::insert_inst(size_t index, InstId inst) {
    TIRO_DEBUG_ASSERT(index <= insts_.size(), "Index out of bounds.");
    insts_.insert(insts_.begin() + index, inst);
}

void Block::insert_insts(size_t index, Span<const InstId> insts) {
    TIRO_DEBUG_ASSERT(index <= insts_.size(), "Index out of bounds.");
    insts_.insert(insts_.begin() + index, insts.begin(), insts.end());
}

void Block::append_inst(InstId inst) {
    insts_.push_back(inst);
}

size_t Block::phi_count(const Function& parent) const {
    auto non_phi = std::find_if(
        insts_.begin(), insts_.end(), [&](const auto& s) { return !is_phi_define(parent, s); });
    return static_cast<size_t>(non_phi - insts_.begin());
}

void Block::remove_phi(Function& parent, InstId inst, Value&& new_value) {
    TIRO_DEBUG_ASSERT(new_value.type() != ValueType::Phi, "New value must not be a phi node.");

    const auto phi_start = insts_.begin();
    const auto phi_end = insts_.begin() + phi_count(parent);
    const auto old_pos = std::find(phi_start, phi_end, inst);
    TIRO_DEBUG_ASSERT(old_pos != phi_end, "Failed to find the definition among the phi functions.");

    parent[inst].value(std::move(new_value));
    std::rotate(old_pos, old_pos + 1, phi_end); // Move after other phis
}

void Block::format(FormatStream& stream) const {
    stream.format("Block(label: {})", label_);
}

} // namespace tiro::ir
