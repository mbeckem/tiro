#include "tiro/bytecode_gen/locations.hpp"

#include "tiro/core/defs.hpp"
#include "tiro/core/safe_int.hpp"
#include "tiro/mir/traversal.hpp"

namespace tiro {

CompiledLocations::CompiledLocations() {}

CompiledLocations::CompiledLocations(size_t total_ssa_locals) {
    locs_.resize(total_ssa_locals);
}

bool CompiledLocations::contains(LocalID ssa_local) const {
    return locs_[ssa_local].valid();
}

void CompiledLocations::set(LocalID ssa_local, CompiledLocalID location) {
    TIRO_ASSERT(ssa_local, "SSA local must be valid.");
    TIRO_ASSERT(location, "Compiled location must be valid.");
    TIRO_ASSERT(!contains(ssa_local),
        "SSA local must not have been assigned a location.");
    locs_[ssa_local] = location;
    physical_locals_ = std::max(physical_locals_, location.value() + 1);
}

// TODO multi register locations (e.g. for member functions)
CompiledLocalID CompiledLocations::get(LocalID ssa_local) const {
    auto v = locs_[ssa_local];
    TIRO_ASSERT(
        v.valid(), "SSA local must have been assigned a physical location.");
    return v;
}

CompiledLocations allocate_locations(const Function& func) {
    CompiledLocations locs(func.local_count());

    // Find locals that are either used as arguments to a phi function
    // or reference the phi definition directly.
    // These are the only members of the phi function's congruence class;
    // ensured created by the naive cssa construction algorithm.
    //
    // Note: tracking defines and usages would simplify this algorithm...
    // TODO: Efficient containers
    // Key: Phi function id, Values: congruence class members
    std::unordered_map<LocalID, std::vector<LocalID>, UseHasher> phi_members;

    // Key: congruence class member, value: phi function id.
    std::unordered_map<LocalID, LocalID, UseHasher> phi_links;

    for (const auto block_id : PreorderTraversal(func)) {
        const auto block = func[block_id];
        for (const auto& stmt : block->stmts()) {
            if (stmt.type() != StmtType::Define)
                continue;

            const auto local_id = stmt.as_define().local;
            const auto& value = func[local_id]->value();

            if (value.type() == RValueType::Phi) {
                const auto phi = func[value.as_phi().value];
                auto& members = phi_members[local_id];
                for (const auto& op : phi->operands()) {
                    members.push_back(op);
                    phi_links[op] = local_id;
                }
                continue;
            }

            if (value.type() == RValueType::UseLocal) {
                const auto target_id = value.as_use_local().target;
                const auto target = func[target_id];
                if (target->value().type() == RValueType::Phi) {
                    phi_members[target_id].push_back(local_id);
                    phi_links[local_id] = target_id;
                }
                continue;
            }
        }
    }

    // Walk through the cfg and assign a new location to each local.
    // This is very inefficient, no inference / liveness analysis is done at the moment.
    SafeInt<u32> next_slot = 0;
    for (const auto block_id : PreorderTraversal(func)) {
        const auto block = func[block_id];
        for (const auto& stmt : block->stmts()) {
            switch (stmt.type()) {
            case StmtType::Assign:
                break;

            case StmtType::Define: {
                const auto local_id = stmt.as_define().local;

                // Location of member's of a phi function's congruence class
                // are assigned when the phi function is encountered.
                if (phi_links.count(local_id))
                    break;

                const CompiledLocalID location((next_slot++).value());

                locs.set(local_id, location);
                const auto value = func[local_id]->value();

                // All members of the phi's congruence class share the same physical location.
                if (value.type() == RValueType::Phi) {
                    const auto pos = phi_members.find(local_id);
                    TIRO_ASSERT(pos != phi_members.end(),
                        "Failed to find members of phi function's congruence "
                        "class.");
                    for (auto member : pos->second) {
                        locs.set(member, location);
                    }
                }
                break;
            }
            }
        }
    }

    return locs;
}

} // namespace tiro
