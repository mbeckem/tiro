#include "compiler/ir_passes/construct_cssa.hpp"

#include "common/error.hpp"
#include "compiler/ir/function.hpp"
#include "compiler/ir/traversal.hpp"

namespace tiro::ir {

namespace {

class CSSABuilder final {
public:
    explicit CSSABuilder(Function& func)
        : func_(func) {
        stmt_buffer_.reserve(32);
    }

    bool run();

    bool visit_block(BlockId block_id);

    bool lift_phi(NotNull<EntityPtr<Block>> block, InstId& phi_def, std::vector<InstId>& new_stmts);

private:
    Function& func_;
    std::vector<InstId> stmt_buffer_;
};

} // namespace

bool CSSABuilder::run() {
    bool changed = false;
    for (const BlockId block_id : PreorderTraversal(func_)) {
        changed |= visit_block(block_id);
    }
    return changed;
}

bool CSSABuilder::visit_block(BlockId block_id) {
    auto block = func_.ptr_to(block_id);
    bool changed = true;

    std::vector<InstId>& new_stmts = stmt_buffer_;
    new_stmts.clear();

    // The loop terminates with "pos" pointing to the first non-phi stmt
    // or the end of the stmt vector.
    auto& stmts = block->raw_insts();
    auto pos = stmts.begin();
    auto end = stmts.end();
    for (; pos != end; ++pos) {
        if (!is_phi_define(func_, *pos))
            break; // Phi nodes cluster at the start

        changed |= lift_phi(block, *pos, new_stmts);
    }

    stmts.insert(pos, new_stmts.begin(), new_stmts.end());
    return changed;
}

bool CSSABuilder::lift_phi(
    NotNull<EntityPtr<Block>> block, InstId& phi_def, std::vector<InstId>& new_stmts) {
    const auto original_inst = phi_def;

    auto& original_value = func_[original_inst].value();
    if (original_value.type() != ValueType::Phi)
        return false;

    auto& original_phi = original_value.as_phi();
    TIRO_DEBUG_ASSERT(original_phi.operand_count(func_) == block->predecessor_count(),
        "Argument mismatch between the number of phi arguments and the number "
        "of predecessors.");

    // Insert a new variable definition at the end of every predecessor block
    // and swap the variable names within the phi function.
    const size_t args = original_phi.operand_count(func_);
    for (size_t i = 0; i < args; ++i) {
        auto operand_id = original_phi.operand(func_, i);
        auto& pred = func_[block->predecessor(i)];
        TIRO_CHECK(target_count(pred.terminator()) < 2,
            "Critical edge encountered during CSSA construction.");

        auto new_operand = func_.make(Inst(Value::make_alias(operand_id)));
        pred.append_inst(new_operand);
        original_phi.operand(func_, i, new_operand);
    }

    // Replace the left hand side of the phi function as well.
    // The new instruction inherits the position and phi operand list of the original one.
    // The original instruction is defined as an alias after the block of phi nodes.
    // This approach has the advantage that we do not have to update any usages that refer
    // to the original instruction.
    auto new_inst = func_.make(Inst(std::move(original_value)));
    phi_def = new_inst;
    func_[original_inst].value(Value::make_alias(new_inst));
    new_stmts.push_back(original_inst);
    return true;
}

bool construct_cssa(Function& func) {
    CSSABuilder cssa(func);
    return cssa.run();
}

} // namespace tiro::ir
