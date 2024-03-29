#include "compiler/ir_passes/critical_edges.hpp"

#include "compiler/ir/function.hpp"
#include "compiler/ir/traversal.hpp"

#include <optional>

namespace tiro::ir {

// Source has multiple successors.
// If the target has multiple predecessors, then this edge must be split.
static std::optional<BlockId> maybe_split(Function& func, BlockId source_id, BlockId target_id) {
    auto target = func.ptr_to(target_id);
    if (target->predecessor_count() <= 1)
        return {};

    auto split_id = func.make(Block(func.strings().insert("split-edge")));
    auto split = func.ptr_to(split_id);
    split->append_predecessor(source_id);
    split->terminator(Terminator::make_jump(target_id));

    target->replace_predecessor(source_id, split_id);
    return split_id;
}

static bool visit_block(Function& func, BlockId block_id, NotNull<EntityPtr<Block>> block) {

    // Edges can only be critical for the "branch" terminator. This is a switch instead
    // of a simple if type check so we can't forget to update it should we introduce switch terminators.
    switch (block->terminator().type()) {

    // These terminators have 0 or 1 successors.
    case TerminatorType::None:
    case TerminatorType::Never:
    case TerminatorType::Jump:
    case TerminatorType::Return:
    case TerminatorType::Exit:
    case TerminatorType::Rethrow:
    case TerminatorType::AssertFail:
        return false;

    // May have N edges but these are all virtual.
    case TerminatorType::Entry:
        return false;

    case TerminatorType::Branch: {
        auto branch = block->terminator().as_branch();
        if (branch.target == branch.fallthrough)
            return false;

        auto new_target = maybe_split(func, block_id, branch.target);
        if (new_target)
            branch.target = *new_target;

        auto new_fallthrough = maybe_split(func, block_id, branch.fallthrough);
        if (new_fallthrough)
            branch.fallthrough = *new_fallthrough;

        if (new_target || new_fallthrough) {
            block->terminator(std::move(branch));
            return true;
        }
        return false;
    }
    }

    TIRO_UNREACHABLE("Invalid block terminator type.");
}

bool split_critical_edges(Function& func) {
    bool changed = false;

    for (const BlockId block_id : PreorderTraversal(func)) {
        const auto block = func.ptr_to(block_id);
        changed |= visit_block(func, block_id, block);
    }
    return changed;
}

} // namespace tiro::ir
