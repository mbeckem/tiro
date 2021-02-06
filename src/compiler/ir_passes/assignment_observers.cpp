#include "compiler/ir_passes/assignment_observers.hpp"

#include "compiler/ir/function.hpp"
#include "compiler/ir/traversal.hpp"

#include "absl/container/flat_hash_map.h"
#include "absl/container/flat_hash_set.h"
#include "absl/container/inlined_vector.h"

namespace tiro::ir {

namespace {

using ValueSet = absl::flat_hash_set<InstId, UseHasher>;

using ValueList = absl::InlinedVector<InstId, 3>;

class Pass {
public:
    explicit Pass(Function& func)
        : func_(func) {}

    void run();

private:
    // Walk the cfg and index blocks that have exception handlers.
    // Returns false if there are none, in which case the pass can stop immediately.
    bool analyze_cfg();

    // Implements all virtual block --> handler edges by referencing relevant assignments
    // in the exception handler's observe_assign instructions.
    void link_instructions();

    // Gathers all assignments for that block + symbol combination.
    void fill_operand_set(ValueSet& operands, BlockId block_id, SymbolId symbol_id);

    // Gathers the value(s) of the given symbol at the start of the block.
    // CAREFUL: in_values must not modified, pointer references unstable hash map memory.
    ValueList* in_values(BlockId block_id, SymbolId symbol_id);

    // Gathers the value(s) of the given symbol at the end of the block.
    // CAREFUL: out_values must not modified, pointer references unstable hash map memory.
    ValueList* out_values(BlockId block_id, SymbolId symbol_id);

private:
    Function& func_;

    // Maps from handler blocks to source blocks that use that handler.
    absl::flat_hash_map<BlockId, std::vector<BlockId>, UseHasher> reverse_handlers_;

    // Memoized values for the symbol's value at the start of a block.
    absl::flat_hash_map<std::tuple<BlockId, SymbolId>, ValueList, UseHasher> in_values_;

