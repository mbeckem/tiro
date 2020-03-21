#include "tiro/bytecode_gen/fwd.hpp"

#include "tiro/bytecode/instruction.hpp"
#include "tiro/core/index_map.hpp"
#include "tiro/mir/types.hpp"

namespace tiro {

class CompiledLocations final {
public:
    CompiledLocations();
    explicit CompiledLocations(size_t total_ssa_locals);

    CompiledLocations(CompiledLocations&&) noexcept = default;
    CompiledLocations& operator=(CompiledLocations&&) noexcept = default;

    /// Returns the required number of physical local variable slots.
    u32 physical_locals() const { return physical_locals_; }

    /// Returns true if the given ssa_local was assigned a physical location.
    bool contains(LocalID ssa_local) const;

    /// Assigns the physical location to the given ssa_local.
    /// \pre ssa_local must not have been assigned a location yet.
    void set(LocalID ssa_local, CompiledLocalID location);

    /// Returns the physical location of the given ssa_local.
    /// \pre ssa_local must have been assigned a location.
    // TODO multi register locations (e.g. for member functions)
    CompiledLocalID get(LocalID ssa_local) const;

private:
    IndexMap<CompiledLocalID, IDMapper<LocalID>> locs_;
    u32 physical_locals_ = 0;
};

/// Assigns a physical location to ssa locals in the given function.
/// Used when compiling a function from MIR to bytecode.
/// Exposed for testing.
CompiledLocations allocate_locations(const Function& func);

} // namespace tiro
