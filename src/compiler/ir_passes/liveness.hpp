#ifndef TIRO_COMPILER_IR_PASSES_LIVENESS_HPP
#define TIRO_COMPILER_IR_PASSES_LIVENESS_HPP

#include "common/defs.hpp"
#include "common/entities/entity_storage.hpp"
#include "common/format.hpp"
#include "common/hash.hpp"
#include "common/ranges/iter_tools.hpp"
#include "compiler/ir/function.hpp"
#include "compiler/ir/fwd.hpp"

#include "absl/container/flat_hash_map.h"

namespace tiro::ir {

/// Represents an interval where a value is live.
struct LiveInterval {
    BlockId block;
    u32 start = 0;
    u32 end = 0;

    LiveInterval() = default;

    /// Constructs a new live interval.
    ///
    /// \param block
    ///     The block in which a value is live.
    ///
    /// \param start
    ///     The start index of the live interval. For intervals that represent
    ///     the definition of a value (i.e. the defining block), this is the statement index
    ///     of the defining statement. For other blocks, this is always 0 since the value
    ///     is live-in.
    ///
    /// \param end
    ///     The index of the last statement that that uses the value (within that block).
    LiveInterval(BlockId block_, u32 start_, u32 end_)
        : block(block_)
        , start(start_)
        , end(end_) {
        TIRO_DEBUG_ASSERT(start_ <= end_, "start must be <= end");
    }

    void format(FormatStream& stream) const;

    void hash(Hasher& h) const;
};

inline bool operator==(const LiveInterval& lhs, const LiveInterval& rhs) {
    return lhs.block == rhs.block && lhs.start == rhs.start && lhs.end == rhs.end;
}

inline bool operator!=(const LiveInterval& lhs, const LiveInterval& rhs) {
    return !(lhs == rhs);
}

/// Live range for a single SSA value.
///
/// A live range for a value is a collection of (non-overlapping) individual live intervals, where
/// every live interval is concerned with only a single IR basic block in which the
/// value is live.
///
/// Every SSA value has a single definition interval, which is the interval
/// starting at the unique definition program point and ending with the last use within
/// the defining block.
///
/// Other intervals in which the value is live (live-in) will always start at the beginning of the block.
///
/// This datastructure is designed to answer the following queries efficiently:
///     - Is the value live-in to a certain block?
///     - Will this value be referenced after the current program point?
///
/// Other queries are not needed by the current compilation process.
class LiveRange final {
public:
    /// Constructs a new live range for the given definition interval.
    /// When `def.start` == `def.end` is true, then the value is considered dead.
    explicit LiveRange(const LiveInterval& def);

    LiveRange(LiveRange&&) noexcept = default;
    LiveRange& operator=(LiveRange&&) noexcept = default;

    /// The definition block, statement and end statement. Same as the original constructor parameters.
    const LiveInterval& definition() const { return def_; }

    /// True if this value is never used.
    bool dead() const { return def_.start == def_.end; }

    /// Returns a sequence over all intervals where the value is live-in.
    auto live_in_intervals() const {
        return TransformView(range_view(live_in_), MapSmallInterval());
    }

    /// Returns true if the value is live-in in the given block.
    bool live_in(BlockId block) const;

    /// Returns true if the value is killed at the given statement index, i.e. if the statement
    /// is the last use of the value. Do not kill a value after the block's terminator.
    /// Values are recognized as dead in the block's successor(s) instead.
    ///
    /// \pre Value must be live in that block.
    bool last_use(BlockId block, u32 stmt) const;

    /// Extend the interval for the given `block` so that it reaches `stmt`.
    /// If `block` is not the defining block, than a new live-in interval will be created on demand,
    /// starting at statement index 0.
    ///
    /// Returns true if a new interval for that block was created, which means that the SSA value
    /// was recognized as a live-in value to that block for the first time.
    bool extend(BlockId block, u32 stmt);

private:
    using SmallInterval = std::pair<BlockId, u32>;

    std::pair<SmallInterval*, bool> ensure_interval(BlockId block);
    const SmallInterval* find_interval(BlockId block) const;

    struct MapSmallInterval {
        LiveInterval operator()(const SmallInterval& si) const {
            return LiveInterval(si.first, 0, si.second);
        }
    };

private:
    /// The defining interval.
    LiveInterval def_;

    // Sorted sequence of intervals, ordered by block id.
    // Could be compressed further by merging adjacent intervals. This would require
    // a consistent ordering of block ids.
    std::vector<SmallInterval> live_in_;
};

/// Contains liveness information for every variable in an IR function.
///
/// For the purpose of liveness information, references to aggregate members
/// are considered as references to the aggregate itself. This feels a bit hacky
/// but i cant think of another way to implement this cleanly right now.
///
/// Note that this implementation is heavily inspired by cranelift's internals, with some complexity stripped
/// because our use case is much simpler.
class Liveness final {
public:
    explicit Liveness(const Function& func);

    Liveness(Liveness&&) noexcept = default;
    Liveness& operator=(Liveness&&) noexcept = default;

    auto live_ranges() const { return range_view(live_ranges_); }

    auto live_in_values(BlockId block) const { return range_view(live_sets_[block]); }

    /// Returns the live range for `value`, or nullptr if none exists.
    const LiveRange* live_range(InstId value) const;

    /// Update liveness information.
    /// Invalidates all references and iterators.
    void compute();

    void format(FormatStream& stream) const;

private:
    using LiveRangeMap = absl::flat_hash_map<InstId, LiveRange, UseHasher>;

    // Values is live-out at the given block. Used for phi function arguments.
    void extend_live_out(InstId value, BlockId pred);

    // Extent the live range of the given value to the specified statement.
    void extent_statement(InstId value, BlockId block, u32 use);

    // Insert the initial definition of the given value.
    void insert_definition(InstId value, BlockId block, u32 start);

    // Dereference aggregate member reference to aggregate.
    InstId normalize(InstId value) const;

    bool is_aggregate_reference(InstId value) const;

private:
    NotNull<const Function*> func_;

    LiveRangeMap live_ranges_;

    EntityStorage<std::vector<InstId>, BlockId> live_sets_;

    // Worklist for liveness propagation to predecessors.
    std::vector<BlockId> work_;
};

} // namespace tiro::ir

TIRO_ENABLE_MEMBER_HASH(tiro::ir::LiveInterval)

TIRO_ENABLE_MEMBER_FORMAT(tiro::ir::LiveInterval)
TIRO_ENABLE_MEMBER_FORMAT(tiro::ir::Liveness)

#endif // TIRO_COMPILER_IR_PASSES_LIVENESS_HPP
