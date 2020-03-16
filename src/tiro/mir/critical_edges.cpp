#include "tiro/mir/critical_edges.hpp"

#include "tiro/mir/traversal.hpp"
#include "tiro/mir/types.hpp"

#include <optional>

namespace tiro::compiler {

// Source has multiple successors.
// If the target has multiple predecessors, then this edge must be split.
static std::optional<mir::BlockID> maybe_split(
    mir::Function& func, mir::BlockID source_id, mir::BlockID target_id) {
    const auto target = func[target_id];
    if (target->predecessor_count() <= 1)
        return {};

    auto split_id = func.make(mir::Block(func.strings().insert("split-edge")));
    auto split = func[split_id];
    split->append_predecessor(source_id);
    split->terminator(mir::Terminator::make_jump(target_id));

    target->replace_predecessor(source_id, split_id);
    return split_id;
}

static bool visit_block(
    mir::Function& func, mir::BlockID block_id, IndexMapPtr<mir::Block> block) {

    // Edges can only be critical for the "branch" terminator. This is a switch instead
    // of a simple if type check so we can't forget to update it should we introduce switch terminators.
    const auto term = block->terminator();
    switch (term.type()) {

    // These terminators have 0 or 1 successors.
    case mir::TerminatorType::None:
    case mir::TerminatorType::Jump:
    case mir::TerminatorType::Return:
    case mir::TerminatorType::Exit:
    case mir::TerminatorType::AssertFail:
    case mir::TerminatorType::Never:
        return false;

    case mir::TerminatorType::Branch: {
        auto branch = term.as_branch();
        if (branch.target == branch.fallthrough)
            return false;

        auto new_target = maybe_split(func, block_id, branch.target);
        if (new_target)
            branch.target = *new_target;

        auto new_fallthrough = maybe_split(func, block_id, branch.fallthrough);
        if (new_fallthrough)
            branch.fallthrough = *new_fallthrough;

        if (new_target || new_fallthrough) {
            block->terminator(branch);
            return true;
        }
        return false;
    }
    }

    TIRO_UNREACHABLE("Invalid block terminator type.");
}

bool split_critical_edges(mir::Function& func) {
    bool changed = false;

    for (const mir::BlockID block_id : mir::PreorderTraversal(func)) {
        const auto block = func[block_id];
        changed |= visit_block(func, block_id, block);
    }
    return changed;
}

} // namespace tiro::compiler
