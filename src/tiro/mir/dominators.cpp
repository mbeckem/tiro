#include "tiro/mir/dominators.hpp"

#include "tiro/compiler/utils.hpp"
#include "tiro/core/fix.hpp"
#include "tiro/mir/traversal.hpp"

namespace tiro {

DominatorTree::DominatorTree(const Function& func)
    : func_(func) {}

DominatorTree::~DominatorTree() {}

void DominatorTree::compute() {
    root_ = func_->entry();
    compute_tree(*func_, entries_);
}

BlockID DominatorTree::immediate_dominator(BlockID node) const {
    return get(node)->idom;
}

bool DominatorTree::dominates(BlockID parent, BlockID child) const {
    TIRO_ASSERT(parent, "Parent must be a valid block id.");
    TIRO_ASSERT(child, "Child must be a valid block id.");

    while (1) {
        if (parent == child)
            return true;

        auto entry = get(child);
        if (entry->idom == child)
            return false;

        child = entry->idom;
    }
}

void DominatorTree::format(FormatStream& stream) const {
    if (!root_) {
        stream.format("<Empty dominator tree>");
        return;
    }

    Fix to_tree = [&](auto&& self, BlockID node) -> StringTree {
        const auto* entry = get(node);

        StringTree tree;
        tree.line = fmt::format(
            "{}", dump_helpers::DumpBlock{*func_, node});

        for (auto child : entry->children) {
            tree.children.push_back(self(child));
        }

        return tree;
    };

    auto tree = to_tree(root_);
    stream.format("Dominator tree:\n{}", format_tree(tree));
}

const DominatorTree::Entry* DominatorTree::get(BlockID block) const {
    TIRO_ASSERT(block, "Block id must be valid.");
    TIRO_ASSERT(entries_.in_bounds(block),
        "Block index is out of bounds. Tree outdated?");

    const auto& entry = entries_[block];
    TIRO_ASSERT(entry.idom, "Block is unreachable. Tree outdated?");
    return &entry;
}

// Returns a mapping from BlockID -> post order rank, i.e. the root has the highest rank.
static IndexMap<size_t, IDMapper<BlockID>> postorder_ranks(
    const Function& func, const ReversePostorderTraversal& rpo) {
    IndexMap<size_t, IDMapper<BlockID>> ranks;
    ranks.resize(func.block_count());

    size_t n = rpo.size();
    for (auto block_id : rpo) {
        ranks[block_id] = --n;
    }
    return ranks;
}

// [CKH+06] Cooper, Keith & Harvey, Timothy & Kennedy, Ken. (2006):
//              A Simple, Fast Dominance Algorithm.
//              Rice University, CS Technical Report 06-33870.
void DominatorTree::compute_tree(const Function& func, EntryMap& entries) {
    auto root = func.entry();
    auto rpo = ReversePostorderTraversal(func);
    auto ranks = postorder_ranks(func, rpo);

    TIRO_ASSERT(rpo.begin() != rpo.end(),
        "Reverse postorder must not be empty (contains entry block).");
    TIRO_ASSERT(*rpo.begin() == root,
        "First entry in reverse postorder must be the entry block.");
    auto rpo_without_root = IterRange(rpo.begin() + 1, rpo.end());

    // [CKH+06] Figure 3
    // Compute immediate dominators for every node
    entries.reset(func.block_count());
    entries[root].idom = root;

    bool changed = true;
    while (changed) {
        changed = false;

        for (auto block_id : rpo_without_root) {
            auto block = func[block_id];

            BlockID new_idom;
            for (auto pred : block->predecessors()) {
                if (entries[pred].idom) {
                    new_idom = intersect(ranks, entries, pred, new_idom);
                }
            }

            if (new_idom != entries[block_id].idom) {
                entries[block_id].idom = new_idom;
                changed = true;
            }
        }
    }

    // Assemble parent -> child links for top -> down traversal
    for (auto block_id : rpo_without_root) {
        auto idom = entries[block_id].idom;
        entries[idom].children.push_back(block_id);
    }
}

BlockID DominatorTree::intersect(const RankMap& ranks,
    const EntryMap& entries, BlockID b1, BlockID b2) {
    // Propagate valid ids if one of (b1, b2) is invalid.
    if (!b1 || !b2)
        return b1 ? b1 : b2;

    while (b1 != b2) {
        while (ranks[b1] < ranks[b2]) {
            b1 = entries[b1].idom;
        }
        while (ranks[b2] < ranks[b1]) {
            b2 = entries[b2].idom;
        }
    }
    return b1;
}

} // namespace tiro
