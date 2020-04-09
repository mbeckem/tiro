#include "tiro/bytecode_gen/locations.hpp"

#include "tiro/core/defs.hpp"
#include "tiro/core/safe_int.hpp"
#include "tiro/ir/dominators.hpp"
#include "tiro/ir/liveness.hpp"
#include "tiro/ir/locals.hpp"
#include "tiro/ir/traversal.hpp"

#include <algorithm>
#include <set>

namespace tiro {

namespace {

// Collection of congruence classes for phi function.
// TODO: Efficient containers
struct PhiClasses {
    // Key: Phi function id, Values: congruence class members (all arguments and the target value).
    std::unordered_map<LocalID, std::vector<LocalID>, UseHasher> members;

    // Key: congruence class member, value: phi function id.
    std::unordered_map<LocalID, LocalID, UseHasher> links;

    // Associates the phi nodes with the given local.
    void add(LocalID phi, LocalID related) {
        members[phi].push_back(related);
        links[related] = phi;
    }

    // Returns true if the local is either an argument to a phi function
    // or the target of a phi function assignment.
    bool is_phi_related(LocalID local) { return related_phi(local).valid(); }

    // If the given local is related to a phi function (either as an operand or
    // as the single user of the function's value (->CSSA), then this functino
    // return the local id of that phi function.
    // Otherwise, an invalid local id is returned.
    LocalID related_phi(LocalID local) {
        auto pos = links.find(local);
        return pos != links.end() ? pos->second : LocalID();
    }
};

struct AllocationContext {
    static_assert(std::is_same_v<CompiledLocalID::UnderlyingType, u32>);

    std::vector<u32> free;

    // Number of allocated locations. The location with index `allocated`
    // is always free. Locations before that may be free if they are in `free`.
    u32 allocated = 0;

    CompiledLocalID allocate() {
        if (!free.empty()) {
            u32 loc = free.back();
            free.pop_back();
            return CompiledLocalID(loc);
        }
        return CompiledLocalID(allocated++);
    }

    void deallocate(CompiledLocalID loc) {
        TIRO_DEBUG_ASSERT(loc, "Invalid local.");

        u32 value = loc.value();
        TIRO_DEBUG_ASSERT(value < allocated, "Free'd location is too large.");
        if (value + 1 == allocated) {
            --allocated;
            return;
        }
        free.push_back(value);
    }

    void reset() {
        free.clear();
        allocated = 0;
    }
};

class RegisterAllocator final {
public:
    explicit RegisterAllocator(const Function& func);

    void run();

    CompiledLocations take_locations() { return std::move(locations_); }

private:
    void find_phi_classes();
    void available_locations(BlockID block_id, AllocationContext& ctx);
    void assign_locations(const Stmt& stmt, AllocationContext& ctx);
    void visit_children(BlockID parent);

private:
    const Function& func_;
    DominatorTree doms_;
    Liveness liveness_;
    PhiClasses classes_;
    CompiledLocations locations_;

