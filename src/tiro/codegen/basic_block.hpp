#ifndef TIRO_CODEGEN_BASIC_BLOCK_HPP
#define TIRO_CODEGEN_BASIC_BLOCK_HPP

#include "tiro/codegen/instructions.hpp"
#include "tiro/compiler/string_table.hpp"
#include "tiro/core/defs.hpp"
#include "tiro/core/not_null.hpp"
#include "tiro/core/span.hpp"

#include <memory>
#include <vector>

namespace tiro::compiler {

class BasicBlock;

// TODO: NotNull-ify

class BasicBlockEdge final {
public:
    enum class Which {
        None,       // No edge at all
        Jump,       // Unconditional edge
        CondJump,   // Two edges: jump and "fall through"
        AssertFail, // Assertion failure
        Never,      // Never returns from this block
        Ret,        // Return from function
        // Throw TODO
    };

    struct None {};

    struct Jump {
        BasicBlock* target;
    };

    struct CondJump {
        Opcode code;
        BasicBlock* target;
        BasicBlock* fallthrough;
    };

public:
    static BasicBlockEdge make_none();

    static BasicBlockEdge make_jump(BasicBlock* target);

    static BasicBlockEdge
    make_cond_jump(Opcode code, BasicBlock* target, BasicBlock* fallthrough);

    static BasicBlockEdge make_assert_fail();

    static BasicBlockEdge make_never();

    static BasicBlockEdge make_ret();

    Which which() const { return which_; }

    const Jump& jump() const {
        TIRO_ASSERT(which_ == Which::Jump, "Invalid access.");
        return jump_;
    }

    const CondJump& cond_jump() const {
        TIRO_ASSERT(which_ == Which::CondJump, "Invalid access.");
        return cond_jump_;
    }

private:
    BasicBlockEdge() = default;

private:
    Which which_;
    union {
        Jump jump_;
        CondJump cond_jump_;
    };
};

std::string_view to_string(BasicBlockEdge::Which which);

/// A basic block is a sequence of instructions.
/// Only jumps to the start of of a basic block (through incoming edges)
/// or from the end of a basic block (through outgoing edges) are allowed.
/// The body of a block is a linear unit of execution.
///
/// TODO: It is currently possible to add jumps through builder().
///
/// TODO: These are currently unused. Rewrite codegen.
///
/// Improvement: efficiency.
class BasicBlock final {
public:
    explicit BasicBlock(InternedString title);

    BasicBlock(const BasicBlock&) = delete;
    BasicBlock& operator=(const BasicBlock&) = delete;

    InternedString title() const { return title_; }

    Span<const NotNull<Instruction*>> code() const { return code_; }

    void append(NotNull<Instruction*> instr) { code_.push_back(instr); }

    const BasicBlockEdge& edge() const { return edge_; }
    void edge(const BasicBlockEdge& edge) { edge_ = edge; }

private:
    InternedString title_;

    // Outgoing edge to the next block(s).
    BasicBlockEdge edge_ = BasicBlockEdge::make_none();

    // Raw instructions (no jumps).
    // TODO: Small vec
    std::vector<NotNull<Instruction*>> code_;
};

/**
 * Stores a pointer to the currently active basic block to make argument passing
 * more convenient and less error prone. The current basic block
 * can be changed using `assign()`.
 * 
 * CurrentBasicBlock instance should be passed via reference.
 */
class CurrentBasicBlock {
public:
    CurrentBasicBlock() = delete;

    explicit CurrentBasicBlock(NotNull<BasicBlock*> initial)
        : block_(initial) {}

    CurrentBasicBlock(const CurrentBasicBlock&) = delete;
    CurrentBasicBlock& operator=(const CurrentBasicBlock&) = delete;

    void assign(NotNull<BasicBlock*> block) { block_ = block; }

    BasicBlock* get() const noexcept { return block_; }
    BasicBlock* operator->() const noexcept { return block_; }
    BasicBlock& operator*() const noexcept { return *block_; }

private:
    BasicBlock* block_ = nullptr;
};

// Improvement: Arena allocator for blocks and their instructions
class BasicBlockStorage final {
public:
    BasicBlockStorage();
    ~BasicBlockStorage();

    /// Constructs a new basic block with the given title. The address of that block remains stable.
    /// The block will live until this storage object is either destroyed or until reset() has been called.
    NotNull<BasicBlock*> make_block(InternedString title);

    /// Destroys all blocks created by this instance.
    void reset();

    BasicBlockStorage(const BasicBlockStorage&) = delete;
    BasicBlockStorage& operator=(const BasicBlockStorage&) = delete;

private:
    std::vector<std::unique_ptr<BasicBlock>> blocks_;
};

} // namespace tiro::compiler

#endif // TIRO_CODEGEN_BASIC_BLOCK_HPP
