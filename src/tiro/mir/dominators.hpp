#ifndef TIRO_MIR_DOMINATORS_HPP
#define TIRO_MIR_DOMINATORS_HPP

#include "tiro/core/format.hpp"
#include "tiro/core/hash.hpp"
#include "tiro/core/index_map.hpp"
#include "tiro/core/not_null.hpp"
#include "tiro/mir/fwd.hpp"
#include "tiro/mir/types.hpp"

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
    BlockID immediate_dominator(BlockID node) const;

    auto immediately_dominated(BlockID parent) const {
        return range_view(get(parent)->children);
    }

    /// Returns true iff `parent` is a dominator of `child`.
    /// Note that blocks always dominate themselves.
    bool dominates(BlockID parent, BlockID child) const;

    /// Returns true iff `parent` strictly dominates the child, i.e. iff
    /// `parent != child && dominates(parent, child)`.
    bool dominates_strict(BlockID parent, BlockID child) const {
        return parent != child && dominates(parent, child);
    }

    void format(FormatStream& stream) const;

private:
    struct Entry {
        // The immediate dominator. Invalid id if unreachable. Same id if root.
        BlockID idom;

        // The immediately dominated children (children[i].parent == this).
        // TODO: Small vec optimization, the # of children is usually small.
        std::vector<BlockID> children;
    };

    // Used for reverse post order rank numbers.
    using RankMap = IndexMap<size_t, IDMapper<BlockID>>;

    // Used to store entries for every block.
    using EntryMap = IndexMap<Entry, IDMapper<BlockID>>;

    const Entry* get(BlockID block) const;

    static void compute_tree(const Function& func, EntryMap& entries);

    static BlockID intersect(
        const RankMap& ranks, const EntryMap& entries, BlockID b1, BlockID b2);

private:
    NotNull<const Function*> func_;
    BlockID root_;
    EntryMap entries_;
};

} // namespace tiro

TIRO_ENABLE_MEMBER_FORMAT(tiro::DominatorTree)

#endif // TIRO_MIR_DOMINATORS_HPP
