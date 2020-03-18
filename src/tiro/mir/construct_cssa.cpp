#include "tiro/mir/construct_cssa.hpp"

#include "tiro/mir/traversal.hpp"
#include "tiro/mir/types.hpp"

namespace tiro {

namespace {

class CSSAConstructor final {
public:
    explicit CSSAConstructor(mir::Function& func)
        : func_(func) {}

    bool run();

    bool visit_block(mir::BlockID block_id);

    bool lift_phi(IndexMapPtr<mir::Block> block, mir::Stmt& phi_def,
        std::vector<mir::Stmt>& new_stmts);

private:
    mir::Function& func_;
    std::vector<mir::Stmt> stmt_buffer_; // TODO small vec
};

} // namespace

bool CSSAConstructor::run() {
    bool changed = false;
    for (const mir::BlockID block_id : mir::PreorderTraversal(func_)) {
        changed |= visit_block(block_id);
    }
    return changed;
}

bool CSSAConstructor::visit_block(mir::BlockID block_id) {
    auto block = func_[block_id];
    bool changed = true;

    std::vector<mir::Stmt>& new_stmts = stmt_buffer_;
    new_stmts.clear();

    // The loop terminates with "pos" pointing to the first non-phi stmt
    // or the end of the stmt vector.
    auto& stmts = block->raw_stmts();
    auto pos = stmts.begin();
    auto end = stmts.end();
    for (; pos != end; ++pos) {
        if (!mir::is_phi_define(func_, *pos))
            break; // Phi nodes cluster at the start

        changed |= lift_phi(block, *pos, new_stmts);
    }

    stmts.insert(pos, new_stmts.begin(), new_stmts.end());
    return changed;
}

bool CSSAConstructor::lift_phi(IndexMapPtr<mir::Block> block,
    mir::Stmt& phi_def, std::vector<mir::Stmt>& new_stmts) {
    const auto original_local = phi_def.as_define().local;
    const auto rvalue = func_[original_local]->value();
    if (rvalue.type() != mir::RValueType::Phi)
        return false;

    const auto phi = func_[rvalue.as_phi().value];
    TIRO_ASSERT(phi->operand_count() == block->predecessor_count(),
        "Argument mismatch between the number of phi arguments and the number "
        "of predecessors.");

    // Insert a new variable definition at the end of every predecessor block
    // and swap the variable names within the phi function.
    const size_t args = phi->operand_count();
    for (size_t i = 0; i < args; ++i) {
        auto operand_id = phi->operand(i);
        auto pred = func_[block->predecessor(i)];

        auto new_operand = func_.make(
            mir::Local(mir::RValue::make_use_local(operand_id)));
        pred->append_stmt(mir::Stmt::make_define(new_operand));
        phi->operand(i, new_operand);
    }

    // Replace the left hand side of the phi function as well.
    // The new local inherits the position and phi operand list of the original one.
    // The original local is defined as a usage stmt after the block of phi nodes.
    // This approach has the advantage that we do not have to update any usages that refer
    // to the original local.
    auto new_local = func_.make(mir::Local(rvalue));
    phi_def = mir::Stmt::make_define(new_local);
    func_[original_local]->value(mir::RValue::make_use_local(new_local));
    new_stmts.push_back(mir::Stmt::make_define(original_local));
    return true;
}

bool construct_cssa(mir::Function& func) {
    CSSAConstructor cssa(func);
    return cssa.run();
}

} // namespace tiro
