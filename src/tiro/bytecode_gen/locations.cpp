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
    // Holds free registers with `value < allocated - 1`.
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

    const Function& func() const { return func_; }
    const Liveness& liveness() const { return liveness_; }
    const CompiledLocations& locations() const { return locations_; }

private:
    void find_phi_classes();
    void available_locations(BlockID block_id, AllocationContext& ctx);

    void assign_locations(BlockID block_id, u32 stmt_index, const Stmt& stmt,
        AllocationContext& ctx);

    void visit_children(BlockID parent);

private:
    const Function& func_;
    DominatorTree doms_;
    Liveness liveness_;
    CompiledLocations locations_;

    std::vector<BlockID> stack_;
};

} // namespace

template<typename T>
static void unique_ordered(std::vector<T>& vec) {
    std::sort(vec.begin(), vec.end());
    vec.erase(std::unique(vec.begin(), vec.end()), vec.end());
}

RegisterAllocator::RegisterAllocator(const Function& func)
    : func_(func)
    , doms_(func)
    , liveness_(func)
    , locations_(func.local_count()) {}

void RegisterAllocator::run() {
    doms_.compute();
    liveness_.compute();

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

        size_t index = 0;
        for (const auto& stmt : block->stmts()) {
            assign_locations(block_id, index, stmt, ctx);
            ++index;
        }

        visit_children(block_id);
    }
}

void RegisterAllocator::available_locations(
    BlockID block_id, AllocationContext& ctx) {
    ctx.reset();

    // TODO small vec? reuse?
    std::vector<u32> allocated;

    // Mark live-in values as used to ensure we don't interfere with them.
    for (const auto local : liveness_.live_in_values(block_id)) {
        auto assigned = locations_.get(local);
        visit_physical_locals(assigned, [&](CompiledLocalID id) {
            TIRO_DEBUG_ASSERT(id, "Invalid assigned location.");
            allocated.push_back(id.value());
        });
    }

    // All operands to phi functions are also treated as live at the entry of a block.
    // This is not as efficient as it could be (lots of copying to 'call' phi functions)
    // but it will do for now.
    auto block = func_[block_id];
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

    unique_ordered(allocated);
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

void RegisterAllocator::assign_locations(BlockID block_id, u32 stmt_index,
    const Stmt& stmt, AllocationContext& ctx) {
    struct Visitor {
        RegisterAllocator& self;
        AllocationContext& ctx;
        BlockID block_id;
        const Stmt& stmt;
        u32 stmt_index;

        void visit_assign(const Stmt::Assign&) { deallocate_dead_uses(); }

        void visit_define(const Stmt::Define& d) {
            const auto def_id = d.local;
            const auto value = self.func_[def_id]->value();
            // TODO: Game plan
            // - Assign phi function locations and normal defines separately
            // - Phi functions interfere with all their arguments (suboptimal, but good enough for now)
            // - Mark phi defines live at the beginning of the block, in addition to the normal live-ins.

            // FIXME
            // deallocate_dead_uses();

            const auto location = allocate(value.type());
            self.locations_.set(def_id, location);
            if (dead_define(def_id)) {
                deallocate(location);
            }
        }

        bool dead_define(LocalID local) {
            auto range = TIRO_NN(self.liveness().live_range(local));
            return range->dead();
        }

        void deallocate_dead_uses() {
            // Gather locals that are dead after this statement.
            // TODO small vector, reuse.
            std::vector<LocalID> free_locals;
            visit_uses(self.func(), stmt, [&](LocalID value) {
                auto range = TIRO_NN(self.liveness().live_range(value));
                if (range->last_use(block_id, stmt_index))
                    free_locals.push_back(value);
            });

            // Only consider every local once.
            unique_ordered(free_locals);

            // Deallocate the storage for the dead locals.
            for (auto local : free_locals) {
                auto loc = self.locations().get(local);
                deallocate(loc);
            }
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

    stmt.visit(Visitor{*this, ctx, block_id, stmt, stmt_index});
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