    // Memoized values for the symbol's value at the end of a block.
    absl::flat_hash_map<std::tuple<BlockId, SymbolId>, ValueList, UseHasher> out_values_;
};

} // namespace

void Pass::run() {
    if (!analyze_cfg())
        return; // Fast path for functions that do not have exception handlers.

    link_instructions();
}

bool Pass::analyze_cfg() {
    for (auto block_id : PreorderTraversal(func_)) {
        auto block = func_[block_id];
        auto handler_id = block->handler();
        if (handler_id) {
            reverse_handlers_[handler_id].push_back(block_id);
        }
    }
    return !reverse_handlers_.empty();
}

void Pass::link_instructions() {
    ValueSet operand_set;

    // Walk every exception handler block and construct the operand sets for all observe_assign instructions.
    // This is done by walking the reverse edges to the source blocks (which use the block as a handler) and
    // inspecting the current value(s) for a certain symbol - very similar to the classical phi node analysis.
    // In this case however, side effects are important, so all assignments within a block must be observed
    // to ensure that the exception handler sees the correct value of a symbol after an assignment happened.
    //
    // All publish_assign instructions that stay completely unreferenced by any observe_assign
    // instructions (these should be the vast majority in normal code) will be optimized out by the dead
    // code elimination pass.
    for (const auto& [handler_id, source_ids] : reverse_handlers_) {
        auto handler = func_[handler_id];
        for (auto inst_id : handler->insts()) {
            auto inst = func_[inst_id];
            if (inst->value().type() != ValueType::ObserveAssign) {
                continue;
            }

            auto obs = inst->value().as_observe_assign();
            TIRO_DEBUG_ASSERT(!obs.operands.valid(),
                "Operands must not have been assigned to this ObserveAssign instruction yet.");

            // Construct operand set
            operand_set.clear();
            for (auto source_id : source_ids) {
                TIRO_DEBUG_ASSERT(
                    func_[source_id]->handler() == handler_id, "Inconsistent block handler.");
                fill_operand_set(operand_set, source_id, obs.symbol);
            }

#if TIRO_DEBUG
            for (const auto op_inst_id : operand_set) {
                auto op_inst = func_[op_inst_id];
                TIRO_DEBUG_ASSERT(op_inst->value().type() == ValueType::PublishAssign,
                    "All operands must be publish_assign instructions.");
            }
#endif

            // Alter the existing instruction
            LocalList::Storage storage(operand_set.begin(), operand_set.end());
            obs.operands = func_.make(LocalList(std::move(storage)));
            inst->value(std::move(obs));
        }
    }
}

void Pass::fill_operand_set(ValueSet& operands, BlockId block_id, SymbolId symbol_id) {
    TIRO_DEBUG_ASSERT(block_id != func_.entry(), "Must not visit the entry block.");

    // Ensure initial value is available to the exception handler.
    {
        ValueList* pred_values = in_values(block_id, symbol_id);
        operands.insert(pred_values->begin(), pred_values->end());
    }

    // Simulate assignments in this block.
    auto block = func_[block_id];
    for (auto inst_id : block->insts()) {
        auto inst = func_[inst_id];
        switch (inst->value().type()) {
        case ValueType::PublishAssign: {
            auto& assign = inst->value().as_publish_assign();
            if (assign.symbol == symbol_id)
                operands.insert(inst_id);
            break;
        }
        default:
            break;
        }
    }
}

ValueList* Pass::in_values(BlockId block_id, SymbolId symbol_id) {
    if (auto it = in_values_.find({block_id, symbol_id}); it != in_values_.end()) {
        return &it->second;
    }

    auto write = [&](ValueList set) -> ValueList* {
        return &(in_values_[{block_id, symbol_id}] = std::move(set));
    };

    auto block = func_[block_id];

    // Must also take handler edges into account here.
    if (block->is_handler()) {
        const auto reverse_it = reverse_handlers_.find(block_id);
        TIRO_DEBUG_ASSERT(reverse_it != reverse_handlers_.end(),
            "There must be reverse edges for every handler block.");

        const auto& handler_preds = reverse_it->second;
        if (handler_preds.empty())
            return write({});
        if (handler_preds.size() == 1)
            return write(*out_values(handler_preds[0], symbol_id));

        ValueSet set;
        write({}); // Place a sentinel to stop recursion in control flow loops.
        for (const auto pred_id : reverse_it->second) {
            ValueList* pred_values = out_values(pred_id, symbol_id);
            set.insert(pred_values->begin(), pred_values->end());
        }
        return write(ValueList(set.begin(), set.end()));
    }

    // Not a handler, inspect normal predecessors.
    const auto pred_count = block->predecessor_count();
    if (pred_count == 0)
        return write({});
    if (pred_count == 1)
        return write(*out_values(block->predecessor(0), symbol_id));

    ValueSet set;
    write({}); // Place a sentinel to stop recursion in control flow loops.
    for (const auto pred_id : block->predecessors()) {
        ValueList* pred_values = out_values(pred_id, symbol_id);
        set.insert(pred_values->begin(), pred_values->end());
    }
    return write(ValueList(set.begin(), set.end()));
}

ValueList* Pass::out_values(BlockId block_id, SymbolId symbol_id) {
    if (auto it = out_values_.find({block_id, symbol_id}); it != out_values_.end()) {
        return &it->second;
    }

    auto write = [&](ValueList set) -> ValueList* {
        return &(out_values_[{block_id, symbol_id}] = std::move(set));
    };

    auto block = func_[block_id];

    // Simulate assignments to the symbol.
    for (const auto& inst_id : reverse_view(block->insts())) {
        auto inst = func_[inst_id];
        switch (inst->value().type()) {
        case ValueType::PublishAssign: {
            const auto& assign = inst->value().as_publish_assign();
            if (assign.symbol == symbol_id)
                return write({inst_id});
            break;
        }
        default:
            break;
        }
    }

    // No assignment found in this block, lookup in the predecessors.
    return write(*in_values(block_id, symbol_id));
}

void connect_assignment_observers(Function& func) {
    Pass pass(func);
    pass.run();
}

} // namespace tiro::ir
