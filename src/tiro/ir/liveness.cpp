#include "tiro/ir/liveness.hpp"

#include "tiro/ir/locals.hpp"
#include "tiro/ir/traversal.hpp"

#include <algorithm>

namespace tiro {

void LiveInterval::format(FormatStream& stream) const {
    stream.format("{{block: {}, start: {}, end: {}}}", block, start, end);
}

void LiveInterval::build_hash(Hasher& h) const {
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

std::pair<LiveRange::SmallInterval*, bool>
LiveRange::ensure_interval(BlockId block) {
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

const LiveRange* Liveness::live_range(LocalId value) const {
    auto pos = live_ranges_.find(value);
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
        const auto block = func[block_id];

        u32 live_start = 0;
        for (const auto& stmt : block->stmts()) {
            visit_definitions(func, stmt,
                [&](LocalId value) { define(value, block_id, live_start); });
            ++live_start;
        }
    }

    // Visit all uses, propagating liveness information to the predecessor blocks.
    for (auto block_id : PreorderTraversal(func)) {
        const auto block = func[block_id];
        const size_t stmt_count = block->stmt_count();
        const size_t phi_count = block->phi_count(func);

        // Values used as operands in the phi functions must be live-out in their
        // predecessor blocks.
        // They do NOT become live-in in the current block through the phi function.
        {
            const size_t pred_count = block->predecessor_count();
            for (size_t i = 0; i < phi_count; ++i) {
                auto local = func[block->stmt(i).as_define().local];

                const auto& value = local->value();
                if (value.type() != RValueType::Phi)
                    continue;

                auto phi = func[value.as_phi().value];
                TIRO_DEBUG_ASSERT(phi->operand_count() == pred_count,
                    "Mismatch between phi operand count and predecessor "
                    "count.");

                for (size_t p = 0; p < pred_count; ++p) {
                    live_out(phi->operand(p), block->predecessor(p));
                }
            }
        }

        // Handle normal value uses.
        for (size_t i = phi_count; i < stmt_count; ++i) {
            visit_uses(func, block->stmt(i),
                [&](LocalId value) { extend(value, block_id, i); });
        }
        visit_locals(func, block->terminator(),
            [&](LocalId value) { extend(value, block_id, stmt_count); });
    }
}

void Liveness::format(FormatStream& stream) const {
    const Function& func = *func_;

    // Print items in sorted order for better readabilty.
    std::vector<LocalId> values;
    {
        for (const auto& pair : live_ranges())
            values.push_back(pair.first);
        std::sort(values.begin(), values.end());
    }

    stream.format("Liveness:\n");
    for (const auto value : values) {
        const auto range = TIRO_NN(live_range(value));

        stream.format("  Value {}:\n", dump_helpers::DumpLocal{func, value});

        auto def = range->definition();
        stream.format("    - definition: {} [{}-{}]\n",
            dump_helpers::DumpBlock{func, def.block}, def.start, def.end);

        for (auto live : range->live_in_intervals()) {
            stream.format("    - live: {} [{}-{}]\n",
                dump_helpers::DumpBlock{func, live.block}, live.start,
                live.end);
        }
    }
}

void Liveness::live_out(LocalId value, BlockId pred_id) {
    const auto pred = (*func_)[pred_id];
    const size_t end = pred->stmt_count() + 1; // After terminator
    extend(value, pred_id, end);
}

void Liveness::define(LocalId value, BlockId block_id, u32 start) {
    [[maybe_unused]] auto [pos, inserted] = live_ranges_.try_emplace(
        value, LiveInterval(block_id, start, start));
    TIRO_DEBUG_ASSERT(
        inserted, "A live range entry for that value already exists.");
}

void Liveness::extend(LocalId value, BlockId block_id, u32 use) {
    TIRO_DEBUG_ASSERT(
        work_.empty(), "Worklist is always processed until it is empty again.");

    LiveRange* range = [&]() {
        auto pos = live_ranges_.find(value);
        TIRO_DEBUG_ASSERT(pos != live_ranges_.end(),
            "No live range entry exists for that value.");
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

        auto current = func[current_id];
        for (auto pred_id : current->predecessors()) {
            const auto pred = func[pred_id];
            const size_t end = pred->stmt_count() + 1; // After terminator
            if (range->extend(pred_id, end)) {
                work_.push_back(pred_id);
            }
        }
    }
}

} // namespace tiro
