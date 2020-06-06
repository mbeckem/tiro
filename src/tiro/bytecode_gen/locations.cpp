#include "tiro/bytecode_gen/locations.hpp"

#include <algorithm>

namespace tiro {

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

BytecodeLocations::BytecodeLocations(size_t total_blocks, size_t total_ssa_locals) {
    copies_.resize(total_blocks);
    locs_.resize(total_ssa_locals);
}

bool BytecodeLocations::contains(LocalId ssa_local) const {
    return locs_[ssa_local].has_value();
}

void BytecodeLocations::set(LocalId ssa_local, const BytecodeLocation& loc) {
    TIRO_DEBUG_ASSERT(ssa_local, "SSA local must be valid.");
    locs_[ssa_local] = loc;
}

BytecodeLocation BytecodeLocations::get(LocalId ssa_local) const {
    TIRO_DEBUG_ASSERT(
        contains(ssa_local), "SSA local must have been assigned a physical location.");
    return *locs_[ssa_local];
}

std::optional<BytecodeLocation> BytecodeLocations::try_get(LocalId ssa_local) const {
    if (locs_.in_bounds(ssa_local))
        return locs_[ssa_local];
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

static u32 aggregate_size(AggregateType type) {
    switch (type) {
    case AggregateType::Method:
        return 2;
    }

    TIRO_UNREACHABLE("Invalid aggregate type.");
}

static u32 aggregate_member_size(AggregateMember member) {
    switch (member) {
    case AggregateMember::MethodInstance:
    case AggregateMember::MethodFunction:
        return 1;
    }

    TIRO_UNREACHABLE("Invalid aggregate type.");
}

u32 allocated_register_size(LocalId local_id, const Function& func) {
    auto& rvalue = func[local_id]->value();
    switch (rvalue.type()) {
    case RValueType::Aggregate:
        return aggregate_size(rvalue.as_aggregate().type());
    case RValueType::GetAggregateMember:
        return 0;
    case RValueType::Phi: {
        auto phi = func[rvalue.as_phi().value];
        if (phi->operand_count() == 0)
            return 0;

        // Phi arguments must be realized.
        u32 regs = realized_register_size(phi->operand(0), func);
        for (size_t i = 1, n = phi->operand_count(); i < n; ++i) {
            u32 other_regs = realized_register_size(phi->operand(1), func);
            TIRO_CHECK(
                regs == other_regs, "All phi operands must have the same register requirements.");
        }
        return regs;
    }
    default:
        return 1;
    }
}

u32 realized_register_size(LocalId local_id, const Function& func) {
    auto& rvalue = func[local_id]->value();
    if (rvalue.type() == RValueType::GetAggregateMember) {
        auto& get_member = rvalue.as_get_aggregate_member();
        return aggregate_member_size(get_member.member);
    }
    return allocated_register_size(local_id, func);
}

BytecodeLocation get_aggregate_member(LocalId aggregate_id, AggregateMember member,
    const BytecodeLocations& locs, const Function& func) {

    const auto& aggregate = func[aggregate_id]->value().as_aggregate();
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
        }

        TIRO_UNREACHABLE("Invalid aggregate member.");
    }();
    TIRO_DEBUG_ASSERT(member_loc.size() == aggregate_member_size(member),
        "Member location is inconsistent with member size.");
    return member_loc;
}

BytecodeLocation
storage_location(LocalId local_id, const BytecodeLocations& locs, const Function& func) {
    auto& rvalue = func[local_id]->value();

    // Aggregate members are implemented as storage aliases.
    if (rvalue.type() == RValueType::GetAggregateMember) {
        auto& get_member = rvalue.as_get_aggregate_member();
        return get_aggregate_member(get_member.aggregate, get_member.member, locs, func);
    }

    return locs.get(local_id);
}

} // namespace tiro
