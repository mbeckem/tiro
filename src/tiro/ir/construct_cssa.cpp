#include "tiro/ir/construct_cssa.hpp"

#include "tiro/ir/function.hpp"
#include "tiro/ir/traversal.hpp"

namespace tiro {

namespace {

class CSSAConstructor final {
public:
    explicit CSSAConstructor(Function& func)
        : func_(func) {}

    bool run();

    bool visit_block(BlockId block_id);

    bool lift_phi(IndexMapPtr<Block> block, Stmt& phi_def, std::vector<Stmt>& new_stmts);

private:
    Function& func_;
    std::vector<Stmt> stmt_buffer_; // TODO small vec
};

} // namespace

bool CSSAConstructor::run() {
    bool changed = false;
    for (const BlockId block_id : PreorderTraversal(func_)) {
        changed |= visit_block(block_id);
    }
    return changed;
}

bool CSSAConstructor::visit_block(BlockId block_id) {
    auto block = func_[block_id];
    bool changed = true;

    std::vector<Stmt>& new_stmts = stmt_buffer_;
    new_stmts.clear();

    // The loop terminates with "pos" pointing to the first non-phi stmt
    // or the end of the stmt vector.
    auto& stmts = block->raw_stmts();
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

bool CSSAConstructor::lift_phi(
    IndexMapPtr<Block> block, Stmt& phi_def, std::vector<Stmt>& new_stmts) {
    const auto original_local = phi_def.as_define().local;
    const auto rvalue = func_[original_local]->value();
    if (rvalue.type() != RValueType::Phi)
        return false;

    const auto phi = func_[rvalue.as_phi().value];
    TIRO_DEBUG_ASSERT(phi->operand_count() == block->predecessor_count(),
        "Argument mismatch between the number of phi arguments and the number "
        "of predecessors.");

    // Insert a new variable definition at the end of every predecessor block
    // and swap the variable names within the phi function.
    const size_t args = phi->operand_count();
    for (size_t i = 0; i < args; ++i) {
        auto operand_id = phi->operand(i);
        auto pred = func_[block->predecessor(i)];
        TIRO_CHECK(target_count(pred->terminator()) < 2,
            "Critical edge encountered during CSSA construction.");

        auto new_operand = func_.make(Local(RValue::make_use_local(operand_id)));
        pred->append_stmt(Stmt::make_define(new_operand));
        phi->operand(i, new_operand);
    }

    // Replace the left hand side of the phi function as well.
    // The new local inherits the position and phi operand list of the original one.
    // The original local is defined as a usage stmt after the block of phi nodes.
    // This approach has the advantage that we do not have to update any usages that refer
    // to the original local.
    auto new_local = func_.make(Local(rvalue));
    phi_def = Stmt::make_define(new_local);
    func_[original_local]->value(RValue::make_use_local(new_local));
    new_stmts.push_back(Stmt::make_define(original_local));
    return true;
}

bool construct_cssa(Function& func) {
    CSSAConstructor cssa(func);
    return cssa.run();
}

} // namespace tiro
