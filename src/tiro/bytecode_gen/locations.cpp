#include "tiro/bytecode_gen/locations.hpp"

#include "tiro/bytecode_gen/parallel_copy.hpp"
#include "tiro/core/defs.hpp"
#include "tiro/core/dynamic_bitset.hpp"
#include "tiro/core/math.hpp"
#include "tiro/core/safe_int.hpp"
#include "tiro/ir/dominators.hpp"
#include "tiro/ir/liveness.hpp"
#include "tiro/ir/locals.hpp"
#include "tiro/ir/traversal.hpp"

#include <algorithm>
#include <unordered_map>

namespace tiro {

namespace {

struct AllocContext {
    static constexpr u32 initial_size = 64;

    DynamicBitset occupied{initial_size};

    void reset() { occupied.clear(); }
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
    struct PhiLink {
        BlockID pred;
        BlockID succ;

        // The predecessors allocation context.
        // TODO: Remembered for allocation of spare local, can be optimized!
        AllocContext ctx;
    };

    void color_block(BlockID block, AllocContext& ctx);
    void occupy_live_in(BlockID block_id, AllocContext& ctx);
    void assign_locations(
        BlockID block_id, u32 stmt_index, const Stmt& stmt, AllocContext& ctx);

    void visit_children(BlockID parent);

    void
    implement_phi_copies(BlockID pred_id, BlockID succ_id, AllocContext& ctx);

    CompiledLocation allocate_registers(LocalID def, AllocContext& ctx);
    void deallocate_registers([[maybe_unused]] LocalID def_id,
        const CompiledLocation& loc, AllocContext& ctx);

