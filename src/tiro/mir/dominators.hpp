#ifndef TIRO_MIR_DOMINATORS_HPP
#define TIRO_MIR_DOMINATORS_HPP

#include "tiro/core/format.hpp"
#include "tiro/core/hash.hpp"
#include "tiro/core/index_map.hpp"
#include "tiro/core/not_null.hpp"
#include "tiro/mir/fwd.hpp"
#include "tiro/mir/types.hpp"

namespace tiro::compiler {

class DominatorTree final {
public:
    DominatorTree(const mir::Function& func);
    ~DominatorTree();

    DominatorTree(DominatorTree&&) noexcept = default;
    DominatorTree& operator=(DominatorTree&&) noexcept = default;

    /// Computes the dominator tree with the current state of the function's cfg.
    void compute();

    /// Returns the immediate dominator for the given node.
    /// Note that the root node's immediate dominator is itself.
    mir::BlockID immediate_dominator(mir::BlockID node) const;

    auto immediately_dominated(mir::BlockID parent) const {
        return range_view(get(parent)->children);
    }

    /// Returns true iff `parent` is a dominator of `child`.
    /// Note that blocks always dominate themselves.
    bool dominates(mir::BlockID parent, mir::BlockID child) const;

    /// Returns true iff `parent` strictly dominates the child, i.e. iff
    /// `parent != child && dominates(parent, child)`.
    bool dominates_strict(mir::BlockID parent, mir::BlockID child) const {
        return parent != child && dominates(parent, child);
    }

    void format(FormatStream& stream) const;

private:
    struct Entry {
        // The immediate dominator. Invalid id if unreachable. Same id if root.
        mir::BlockID idom;

        // The immediately dominated children (children[i].parent == this).
        // TODO: Small vec optimization, the # of children is usually small.
        std::vector<mir::BlockID> children;
    };

    // Used for reverse post order rank numbers.
    using RankMap = IndexMap<size_t, IDMapper<mir::BlockID>>;

    // Used to store entries for every block.
    using EntryMap = IndexMap<Entry, IDMapper<mir::BlockID>>;

    const Entry* get(mir::BlockID block) const;

    static void compute_tree(const mir::Function& func, EntryMap& entries);

    static mir::BlockID intersect(const RankMap& ranks, const EntryMap& entries,
        mir::BlockID b1, mir::BlockID b2);

private:
    NotNull<const mir::Function*> func_;
    mir::BlockID root_;
    EntryMap entries_;
};

} // namespace tiro::compiler

TIRO_ENABLE_MEMBER_FORMAT(tiro::compiler::DominatorTree)

#endif // TIRO_MIR_DOMINATORS_HPP