    std::vector<BlockID> stack_;
};

} // namespace

RegisterAllocator::RegisterAllocator(const Function& func)
    : func_(func)
    , doms_(func)
    , liveness_(func)
    , locations_(func.local_count()) {}

void RegisterAllocator::run() {
    doms_.compute();
    liveness_.compute();
    find_phi_classes();

    // DFS in dominator order.
    // Walk through the cfg in the order induced by the dominator tree (depth first) and
    // perform greedy coloring for all locals encountered on the way.
    // This approach has been found to be optimal (wrt the amount of used locals) by Hack et al.
    stack_.push_back(func_.entry());

    AllocationContext ctx;
    while (!stack_.empty()) {
        const auto block_id = stack_.back();
        stack_.pop_back();

        const auto block = func_[block_id];
        available_locations(block_id, ctx);
        for (const auto& stmt : block->stmts()) {
            assign_locations(stmt, ctx);
        }

        visit_children(block_id);
    }
}

// Find locals that are either used as arguments to a phi function
// or reference the phi definition directly.
// These are the only members of the phi function's congruence class;
// ensured created by the naive cssa construction algorithm.
void RegisterAllocator::find_phi_classes() {
    for (const auto block_id : PreorderTraversal(func_)) {
        const auto block = func_[block_id];
        for (const auto& stmt : block->stmts()) {
            if (stmt.type() != StmtType::Define)
                continue;

            const auto local_id = stmt.as_define().local;
            const auto& value = func_[local_id]->value();

            if (value.type() == RValueType::Phi) {
                const auto phi = func_[value.as_phi().value];
                for (const auto& operand : phi->operands()) {
                    classes_.add(local_id, operand);
                }
                continue;
            }

            if (value.type() == RValueType::UseLocal) {
                const auto target_id = value.as_use_local().target;
                const auto target = func_[target_id];
                if (target->value().type() == RValueType::Phi) {
                    classes_.add(target_id, local_id);
                }
                continue;
            }
        }
    }
}

void RegisterAllocator::available_locations(
    BlockID block_id, AllocationContext& ctx) {
    ctx.reset();

    // TODO small vec? reuse?
    auto block = func_[block_id];
    std::vector<u32> allocated;

    // Mark live-in values as used to ensure we don't interfere with them.
    for (const auto local : liveness_.live_in_values(block_id)) {
        auto assigned = locations_.get(local);
        visit_physical_locals(assigned, [&](CompiledLocalID id) {
            TIRO_DEBUG_ASSERT(id, "Invalid assigned location.");
            allocated.push_back(id.value());
        });
    }

    // All operands to phi functions are also treated as live.
    // This is not as efficient as it could be (lots of copying to 'call' phi functions)
    // but it will do for now.
    for (size_t i = 0, n = block->phi_count(func_); i < n; ++i) {
        const auto& stmt = block->stmt(i);
        visit_uses(func_, stmt, [&](LocalID operand) {
            // Some phi operands have not been assigned a location yet (-> loops!).
            if (auto assigned = locations_.try_get(operand)) {
                visit_physical_locals(*assigned, [&](CompiledLocalID id) {
                    TIRO_DEBUG_ASSERT(id, "Invalid assigned location.");
                    allocated.push_back(id.value());
                });
            }
        });
    }

    std::sort(allocated.begin(), allocated.end());

    u32 count = 0;
    for (u32 loc : allocated) {
        // Skipped elements are free.
        for (u32 i = count; i < loc; ++i) {
            ctx.free.push_back(i);
        }
        count = loc + 1;
    }
    ctx.allocated = count;
}

void RegisterAllocator::assign_locations(
    const Stmt& stmt, AllocationContext& ctx) {
    struct Visitor {
        RegisterAllocator& self;
        AllocationContext& ctx;

        void visit_assign(const Stmt::Assign&) {}

        void visit_define(const Stmt::Define& d) {
            const auto def_id = d.local;
            const auto value = self.func_[def_id]->value();
            const auto location = allocate(value.type());
            self.locations_.set(def_id, location);
        }

        CompiledLocation allocate(RValueType type) {
            switch (type) {
            case RValueType::MethodHandle: {
                auto instance = ctx.allocate();
                auto function = ctx.allocate();
                return CompiledLocation::make_method(instance, function);
            }
            default:
                return CompiledLocation::make_value(ctx.allocate());
            }
        }

        void deallocate(const CompiledLocation& loc) {
            visit_physical_locals(
                loc, [&](CompiledLocalID id) { ctx.deallocate(id); });
        }
    };

    stmt.visit(Visitor{*this, ctx});
}

void RegisterAllocator::visit_children(BlockID parent) {
    const size_t old_size = stack_.size();
    for (auto child : doms_.immediately_dominated(parent))
        stack_.push_back(child);

    std::reverse(
        stack_.begin() + static_cast<ptrdiff_t>(old_size), stack_.end());
}

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
    TIRO_DEBUG_ASSERT(type_ == CompiledLocationType::Value,
        "Bad member access on CompiledLocation: not a Value.");
    return value_;
}

const CompiledLocation::Method& CompiledLocation::as_method() const {
    TIRO_DEBUG_ASSERT(type_ == CompiledLocationType::Method,
        "Bad member access on CompiledLocation: not a Method.");
    return method_;
}

// [[[end]]]

u32 physical_locals_count(const CompiledLocation& loc) {
    switch (loc.type()) {
    case CompiledLocationType::Value:
        return 1;
    case CompiledLocationType::Method:
        return 2;
    }
    TIRO_UNREACHABLE("Invalid location type.");
}

void visit_physical_locals(
    const CompiledLocation& loc, FunctionRef<void(CompiledLocalID)> callback) {
    switch (loc.type()) {
    case CompiledLocationType::Value:
        callback(loc.as_value());
        return;
    case CompiledLocationType::Method:
        callback(loc.as_method().function);
        callback(loc.as_method().instance);
        return;
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
    TIRO_DEBUG_ASSERT(ssa_local, "SSA local must be valid.");
    TIRO_DEBUG_ASSERT(!contains(ssa_local),
        "SSA local must not have been assigned a location.");
    locs_[ssa_local] = loc;

    struct MaxValueVisitor {
        static_assert(std::is_same_v<u32, CompiledLocalID::UnderlyingType>);

        u32 visit_value(CompiledLocalID local) {
            TIRO_DEBUG_ASSERT(local, "Invalid localation.");
            return local.value();
        }

        u32 visit_method(const CompiledLocation::Method& m) {
            TIRO_DEBUG_ASSERT(m.instance, "Invalid localation.");
            TIRO_DEBUG_ASSERT(m.function, "Invalid localation.");
            return std::max(m.instance.value(), m.function.value());
        }
    };

    const u32 max_local = loc.visit(MaxValueVisitor());
    physical_locals_ = std::max(physical_locals_, max_local + 1);
}

CompiledLocation CompiledLocations::get(LocalID ssa_local) const {
    TIRO_DEBUG_ASSERT(contains(ssa_local),
        "SSA local must have been assigned a physical location.");
    return *locs_[ssa_local];
}

std::optional<CompiledLocation>
CompiledLocations::try_get(LocalID ssa_local) const {
    if (locs_.in_bounds(ssa_local))
        return locs_[ssa_local];
    return {};
}

CompiledLocations allocate_locations(const Function& func) {
    RegisterAllocator alloc(func);
    alloc.run();
    return alloc.take_locations();
}

} // namespace tiro
