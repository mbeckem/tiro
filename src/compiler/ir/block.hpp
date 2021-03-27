#ifndef TIRO_COMPILER_IR_BLOCK_HPP
#define TIRO_COMPILER_IR_BLOCK_HPP

#include "common/adt/span.hpp"
#include "common/assert.hpp"
#include "common/defs.hpp"
#include "common/enum_flags.hpp"
#include "common/format.hpp"
#include "common/text/string_table.hpp"
#include "compiler/ir/entities.hpp"
#include "compiler/ir/fwd.hpp"
#include "compiler/ir/terminator.hpp"

namespace tiro::ir {

/// Represents a single basic block in the control flow graph of a function.
///
/// A block contains a simple sequence of statements. The list of statements
/// does not contain inner control flow: if the basic block is entered, its complete
/// sequence of statements will be executed.
///
/// Blocks are connected by incoming and outgoing edges. These model the control flow,
/// including branches, jumps and returns.
///
/// When an instruction in a block raises an exception, control flow moves
/// to the start of the given handler block if one was defined. Otherwise, the function
/// rethrows the exception to its parent.
///
/// The handler edge must not be traversed when the cfg is visited for normal control flow,
/// because handler blocks have an implicit in-edge from outside the function (they
/// are called by the runtime). For example, handler blocks cannot use phi nodes
/// to access values from normal control flow before the exception was raised.
///
/// The initial "entry" block and handler blocks of a functions do not have any incoming edges,
/// and only the final "exit" block has an outgoing return edge.
class Block final {
public:
    explicit Block(InternedString label);
    ~Block();

    Block(Block&&) noexcept;
    Block& operator=(Block&&);

    // Prevent accidental copying.
    Block(const Block&) = delete;
    Block& operator=(const Block&) = delete;

    InternedString label() const { return label_; }
    void label(InternedString label) { label_ = label; }

    /// A sealed block no longer accepts incoming edges.
    void sealed(bool is_sealed) { flags_.set(IsSealed, is_sealed); }
    bool sealed() const { return flags_.test(IsSealed); }

    /// A filled block no longer accepts additional statements.
    void filled(bool is_filled) { flags_.set(IsFilled, is_filled); }
    bool filled() const { return flags_.test(IsFilled); }

    /// A block with `is_handler() == true` is an entry block of exceptional control flow.
    /// It must not have any predecessors, except for the entry block (the only incoming edge is virtual and comes from outside the function).
    void is_handler(bool is_handler) { flags_.set(IsHandler, is_handler); }
    bool is_handler() const { return flags_.test(IsHandler); }

    // The out-edge(s) for this block under normal (non exceptional) circumstances.
    const Terminator& terminator() const { return term_; }
    Terminator& terminator() { return term_; }
    void terminator(Terminator&& term) { term_ = std::move(term); }

    /// The exception handler out edge for this block. May not be present.
    void handler(BlockId handler_id) { handler_ = handler_id; }
    BlockId handler() const { return handler_; }

    /// The in edges for this block.
    auto predecessors() const { return range_view(predecessors_); }

    BlockId predecessor(size_t index) const;
    size_t predecessor_count() const;
    void append_predecessor(BlockId predecessor);
    void replace_predecessor(BlockId old_pred, BlockId new_pred);

    auto insts() const { return range_view(insts_); }

    InstId inst(size_t index) const;
    size_t inst_count() const;

    void insert_inst(size_t index, InstId inst);
    void insert_insts(size_t index, Span<const InstId> insts);
    void append_inst(InstId inst);

    /// Returns the number of phi nodes at the beginning of the block.
    size_t phi_count(const Function& parent) const;

    /// Called to transform a phi function into a normal value.
    /// This function will apply the new value and move the definition after the other phi functions
    /// to ensure that phis remain clustered at the start of the block.
    void remove_phi(Function& parent, InstId inst, Value&& new_value);

    auto& raw_insts() { return insts_; }

    /// Removes all statements from this block for which the given predicate
    /// returns true.
    template<typename Pred>
    void remove_insts(Pred&& pred) {
        auto rem = std::remove_if(insts_.begin(), insts_.end(), pred);
        insts_.erase(rem, insts_.end());
    }

    void format(FormatStream& stream) const;

private:
    enum Props : u32 {
        IsSealed = 1 << 0,
        IsFilled = 1 << 1,
        IsHandler = 1 << 2,
    };

private:
    InternedString label_;
    Flags<Props> flags_;
    Terminator term_ = Terminator::None{};
    absl::InlinedVector<BlockId, 4> predecessors_;
    absl::InlinedVector<InstId, 6> insts_;
    BlockId handler_;
};

} // namespace tiro::ir

TIRO_ENABLE_MEMBER_FORMAT(tiro::ir::Block)

#endif // TIRO_COMPILER_IR_BLOCK_HPP