    CompiledLocalID allocate_register(AllocContext& ctx);
    void deallocate_register(CompiledLocalID loc, AllocContext& ctx);

private:
    const Function& func_;
    DominatorTree doms_;
    Liveness liveness_;
    CompiledLocations locations_;
    std::vector<BlockID> stack_;
    std::vector<PhiLink> phi_links_;
};

} // namespace

// True if the statement needs a register that is distinct from all input registers.
// That is the case if the statement is implemented using multiple bytecode instructions, because
// we would overwrite our input values otherwise.
static bool needs_distinct_register(const Function& func, const Stmt& stmt) {
    switch (stmt.type()) {
    case StmtType::Assign:
        return false;

    case StmtType::Define: {
        auto& value = func[stmt.as_define().local]->value();
        return value.type() == RValueType::Format;
    }
    }

    TIRO_UNREACHABLE("Invalid statement type.");
}

RegisterAllocator::RegisterAllocator(const Function& func)
    : func_(func)
    , doms_(func)
    , liveness_(func)
    , locations_(func.block_count(), func.local_count()) {}

void RegisterAllocator::run() {
    doms_.compute();
    liveness_.compute();

    // DFS in dominator order.
    // Walk through the cfg in the order induced by the dominator tree (depth first) and
    // perform greedy coloring for all locals encountered on the way.
    // This approach has been found to be optimal (wrt the amount of used locals) by Hack et al.
    stack_.push_back(func_.entry());

    {
        AllocContext ctx;
        while (!stack_.empty()) {
            const auto block_id = stack_.back();
            stack_.pop_back();

            color_block(block_id, ctx);
            visit_children(block_id);
        }
    }

    for (auto& [pred_id, succ_id, ctx] : phi_links_) {
        implement_phi_copies(pred_id, succ_id, ctx);
    }
}

/// Partially implements Algorithm 1 presented in
///
/// Braun, Matthias & Mallon, Christoph & Hack, Sebastian. (2010).
/// Preference-Guided Register Assignment.
/// 6011. 205-223. 10.1007/978-3-642-11970-5_12.
void RegisterAllocator::color_block(BlockID block_id, AllocContext& ctx) {
    const auto block = func_[block_id];
    const size_t phi_count = block->phi_count(func_);
    const size_t stmt_count = block->stmt_count();
    ctx.reset();

    // Mark all live-in registers as occupied.
    occupy_live_in(block_id, ctx);

    // Assign locations to phi functions.
    // Operands of the phi function are not treated as live (unless they're
    // live-in to the block through other means).
    for (size_t i = 0; i < phi_count; ++i) {
        auto def_id = block->stmt(i).as_define().local;
        auto loc = allocate_registers(def_id, ctx);
        locations_.set(def_id, loc);
    }

    // Assign locations to all normal statements.
    for (size_t i = phi_count; i < stmt_count; ++i) {
        assign_locations(block_id, i, block->stmt(i), ctx);
    }

    // Delay implementation of phi operand copying until all nodes have been seen.
    visit_targets(block->terminator(), [&](BlockID succ_id) {
        if (func_[succ_id]->phi_count(func_) > 0) {
            TIRO_DEBUG_ASSERT(
                block->terminator().type() == TerminatorType::Jump,
                "Phi operands can only move over plain jump edges.");

            phi_links_.push_back({block_id, succ_id, ctx});
        }
    });
}

void RegisterAllocator::occupy_live_in(BlockID block_id, AllocContext& ctx) {
    for (const auto local : liveness_.live_in_values(block_id)) {
        auto assigned = locations_.get(local);
        visit_physical_locals(assigned, [&](CompiledLocalID id) {
            TIRO_DEBUG_ASSERT(id, "Invalid assigned location.");
            ctx.occupied.set(id.value());
        });
    }
}

void RegisterAllocator::assign_locations(
    BlockID block_id, u32 stmt_index, const Stmt& stmt, AllocContext& ctx) {

    const bool needs_distinct = needs_distinct_register(func_, stmt);
    auto reuse_dead_vars = [&]() {
        // Deallocate operands that die at this statement.
        // Multiple visits are fine (-> redundant clears on bitset).
        visit_uses(func_, stmt, [&](LocalID value_id) {
            auto live_range = TIRO_NN(liveness_.live_range(value_id));
            if (live_range->last_use(block_id, stmt_index)) {
                auto loc = locations_.get(value_id);
                deallocate_registers(value_id, loc, ctx);
            }
        });
    };

    if (!needs_distinct)
        reuse_dead_vars();

    // Assign locations to the defined values (if any).
    visit_definitions(func_, stmt, [&](LocalID def_id) {
        auto loc = allocate_registers(def_id, ctx);
        locations_.set(def_id, loc);
    });

    // Immediately free all locations that are never read.
    visit_definitions(func_, stmt, [&](LocalID def_id) {
        auto live_range = TIRO_NN(liveness_.live_range(def_id));
        if (live_range->dead()) {
            auto loc = locations_.get(def_id);
            deallocate_registers(def_id, loc, ctx);
        }
    });

    if (needs_distinct)
        reuse_dead_vars();
}

void RegisterAllocator::implement_phi_copies(
    BlockID pred_id, BlockID succ_id, AllocContext& ctx) {
    auto succ = func_[succ_id];

    const size_t phi_count = succ->phi_count(func_);
    if (phi_count == 0)
        return;

    const size_t index_in_succ = [&]() {
        for (size_t i = 0, n = succ->predecessor_count(); i < n; ++i) {
            if (succ->predecessor(i) == pred_id)
                return i;
        }
        TIRO_ERROR("Failed to find predecessor block in successor.");
    }();

    // TODO: Small vec
    std::vector<RegisterCopy> copies;
    for (size_t i = 0; i < phi_count; ++i) {
        auto phi_local_id = succ->stmt(i).as_define().local;
        auto phi_id = func_[phi_local_id]->value().as_phi().value;
        auto phi = func_[phi_id];
        auto source_local_id = phi->operand(index_in_succ);

        auto source_loc = locations_.get(source_local_id);
        auto dest_loc = locations_.get(phi_local_id);
        TIRO_CHECK(source_loc.type() == CompiledLocationType::Value
                       && dest_loc.type() == CompiledLocationType::Value,
            "Only plain values can be passed to phi arguments.");

        if (source_loc != dest_loc)
            copies.push_back({source_loc.as_value(), dest_loc.as_value()});
    }

    sequentialize_parallel_copies(
        copies, [&]() { return allocate_register(ctx); });

    locations_.set_phi_copies(pred_id, std::move(copies));
}

void RegisterAllocator::visit_children(BlockID parent) {
    const size_t old_size = stack_.size();
    for (auto child : doms_.immediately_dominated(parent))
        stack_.push_back(child);

    std::reverse(
        stack_.begin() + static_cast<ptrdiff_t>(old_size), stack_.end());
}

CompiledLocation
RegisterAllocator::allocate_registers(LocalID def_id, AllocContext& ctx) {
    // TODO: Hacky way to represent multi-register values.
    const auto type = func_[def_id]->value().type();
    switch (type) {
    case RValueType::MethodHandle: {
        auto instance = allocate_register(ctx);
        auto function = allocate_register(ctx);
        return CompiledLocation::make_method(instance, function);
    }
    default:
        return allocate_register(ctx);
    }
}

void RegisterAllocator::deallocate_registers([[maybe_unused]] LocalID def_id,
    const CompiledLocation& loc, AllocContext& ctx) {
    visit_physical_locals(
        loc, [&](CompiledLocalID reg) { deallocate_register(reg, ctx); });
}

// Naive implementation: just return the first free register.
// Can be improved by implementing the "register preference" approach described
// by Braun et al.
CompiledLocalID RegisterAllocator::allocate_register(AllocContext& ctx) {
    auto& occupied = ctx.occupied;
    auto reg = occupied.first_unset();
    if (reg == DynamicBitset::npos) {
        // TODO: overflow?
        reg = occupied.size();
        occupied.resize(
            std::max(AllocContext::initial_size, occupied.size() * 2));
    }
    occupied.set(reg);

    if (reg >= locations_.total_registers()) {
        locations_.total_registers(reg + 1);
    }

    return CompiledLocalID(reg);
}

void RegisterAllocator::deallocate_register(
    CompiledLocalID reg, AllocContext& ctx) {
    TIRO_DEBUG_ASSERT(reg, "Invalid register.");
    ctx.occupied.clear(reg.value());
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

bool operator==(const CompiledLocation& lhs, const CompiledLocation& rhs) {
    if (lhs.type() != rhs.type())
        return false;

    struct EqualityVisitor {
        const CompiledLocation& rhs;

        bool
        visit_value([[maybe_unused]] const CompiledLocation::Value& value) {
            [[maybe_unused]] const auto& other = rhs.as_value();
            return value == other;
        }

        bool
        visit_method([[maybe_unused]] const CompiledLocation::Method& method) {
            [[maybe_unused]] const auto& other = rhs.as_method();
            return method.instance == other.instance
                   && method.function == other.function;
        }
    };
    return lhs.visit(EqualityVisitor{rhs});
}

bool operator!=(const CompiledLocation& lhs, const CompiledLocation& rhs) {
    return !(lhs == rhs);
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

CompiledLocations::CompiledLocations(
    size_t total_blocks, size_t total_ssa_locals) {
    copies_.resize(total_blocks);
    locs_.resize(total_ssa_locals);
}

bool CompiledLocations::contains(LocalID ssa_local) const {
    return locs_[ssa_local].has_value();
}

void CompiledLocations::set(LocalID ssa_local, const CompiledLocation& loc) {
    TIRO_DEBUG_ASSERT(ssa_local, "SSA local must be valid.");
    locs_[ssa_local] = loc;
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

bool CompiledLocations::has_phi_copies(BlockID block) const {
    return copies_.in_bounds(block) && !copies_[block].empty();
}

void CompiledLocations::set_phi_copies(
    BlockID block, std::vector<RegisterCopy> copies) {
    TIRO_DEBUG_ASSERT(block, "Block must be valid.");
    copies_[block] = std::move(copies);
}

const std::vector<RegisterCopy>&
CompiledLocations::get_phi_copies(BlockID block) const {
    TIRO_DEBUG_ASSERT(block, "Block must be valid.");
    return copies_[block];
}

CompiledLocations allocate_locations(const Function& func) {
    RegisterAllocator alloc(func);
    alloc.run();
    return alloc.take_locations();
}

} // namespace tiro
