#include "compiler/bytecode_gen/alloc_registers.hpp"

#include "common/dynamic_bitset.hpp"
#include "common/math.hpp"
#include "common/safe_int.hpp"
#include "compiler/bytecode_gen/parallel_copy.hpp"
#include "compiler/ir_passes/dominators.hpp"
#include "compiler/ir_passes/liveness.hpp"
#include "compiler/ir_passes/visit.hpp"

#include "absl/container/flat_hash_map.h"

#include <algorithm>

namespace tiro {

namespace {

using ir::Function;
using ir::BlockId;
using ir::InstId;
using ir::Liveness;
using ir::DominatorTree;
using ir::ValueType;
using ir::TerminatorType;

class RegisterContext {
public:
    explicit RegisterContext(u32 preallocated)
        : preallocated_(preallocated) {}

    void reset() { occupied.clear(); }

    BytecodeRegister get_fresh();

    void set_occupied(const BytecodeLocation& loc);
    void clear_occupied(const BytecodeLocation& loc);

    void set_occupied(BytecodeRegister reg);
    void clear_occupied(BytecodeRegister reg);

private:
    static constexpr size_t initial_size = 64;

    u32 to_bitset_index(BytecodeRegister reg);
    BytecodeRegister from_bitset_index(u32 index);

    void ensure_bitset_size(u32 bitset_index);

private:
    u32 preallocated_;
    DynamicBitset occupied{initial_size};
};

class RegisterAllocator final {
public:
    explicit RegisterAllocator(const Function& func);

    void run();

    BytecodeLocations take_locations() { return std::move(locations_); }

    const Function& func() const { return func_; }
    const Liveness& liveness() const { return liveness_; }
    const BytecodeLocations& locations() const { return locations_; }

private:
    struct PhiLink {
        BlockId pred;
        BlockId succ;

        // The predecessors allocation context.
        // TODO: Remembered for allocation of spare local, can be optimized!
        RegisterContext ctx;
    };

    void preallocate_registers();

    void color_block(BlockId block, RegisterContext& ctx);
    void occupy_live_in(BlockId block_id, RegisterContext& ctx);
    void assign_locations(BlockId block_id, u32 stmt_index, InstId inst, RegisterContext& ctx);

    void visit_children(BlockId parent);

    void implement_phi_copies(BlockId pred_id, BlockId succ_id, RegisterContext& ctx);

    BytecodeLocation allocate_registers(InstId def, RegisterContext& ctx);
    void deallocate_registers(InstId def_id, const BytecodeLocation& loc, RegisterContext& ctx);

    BytecodeRegister allocate_register(RegisterContext& ctx);
    void deallocate_register(BytecodeRegister loc, RegisterContext& ctx);

    u32 allocated_size(InstId inst_id);
    std::optional<u32> allocated_size_recursive(InstId inst_id);
    std::optional<u32> allocated_size_realized(InstId inst_id);

    // True if the instruction needs a register that is distinct from all input registers.
    // That is the case if the instruction is implemented using multiple bytecode instructions, because
    // we would overwrite our input values otherwise.
    bool needs_distinct_register(InstId inst_id);

    // Returns the matching symbol key for preallocated value locations, or an empty optional
    // if this is a normal ssa instruction.
    std::optional<SymbolId> check_preallocated(InstId inst_id);

    RegisterContext make_context() { return RegisterContext(preallocated_); }

private:
    const Function& func_;
    DominatorTree doms_;
    Liveness liveness_;
    BytecodeLocations locations_;

    // Number of registers preallocated (e.g. for symbol_locations_) before allocating registers according
    // to liveness analysis. The first N registers are reserved for the lifetime of the entire function.
    u32 preallocated_ = 0;

    // Depth first search traversal of the dominator tree.
    std::vector<BlockId> stack_;

    // Predecessor to successor links, successor receives phi arguments.
    std::vector<PhiLink> phi_links_;

    // Sizes (in registers) for phi functions, determined at first
    // argument site. 0 is used as a sentinel value to mark an active
    // recursive call (0 is never a valid value in this map).
    absl::flat_hash_map<InstId, u32, UseHasher> phi_sizes_;

