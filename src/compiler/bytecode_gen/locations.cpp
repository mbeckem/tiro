#include "compiler/bytecode_gen/locations.hpp"

#include <algorithm>

namespace tiro {

using ir::Function;
using ir::InstId;
using ir::BlockId;
using ir::AggregateMember;
using ir::AggregateType;
using ir::ValueType;

BytecodeLocation::BytecodeLocation() {
    std::fill(regs_, regs_ + max_registers, BytecodeRegister());
}

BytecodeLocation::BytecodeLocation(BytecodeRegister reg) {
    TIRO_DEBUG_ASSERT(reg.valid(), "Register must be valid.");
    regs_[0] = reg;
    std::fill(regs_ + 1, regs_ + max_registers, BytecodeRegister());
}

BytecodeLocation::BytecodeLocation(Span<const BytecodeRegister> regs) {
    TIRO_DEBUG_ASSERT(regs.size() <= max_registers, "Too many registers.");
    auto pos = std::copy(regs.begin(), regs.end(), regs_);
    std::fill(pos, regs_ + max_registers, BytecodeRegister());
}

bool BytecodeLocation::empty() const {
    return !regs_[0].valid();
}

u32 BytecodeLocation::size() const {
    auto pos = std::find(regs_, regs_ + max_registers, BytecodeRegister());
    return static_cast<u32>(pos - regs_);
}

BytecodeRegister BytecodeLocation::operator[](u32 index) const {
    TIRO_DEBUG_ASSERT(index < size(), "Index out of bounds.");
    return regs_[index];
}

void BytecodeLocation::format(FormatStream& stream) const {
    stream.format("BytecodeLocation(");
    for (u32 i = 0, n = size(); i < n; ++i) {
        if (i > 0)
            stream.format(", ");
        stream.format("{}", get(i));
    }
    stream.format(")");
}

bool operator==(const BytecodeLocation& lhs, const BytecodeLocation& rhs) {
    return std::equal(lhs.begin(), lhs.end(), rhs.begin(), rhs.end());
}

bool operator!=(const BytecodeLocation& lhs, const BytecodeLocation& rhs) {
    return !(lhs == rhs);
}

BytecodeLocations::BytecodeLocations() {}

BytecodeLocations::BytecodeLocations(size_t total_blocks, size_t total_insts) {
    copies_.resize(total_blocks);
    locs_.resize(total_insts);
}

bool BytecodeLocations::contains(InstId inst_id) const {
    return locs_[inst_id].has_value();
}

void BytecodeLocations::set(InstId inst_id, const BytecodeLocation& loc) {
    TIRO_DEBUG_ASSERT(inst_id, "Instruction id must be valid.");
    locs_[inst_id] = loc;
}

BytecodeLocation BytecodeLocations::get(InstId inst_id) const {
    TIRO_DEBUG_ASSERT(
        contains(inst_id), "Instruction must have been assigned a physical location.");
    return *locs_[inst_id];
}

std::optional<BytecodeLocation> BytecodeLocations::try_get(InstId inst_id) const {
    if (locs_.in_bounds(inst_id))
        return locs_[inst_id];
    return {};
}

bool BytecodeLocations::has_phi_copies(BlockId block) const {
    return copies_.in_bounds(block) && !copies_[block].empty();
}

void BytecodeLocations::set_phi_copies(BlockId block, std::vector<RegisterCopy> copies) {
    TIRO_DEBUG_ASSERT(block, "Block must be valid.");
    copies_[block] = std::move(copies);
}

const std::vector<RegisterCopy>& BytecodeLocations::get_phi_copies(BlockId block) const {
    TIRO_DEBUG_ASSERT(block, "Block must be valid.");
    return copies_[block];
}

bool BytecodeLocations::has_preallocated_location(SymbolId symbol) const {
    TIRO_DEBUG_ASSERT(symbol, "Symbol must be valid.");
    return preallocated_.find(symbol) != preallocated_.end();
}

void BytecodeLocations::set_preallocated_location(
    SymbolId symbol, const BytecodeLocation& location) {
    TIRO_DEBUG_ASSERT(symbol, "Symbol must be valid.");
    preallocated_[symbol] = location;
}

BytecodeLocation BytecodeLocations::get_preallocated_location(SymbolId symbol) const {
    TIRO_DEBUG_ASSERT(symbol, "Symbol must be valid.");

    auto it = preallocated_.find(symbol);
    TIRO_DEBUG_ASSERT(it != preallocated_.end(), "No preallocated location for that symbol.");
    return it->second;
}

u32 aggregate_size(AggregateType type) {
    switch (type) {
    case AggregateType::Method:
        return 2;

    case AggregateType::IteratorNext:
        return 2;
    }

    TIRO_UNREACHABLE("Invalid aggregate type.");
}

u32 aggregate_member_size(AggregateMember member) {
    switch (member) {
    case AggregateMember::MethodInstance:
    case AggregateMember::MethodFunction:
    case AggregateMember::IteratorNextValid:
    case AggregateMember::IteratorNextValue:
        return 1;
    }

    TIRO_UNREACHABLE("Invalid aggregate type.");
}

BytecodeLocation get_aggregate_member(InstId aggregate_id, AggregateMember member,
    const BytecodeLocations& locs, const Function& func) {

    [[maybe_unused]] const auto& aggregate = func[aggregate_id]->value().as_aggregate();
    TIRO_DEBUG_ASSERT(
        aggregate.type() == aggregate_type(member), "Type mismatch in aggregate access.");

    auto aggregate_loc = locs.get(aggregate_id);
    TIRO_DEBUG_ASSERT(aggregate_loc.size() == aggregate_size(aggregate.type()),
        "Aggregate location has invalid size.");

    auto member_loc = [&]() -> BytecodeLocation {
        switch (member) {
        case AggregateMember::MethodInstance:
            return aggregate_loc[0];
        case AggregateMember::MethodFunction:
            return aggregate_loc[1];

        case AggregateMember::IteratorNextValid:
            return aggregate_loc[0];
        case AggregateMember::IteratorNextValue:
            return aggregate_loc[1];
        }

        TIRO_UNREACHABLE("Invalid aggregate member.");
    }();
    TIRO_DEBUG_ASSERT(member_loc.size() == aggregate_member_size(member),
        "Member location is inconsistent with member size.");
    return member_loc;
}

BytecodeLocation
storage_location(InstId inst_id, const BytecodeLocations& locs, const Function& func) {
    auto& value = func[inst_id]->value();

    // Aggregate members are implemented as storage aliases.
    if (value.type() == ValueType::GetAggregateMember) {
        auto& get_member = value.as_get_aggregate_member();
        return get_aggregate_member(get_member.aggregate, get_member.member, locs, func);
    }

    return locs.get(inst_id);
}

} // namespace tiro
