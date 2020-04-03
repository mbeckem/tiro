#include "tiro/bytecode_gen/locations.hpp"

#include "tiro/core/defs.hpp"
#include "tiro/core/safe_int.hpp"
#include "tiro/ir/traversal.hpp"

namespace tiro {

/* [[[cog
    import unions
    import bytecode_gen
    unions.implement_type(bytecode_gen.CompiledLocationType)
]]] */
std::string_view to_string(CompiledLocationType type) {
    switch (type) {
    case CompiledLocationType::Value:
        return "Value";
    case CompiledLocationType::Method:
        return "Method";
    }
    TIRO_UNREACHABLE("Invalid CompiledLocationType.");
}
// [[[end]]]

/* [[[cog
    import unions
    import bytecode_gen
    unions.implement_type(bytecode_gen.CompiledLocation)
]]] */
CompiledLocation CompiledLocation::make_value(const Value& value) {
    return value;
}

CompiledLocation CompiledLocation::make_method(
    const CompiledLocalID& instance, const CompiledLocalID& function) {
    return Method{instance, function};
}

CompiledLocation::CompiledLocation(const Value& value)
    : type_(CompiledLocationType::Value)
    , value_(value) {}

CompiledLocation::CompiledLocation(const Method& method)
    : type_(CompiledLocationType::Method)
    , method_(method) {}

const CompiledLocation::Value& CompiledLocation::as_value() const {
    TIRO_ASSERT(type_ == CompiledLocationType::Value,
        "Bad member access on CompiledLocation: not a Value.");
    return value_;
}

const CompiledLocation::Method& CompiledLocation::as_method() const {
    TIRO_ASSERT(type_ == CompiledLocationType::Method,
        "Bad member access on CompiledLocation: not a Method.");
    return method_;
}

// [[[end]]]

u32 physical_locals(const CompiledLocation& loc) {
    switch (loc.type()) {
    case CompiledLocationType::Value:
        return 1;
    case CompiledLocationType::Method:
        return 2;
    }
    TIRO_UNREACHABLE("Invalid location type.");
}

CompiledLocations::CompiledLocations() {}

CompiledLocations::CompiledLocations(size_t total_ssa_locals) {
    locs_.resize(total_ssa_locals);
}

bool CompiledLocations::contains(LocalID ssa_local) const {
    return locs_[ssa_local].has_value();
}

void CompiledLocations::set(LocalID ssa_local, const CompiledLocation& loc) {
    TIRO_ASSERT(ssa_local, "SSA local must be valid.");
    TIRO_ASSERT(!contains(ssa_local),
        "SSA local must not have been assigned a location.");
    locs_[ssa_local] = loc;

    struct MaxValueVisitor {
        static_assert(std::is_same_v<u32, CompiledLocalID::UnderlyingType>);

        u32 visit_value(CompiledLocalID local) {
            TIRO_ASSERT(local, "Invalid localation.");
            return local.value();
        }

        u32 visit_method(const CompiledLocation::Method& m) {
            TIRO_ASSERT(m.instance, "Invalid localation.");
            TIRO_ASSERT(m.function, "Invalid localation.");
            return std::max(m.instance.value(), m.function.value());
        }
    };

    const u32 max_local = loc.visit(MaxValueVisitor());
    physical_locals_ = std::max(physical_locals_, max_local + 1);
}

// TODO multi register locations (e.g. for member functions)
CompiledLocation CompiledLocations::get(LocalID ssa_local) const {
    TIRO_ASSERT(contains(ssa_local),
        "SSA local must have been assigned a physical location.");
    return *locs_[ssa_local];
}

CompiledLocations allocate_locations(const Function& func) {
    CompiledLocations locations(func.local_count());

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
    SafeInt<u32> location_counter = 0;
    auto next_location = [&]() {
        return CompiledLocalID((location_counter++).value());
    };

    for (const auto block_id : PreorderTraversal(func)) {
        const auto block = func[block_id];
        for (const auto& stmt : block->stmts()) {
            switch (stmt.type()) {
            case StmtType::Assign:
                break;

            case StmtType::Define: {
                const auto local_id = stmt.as_define().local;
                const auto value = func[local_id]->value();

                // Location of member's of a phi function's congruence class
                // are assigned when the phi function is encountered.
                if (phi_links.count(local_id))
                    break;

                const CompiledLocation location = [&]() {
                    if (value.type() == RValueType::MethodHandle) {
                        auto instance = next_location();
                        auto function = next_location();
                        return CompiledLocation::make_method(
                            instance, function);
                    }
                    return CompiledLocation::make_value(next_location());
                }();
                locations.set(local_id, location);

                // All members of the phi's congruence class share the same physical location.
                if (value.type() == RValueType::Phi) {
                    const auto pos = phi_members.find(local_id);
                    TIRO_ASSERT(pos != phi_members.end(),
                        "Failed to find members of phi function's congruence "
                        "class.");
                    for (auto member : pos->second) {
                        locations.set(member, location);
                    }
                }
                break;
            }
            }
        }
    }

    return locations;
}

} // namespace tiro