    // Implements parallel copy -> sequential copy algorithm.
    ParallelCopyAlgorithm parallel_copies_;
};

} // namespace

BytecodeRegister RegisterContext::get_fresh() {
    auto index = occupied.find_unset();
    if (index == DynamicBitset::npos) {
        index = occupied.size();
        ensure_bitset_size(index);
    }
    occupied.set(index);

    return from_bitset_index(index);
}

void RegisterContext::set_occupied(const BytecodeLocation& loc) {
    for (const auto& reg : loc)
        set_occupied(reg);
}

[[maybe_unused]] void RegisterContext::clear_occupied(const BytecodeLocation& loc) {
    for (const auto& reg : loc)
        clear_occupied(reg);
}

void RegisterContext::set_occupied(BytecodeRegister reg) {
    TIRO_DEBUG_ASSERT(reg, "Invalid register.");
    if (reg.value() < preallocated_)
        return;

    const auto index = to_bitset_index(reg);
    ensure_bitset_size(index);
    occupied.set(index);
}

void RegisterContext::clear_occupied(BytecodeRegister reg) {
    TIRO_DEBUG_ASSERT(reg, "Invalid register.");
    if (reg.value() < preallocated_)
        return;

    const auto index = to_bitset_index(reg);
    ensure_bitset_size(index);
    occupied.clear(index);
}

u32 RegisterContext::to_bitset_index(BytecodeRegister reg) {
    TIRO_DEBUG_ASSERT(reg.value() >= preallocated_,
        "Preallocated registers must not be used in the occupied bitset.");
    return reg.value() - preallocated_;
}

BytecodeRegister RegisterContext::from_bitset_index(u32 bitset_index) {
    return BytecodeRegister(bitset_index + preallocated_);
}

void RegisterContext::ensure_bitset_size(u32 bitset_index) {
    if (bitset_index >= occupied.size())
        occupied.resize((SafeInt(bitset_index) + 1).value());
}

RegisterAllocator::RegisterAllocator(const Function& func)
    : func_(func)
    , doms_(func)
    , liveness_(func)
    , locations_(func.block_count(), func.inst_count()) {}

void RegisterAllocator::run() {
    preallocate_registers();

    doms_.compute();
    liveness_.compute();

    // DFS in dominator order.
    // Walk through the cfg in the order induced by the dominator tree (depth first) and
    // perform greedy coloring for all insts encountered on the way.
    // This approach has been found to be optimal (wrt the amount of used registers) by Hack et al.
    stack_.push_back(func_.entry());

    {
        RegisterContext ctx = make_context();
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

void RegisterAllocator::preallocate_registers() {
    u32 total = 0;

    // Walk all observe_assign instructions in all handler blocks to find all
    // referenced symbols.
    const auto entry_block = func_[func_.entry()];
    for (const auto& handler_id : entry_block->terminator().as_entry().handlers) {
        const auto handler_block = func_[handler_id];
        for (const auto& inst_id : handler_block->insts()) {
            const auto inst = func_[inst_id];
            if (inst->value().type() != ValueType::ObserveAssign)
                continue;

            const auto symbol_id = inst->value().as_observe_assign().symbol;
            TIRO_DEBUG_ASSERT(symbol_id, "Invalid symbol id.");

            if (locations_.has_preallocated_location(symbol_id))
                continue;

            std::array<BytecodeRegister, BytecodeLocation::max_size()> allocated{};
            const u32 regs = allocated_size(inst_id);
            TIRO_DEBUG_ASSERT(regs < allocated.size(), "Too many registers.");
            for (u32 i = 0; i < regs; ++i) {
                allocated[i] = BytecodeRegister(total++);
            }

            locations_.set_preallocated_location(
                symbol_id, BytecodeLocation(Span(allocated.data(), regs)));
        }
    }

    locations_.total_registers(total);
    preallocated_ = total;
}

/// Partially implements Algorithm 1 presented in
///
/// Braun, Matthias & Mallon, Christoph & Hack, Sebastian. (2010).
/// Preference-Guided Register Assignment.
/// 6011. 205-223. 10.1007/978-3-642-11970-5_12.
void RegisterAllocator::color_block(BlockId block_id, RegisterContext& ctx) {
    const auto block = func_[block_id];
    const size_t phi_count = block->phi_count(func_);
    const size_t stmt_count = block->inst_count();
    ctx.reset();

    // Mark all live-in registers as occupied.
    occupy_live_in(block_id, ctx);

    // Assign locations to phi functions.
    // Operands of the phi function are not treated as live (unless they're
    // live-in to the block through other means).
    for (size_t i = 0; i < phi_count; ++i) {
        auto inst_id = block->inst(i);
        auto loc = allocate_registers(inst_id, ctx);
        locations_.set(inst_id, loc);
    }

    // Assign locations to all normal statements.
    for (size_t i = phi_count; i < stmt_count; ++i) {
        assign_locations(block_id, i, block->inst(i), ctx);
    }

    // Delay implementation of phi operand copying until all nodes have been seen.
    visit_targets(block->terminator(), [&](BlockId succ_id) {
        if (func_[succ_id]->phi_count(func_) > 0) {
            TIRO_DEBUG_ASSERT(block->terminator().type() == TerminatorType::Jump,
                "Phi operands can only move over plain jump edges.");

            phi_links_.push_back({block_id, succ_id, ctx});
        }
    });
}

void RegisterAllocator::occupy_live_in(BlockId block_id, RegisterContext& ctx) {
    for (const auto inst : liveness_.live_in_values(block_id))
        ctx.set_occupied(locations_.get(inst));
}

void RegisterAllocator::assign_locations(
    BlockId block_id, u32 stmt_index, InstId inst, RegisterContext& ctx) {

    const bool needs_distinct = needs_distinct_register(inst);
    auto reuse_dead_vars = [&]() {
        // Deallocate operands that die at this statement.
        // Multiple visits are fine (-> redundant clears on bitset).
        visit_inst_operands(func_, inst, [&](InstId value_id) {
            auto live_range = TIRO_NN(liveness_.live_range(value_id));
            if (live_range->last_use(block_id, stmt_index)) {
                auto loc = locations_.get(value_id);
                deallocate_registers(value_id, loc, ctx);
            }
        });
    };

    if (!needs_distinct)
        reuse_dead_vars();

    // Assign locations to the defined values.
    {
        auto loc = allocate_registers(inst, ctx);
        locations_.set(inst, loc);
    }

    // Immediately free all locations that are never read.
    {
        auto live_range = TIRO_NN(liveness_.live_range(inst));
        if (live_range->dead()) {
            auto loc = locations_.get(inst);
            deallocate_registers(inst, loc, ctx);
        }
    }

    if (needs_distinct)
        reuse_dead_vars();
}

void RegisterAllocator::implement_phi_copies(
    BlockId pred_id, BlockId succ_id, RegisterContext& ctx) {
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
    for (size_t phi_index = 0; phi_index < phi_count; ++phi_index) {
        auto phi_inst_id = succ->inst(phi_index);
        auto& phi = func_[phi_inst_id]->value().as_phi();
        auto source_inst_id = phi.operand(func_, index_in_succ);

        auto source_loc = storage_location(source_inst_id, locations_, func_);
        auto dest_loc = storage_location(phi_inst_id, locations_, func_);
        TIRO_CHECK(source_loc.size() == dest_loc.size(), "Locations must have the same size.");

        // Ensure that all registers are marked as occupied. This is important
        // for the allocation of spare registers (in the sequentialize_parallel_copies algorithm).
        // If this would not be done, we risk using an existing register for temporary storage, resulting
        // in data corruption.
        ctx.set_occupied(source_loc);
        ctx.set_occupied(dest_loc);

        for (u32 i = 0, n = source_loc.size(); i < n; ++i) {
            copies.push_back({source_loc[i], dest_loc[i]});
        }
    }
    parallel_copies_.sequentialize(copies, [&]() { return allocate_register(ctx); });
    locations_.set_phi_copies(pred_id, std::move(copies));
}

void RegisterAllocator::visit_children(BlockId parent) {
    const size_t old_size = stack_.size();
    for (auto child : doms_.immediately_dominated(parent))
        stack_.push_back(child);

    std::reverse(stack_.begin() + old_size, stack_.end());
}

BytecodeLocation RegisterAllocator::allocate_registers(InstId def_id, RegisterContext& ctx) {
    if (auto symbol_id = check_preallocated(def_id)) {
        return locations_.get_preallocated_location(*symbol_id);
    }

    constexpr u32 buffer_size = BytecodeLocation::max_size();

    std::array<BytecodeRegister, buffer_size> allocated{};
    const u32 regs = allocated_size(def_id);
    TIRO_DEBUG_ASSERT(regs <= buffer_size, "Too many registers.");
    for (u32 i = 0; i < regs; ++i) {
        allocated[i] = allocate_register(ctx);
    }
    return BytecodeLocation(Span<const BytecodeRegister>(allocated.data(), regs));
}

void RegisterAllocator::deallocate_registers(
    InstId def_id, const BytecodeLocation& loc, RegisterContext& ctx) {
    if (check_preallocated(def_id))
        return;

    for (auto reg : loc) {
        deallocate_register(reg, ctx);
    }
}

// Naive implementation: just return the first free register.
// Can be improved by implementing the "register preference" approach described
// by Braun et al.
BytecodeRegister RegisterAllocator::allocate_register(RegisterContext& ctx) {
    const auto reg = ctx.get_fresh();

    if (reg.value() >= locations_.total_registers())
        locations_.total_registers(reg.value() + 1);

    return BytecodeRegister(reg);
}

void RegisterAllocator::deallocate_register(BytecodeRegister reg, RegisterContext& ctx) {
    TIRO_DEBUG_ASSERT(reg, "Invalid register.");
    TIRO_DEBUG_ASSERT(reg.value() >= preallocated_, "Register must not be preallocated.");
    ctx.clear_occupied(reg);
}

u32 RegisterAllocator::allocated_size(InstId inst_id) {
    auto size = allocated_size_recursive(inst_id);
    TIRO_CHECK(size, "Register size of instruction could not be computed.");
    return *size;
}

// The number of registers to allocate for the given value.
// Most values require 1 register. Aggregates may be larger than one register.
// Aggregate member accesses are register aliases and do not require any registes
// by themselves.
// TODO: Most of this complexity would go away if phi functions had static types!
std::optional<u32> RegisterAllocator::allocated_size_recursive(InstId inst_id) {
    auto& value = func_[inst_id]->value();
    switch (value.type()) {
    case ValueType::Write:
        return 0;
    case ValueType::Aggregate:
        return aggregate_size(value.as_aggregate().type());
    case ValueType::GetAggregateMember:
        return 0;
    case ValueType::Phi: {
        if (auto pos = phi_sizes_.find(inst_id); pos != phi_sizes_.end()) {
            u32 size = pos->second;
            if (size != 0)
                return size;

            // size 0 -> recursive call. This breaks the otherwise infinite loop.
            return {};
        }

        auto& phi = value.as_phi();

        if (!phi.operands())
            return 0;
        auto operands = func_[phi.operands()];

        phi_sizes_[inst_id] = 0;
        std::optional<u32> resolved;
        for (size_t i = 0, n = operands->size(); i < n; ++i) {
            auto arg_size = allocated_size_realized(operands->get(i));
            if (arg_size) {
                if (resolved) {
                    TIRO_DEBUG_ASSERT(*resolved == *arg_size,
                        "Phi operands must not resolve to different sizes.");
                } else {
                    resolved = arg_size;
                }
            }
        }

        TIRO_DEBUG_ASSERT(resolved, "Register size of phi function could not be resolved.");
        phi_sizes_[inst_id] = *resolved;
        return *resolved;
    }
    default:
        return 1;
    }
}

// Returns the register size required for the realization of the given inst. This is
// either simply `allocated_register_size()` (for normal values) or the register size of the aliased
// registers (for example, when using aggregate members).
std::optional<u32> RegisterAllocator::allocated_size_realized(InstId inst_id) {
    auto& value = func_[inst_id]->value();
    if (value.type() == ValueType::GetAggregateMember) {
        auto& get_member = value.as_get_aggregate_member();
        return aggregate_member_size(get_member.member);
    }
    return allocated_size_recursive(inst_id);
}

// True if the instruction needs a register that is distinct from all input registers.
// That is the case if the instruction is implemented using multiple bytecode instructions, because
// we would overwrite our input values otherwise.
bool RegisterAllocator::needs_distinct_register(InstId inst_id) {
    auto& value = func_[inst_id]->value();
    return value.type() == ValueType::Format || value.type() == ValueType::Record;
}

std::optional<SymbolId> RegisterAllocator::check_preallocated(InstId inst_id) {
    auto& value = func_[inst_id]->value();
    switch (value.type()) {
    case ValueType::PublishAssign:
        return value.as_publish_assign().symbol;
    default:
        return {};
    }
}

BytecodeLocations allocate_locations(const Function& func) {
    RegisterAllocator alloc(func);
    alloc.run();
    return alloc.take_locations();
}

} // namespace tiro
