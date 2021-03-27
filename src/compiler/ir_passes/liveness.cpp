#include "compiler/ir_passes/liveness.hpp"

#include "compiler/ir/traversal.hpp"
#include "compiler/ir_passes/visit.hpp"

#include <algorithm>

namespace tiro::ir {

void LiveInterval::format(FormatStream& stream) const {
    stream.format("{{block: {}, start: {}, end: {}}}", block, start, end);
}

void LiveInterval::hash(Hasher& h) const {
    h.append(block, start, end);
}

LiveRange::LiveRange(const LiveInterval& def)
    : def_(def) {
    TIRO_DEBUG_ASSERT(def_.block, "Block must be valid.");
}

bool LiveRange::extend(BlockId block, u32 stmt) {
    TIRO_DEBUG_ASSERT(block, "Invalid block id.");

    // Handle extension in the defining block.
    if (block == def_.block) {
        def_.end = std::max(def_.end, stmt);
        return false;
    }

    // All other blocks are live-in.
    auto [interval, inserted] = ensure_interval(block);
    interval->second = std::max(interval->second, stmt);
    return inserted;
}

bool LiveRange::live_in(BlockId block) const {
    return find_interval(block) != nullptr;
}

bool LiveRange::last_use(BlockId block, u32 stmt) const {
    if (block == def_.block)
        return def_.end == stmt;

    const auto interval = find_interval(block);
    return interval && interval->second == stmt;
}

std::pair<LiveRange::SmallInterval*, bool> LiveRange::ensure_interval(BlockId block) {
    auto pos = std::lower_bound(live_in_.begin(), live_in_.end(), block,
        [&](const auto& pair, BlockId id) { return pair.first < id; });

    if (pos != live_in_.end() && pos->first == block)
        return std::pair(&*pos, false);

    pos = live_in_.insert(pos, std::pair(block, 0));
    return std::pair(&*pos, true);
}

const LiveRange::SmallInterval* LiveRange::find_interval(BlockId block) const {
    auto pos = std::lower_bound(live_in_.begin(), live_in_.end(), block,
        [&](const auto& pair, BlockId id) { return pair.first < id; });
    if (pos == live_in_.end() || pos->first != block)
        return nullptr;

    return &*pos;
}

Liveness::Liveness(const Function& func)
    : func_(func) {}

const LiveRange* Liveness::live_range(InstId value) const {
    auto pos = live_ranges_.find(normalize(value));
    return (pos != live_ranges_.end()) ? &pos->second : nullptr;
}

void Liveness::compute() {
    const Function& func = *func_;
    live_ranges_.clear();
    live_sets_.resize(func.block_count());
    work_.clear();

    // Define all local variables. This approach makes two passes in total to remain
    // indifferent about the order in which blocks are visited. This could be packed
    // into a single pass if we would visit the blocks in dominator order (since in our SSA
    // IR, every use is dominated by its definition).
    for (auto block_id : PreorderTraversal(func)) {
        const auto& block = func[block_id];

        u32 live_start = 0;
        for (const auto& inst : block.insts()) {
            insert_definition(inst, block_id, live_start);
            ++live_start;
        }
    }

    // Visit all uses, propagating liveness information to the predecessor blocks.
    for (auto block_id : PreorderTraversal(func)) {
        const auto& block = func[block_id];
        const size_t stmt_count = block.inst_count();
        const size_t phi_count = block.phi_count(func);

        // Values used as operands in the phi functions must be live-out in their
        // predecessor blocks.
        // They do NOT become live-in in the current block through the phi function.
        {
            const size_t pred_count = block.predecessor_count();
            for (size_t i = 0; i < phi_count; ++i) {
                const auto& inst = func[block.inst(i)];
                const auto& value = inst.value();
                switch (value.type()) {
                case ValueType::Phi: {
                    auto& phi = value.as_phi();
                    TIRO_DEBUG_ASSERT(phi.operand_count(func) == pred_count,
                        "Mismatch between phi operand count and predecessor "
                        "count.");

                    for (size_t p = 0; p < pred_count; ++p) {
                        extend_live_out(phi.operand(func, p), block.predecessor(p));
                    }
                    break;
                }
                default:
                    TIRO_DEBUG_ASSERT(false, "Unexpected phi value type.");
                }
            }
        }

        // Handle normal value uses.
        for (size_t i = phi_count; i < stmt_count; ++i) {
            auto inst_id = block.inst(i);
            const auto& inst = func[inst_id];

            // ObserveAssign instructions do *not* influence the liveness of their operands because
            // they are reached through exceptional control flow.
            if (inst.value().type() == ValueType::ObserveAssign)
                continue;

            visit_inst_operands(
                func, inst_id, [&](InstId value) { extent_statement(value, block_id, i); });
        }
        visit_insts(func, block.terminator(),
            [&](InstId value) { extent_statement(value, block_id, stmt_count); });
    }
}

void Liveness::format(FormatStream& stream) const {
    const Function& func = *func_;

    // Print items in sorted order for better readabilty.
    std::vector<InstId> values;
    {
        for (const auto& pair : live_ranges())
            values.push_back(pair.first);
        std::sort(values.begin(), values.end());
    }

    stream.format("Liveness:\n");
    for (const auto value : values) {
        const auto range = TIRO_NN(live_range(value));

        stream.format("  Value {}:\n", dump_helpers::dump(func, value));

        auto def = range->definition();
        stream.format("    - definition: {} [{}-{}]\n", dump_helpers::dump(func, def.block),
            def.start, def.end);

        for (auto live : range->live_in_intervals()) {
            stream.format("    - live: {} [{}-{}]\n", dump_helpers::dump(func, live.block),
                live.start, live.end);
        }
    }
}

void Liveness::extend_live_out(InstId value, BlockId pred_id) {
    const auto& pred = (*func_)[pred_id];
    const size_t end = pred.inst_count() + 1; // After terminator
    extent_statement(value, pred_id, end);
}

void Liveness::insert_definition(InstId value, BlockId block_id, u32 start) {
    if (is_aggregate_reference(value))
        return;

    [[maybe_unused]] auto [pos, inserted] = live_ranges_.try_emplace(
        value, LiveInterval(block_id, start, start));
    TIRO_DEBUG_ASSERT(inserted, "A live range entry for that value already exists.");
}

InstId Liveness::normalize(InstId id) const {
    const auto& value = (*func_)[id].value();
    if (value.type() == ValueType::GetAggregateMember)
        return value.as_get_aggregate_member().aggregate;
    return id;
}

bool Liveness::is_aggregate_reference(InstId id) const {
    const auto& value = (*func_)[id].value();
    return value.type() == ValueType::GetAggregateMember;
}

void Liveness::extent_statement(InstId value, BlockId block_id, u32 use) {
    TIRO_DEBUG_ASSERT(work_.empty(), "Worklist is always processed until it is empty again.");

    value = normalize(value);

    LiveRange* range = [&]() {
        auto pos = live_ranges_.find(value);
        TIRO_DEBUG_ASSERT(pos != live_ranges_.end(), "No live range entry exists for that value.");
        return &pos->second;
    }();

    // Extend returns true when a new interval is created for that block. We use it
    // as a marker value to know when we have to recurse into the predecessor blocks.
    if (range->extend(block_id, use)) {
        work_.push_back(block_id);
    }

    // Propagate liveness information to all predecessors.
    const Function& func = *func_;
    while (!work_.empty()) {
        auto current_id = work_.back();
        work_.pop_back();
        live_sets_[current_id].push_back(value);

        const auto& current = func[current_id];
        for (auto pred_id : current.predecessors()) {
            const auto& pred = func[pred_id];
            const size_t end = pred.inst_count() + 1; // After terminator
            if (range->extend(pred_id, end)) {
                work_.push_back(pred_id);
            }
        }
    }
}

} // namespace tiro::ir
