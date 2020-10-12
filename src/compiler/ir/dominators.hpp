#ifndef TIRO_COMPILER_IR_DOMINATORS_HPP
#define TIRO_COMPILER_IR_DOMINATORS_HPP

#include "common/adt/index_map.hpp"
#include "common/adt/not_null.hpp"
#include "common/format.hpp"
#include "common/hash.hpp"
#include "compiler/ir/function.hpp"
#include "compiler/ir/fwd.hpp"

namespace tiro {

class DominatorTree final {
public:
    DominatorTree(const Function& func);
    ~DominatorTree();

    DominatorTree(DominatorTree&&) noexcept = default;
    DominatorTree& operator=(DominatorTree&&) noexcept = default;

    /// Computes the dominator tree with the current state of the function's cfg.
    void compute();

    /// Returns the immediate dominator for the given node.
    /// Note that the root node's immediate dominator is itself.
    BlockId immediate_dominator(BlockId node) const;

    auto immediately_dominated(BlockId parent) const { return range_view(get(parent)->children); }

    /// Returns true iff `parent` is a dominator of `child`.
    /// Note that blocks always dominate themselves.
    bool dominates(BlockId parent, BlockId child) const;

    /// Returns true iff `parent` strictly dominates the child, i.e. iff
    /// `parent != child && dominates(parent, child)`.
    bool dominates_strict(BlockId parent, BlockId child) const {
        return parent != child && dominates(parent, child);
    }

    void format(FormatStream& stream) const;

private:
    struct Entry {
        // The immediate dominator. Invalid id if unreachable. Same id if root.
        BlockId idom;

        // The immediately dominated children (children[i].parent == this).
        // TODO: Small vec optimization, the # of children is usually small.
        std::vector<BlockId> children;
    };

    // Used for reverse post order rank numbers.
    using RankMap = IndexMap<size_t, IdMapper<BlockId>>;

    // Used to store entries for every block.
    using EntryMap = IndexMap<Entry, IdMapper<BlockId>>;

    const Entry* get(BlockId block) const;

    static void compute_tree(const Function& func, EntryMap& entries);

    static BlockId intersect(const RankMap& ranks, const EntryMap& entries, BlockId b1, BlockId b2);

private:
    NotNull<const Function*> func_;
    BlockId root_;
    EntryMap entries_;
};

} // namespace tiro

TIRO_ENABLE_MEMBER_FORMAT(tiro::DominatorTree)

#endif // TIRO_COMPILER_IR_DOMINATORS_HPP
