#ifndef TIRO_COMPILER_BYTECODE_GEN_LOCATIONS_HPP
#define TIRO_COMPILER_BYTECODE_GEN_LOCATIONS_HPP

#include "compiler/bytecode_gen/fwd.hpp"

#include "bytecode/instruction.hpp"
#include "common/index_map.hpp"
#include "compiler/ir/function.hpp"

#include <optional>

namespace tiro {

/// Represents a group of registers that have been assigned to a value.
class BytecodeLocation final {
public:
    using iterator = const BytecodeRegister*;
    using const_iterator = iterator;

    // Max number of registers in a single location object.
    static constexpr u32 max_registers = 2;

    static constexpr u32 max_size() { return max_registers; }

    /// Constructs an empty bytecode location.
    BytecodeLocation();

    /// Constructs a bytecode location with a single register.
    /// \pre `reg` must be valid.
    BytecodeLocation(BytecodeRegister reg);

    /// Constructs a bytecode location from a span of registers.
    /// \pre `regs.size()` < BytecodeLocation::max_size()`.
    /// \pre All registers in `regs` must be valid.
    BytecodeLocation(Span<const BytecodeRegister> regs);

    const_iterator begin() const { return regs_; }
    const_iterator end() const { return regs_ + size(); }

    bool empty() const;
    u32 size() const;
    BytecodeRegister operator[](u32 index) const;
    BytecodeRegister get(u32 index) const { return (*this)[index]; }

    void format(FormatStream& stream) const;

private:
    BytecodeRegister regs_[max_registers];
};

bool operator==(const BytecodeLocation& lhs, const BytecodeLocation& rhs);
bool operator!=(const BytecodeLocation& lhs, const BytecodeLocation& rhs);

/// Represents a copy between two registers. Typically used for the implementation
/// of phi operand passing.
struct RegisterCopy {
    BytecodeRegister src;
    BytecodeRegister dest;
};

/// Maps virtual locals (from the ir layer) to physical locals (at the bytecode layer).
class BytecodeLocations final {
public:
    BytecodeLocations();
    explicit BytecodeLocations(size_t total_blocks, size_t total_ssa_locals);

    BytecodeLocations(BytecodeLocations&&) noexcept = default;
    BytecodeLocations& operator=(BytecodeLocations&&) noexcept = default;

    /// Returns the required number of physical local variable slots.
    u32 total_registers() const { return total_registers_; }

    /// Sets the required number of physical local variable slots.
    void total_registers(u32 total) { total_registers_ = total; }

    /// Returns true if the given ssa_local was assigned a physical location.
    bool contains(LocalId ssa_local) const;

    /// Assigns the physical location to the given ssa_local.
    void set(LocalId ssa_local, const BytecodeLocation& location);

    /// Returns the physical location of the given ssa_local.
    /// \pre ssa_local must have been assigned a location.
    BytecodeLocation get(LocalId ssa_local) const;

    /// Returns the physical location of the given ssa local, or an empty
    /// optional if the ssa local has not been assigned a location.
    std::optional<BytecodeLocation> try_get(LocalId ssa_local) const;

    /// Returns true if the block was a sequence of phi argument copies.
    bool has_phi_copies(BlockId block) const;

    /// Assigns the given phi argument copies to the given block.
    void set_phi_copies(BlockId block, std::vector<RegisterCopy> copies);

    /// Returns the phi argument copies for the given block.
    const std::vector<RegisterCopy>& get_phi_copies(BlockId block) const;

private:
    // Storage locations of ssa locals.
    IndexMap<std::optional<BytecodeLocation>, IdMapper<LocalId>> locs_;

    // Spare storage locations for the passing of phi arguments. Only assigned
    // to blocks that pass phi arguments to successors.
    IndexMap<std::vector<RegisterCopy>, IdMapper<BlockId>> copies_;

    // Total number of storage locations used.
    u32 total_registers_ = 0;
};

/// Returns the static size of the given aggregate type, in registers.
u32 aggregate_size(AggregateType type);

/// Returns the static size of the given aggregate member, in registers.
u32 aggregate_member_size(AggregateMember member);

/// Returns the actual location of the the given aggregate member.
BytecodeLocation get_aggregate_member(LocalId aggregate_id, AggregateMember member,
    const BytecodeLocations& locs, const Function& func);

/// Returns the actual storage registers used by the given local.
/// Automatically follows aliases like aggregate member references.
BytecodeLocation
storage_location(LocalId local_id, const BytecodeLocations& locs, const Function& func);

} // namespace tiro

TIRO_ENABLE_MEMBER_FORMAT(tiro::BytecodeLocation)

#endif // TIRO_COMPILER_BYTECODE_GEN_LOCATIONS_HPP
