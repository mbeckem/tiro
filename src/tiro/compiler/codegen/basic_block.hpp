#ifndef TIRO_COMPILER_CODEGEN_BASIC_BLOCK_HPP
#define TIRO_COMPILER_CODEGEN_BASIC_BLOCK_HPP

#include "tiro/compiler/codegen/code_builder.hpp"
#include "tiro/compiler/string_table.hpp"
#include "tiro/core/defs.hpp"

#include <memory>
#include <vector>

namespace tiro::compiler {

class BasicBlock;

class BasicBlockEdge final {
public:
    enum class Which {
        None,     // No edge at all
        Jump,     // Unconditional edge
        CondJump, // Two edges: jump and "fall through"
        Ret,      // Return from function
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

    struct Ret {};

public:
    static BasicBlockEdge make_none();

    static BasicBlockEdge make_jump(BasicBlock* target);

    static BasicBlockEdge
    make_cond_jump(Opcode code, BasicBlock* target, BasicBlock* fallthrough);

    static BasicBlockEdge make_ret();

    Which which() const { return which_; }

    const None& none() const {
        TIRO_ASSERT(which_ == Which::None, "Invalid access.");
        return none_;
    }

    const Jump& jump() const {
        TIRO_ASSERT(which_ == Which::Jump, "Invalid access.");
        return jump_;
    }

    const CondJump& cond_jump() const {
        TIRO_ASSERT(which_ == Which::CondJump, "Invalid access.");
        return cond_jump_;
    }

    const Ret& ret() const {
        TIRO_ASSERT(which_ == Which::Ret, "Invalid access.");
        return ret_;
    }

private:
    BasicBlockEdge() = default;

private:
    Which which_;
    union {
        None none_;
        Jump jump_;
        CondJump cond_jump_;
        Ret ret_;
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
    BasicBlock(InternedString title);

    BasicBlock(const BasicBlock&) = delete;
    BasicBlock& operator=(const BasicBlock&) = delete;

    InternedString title() const { return title_; }
    CodeBuilder& builder() { return builder_; }

    const BasicBlockEdge& edge() const { return edge_; }
    void edge(const BasicBlockEdge& edge) { edge_ = edge; }

private:
    InternedString title_;
    BasicBlockEdge edge_ = BasicBlockEdge::make_none();
    std::vector<byte> code_; // Raw instructions (no jumps). Improvement: Typed.
    CodeBuilder builder_;    // Writes into code_.
};

// Improvement: Arena allocator for blocks and their instructions
class BasicBlockStorage final {
public:
    BasicBlockStorage();
    ~BasicBlockStorage();

    /// Constructs a new basic block with the given title. The address of that block remains stable.
    /// The block will live until this storage object is either destroyed or until reset() has been called.
    BasicBlock* make_block(InternedString title);

    /// Destroys all blocks created by this instance.
    void reset();

    BasicBlockStorage(const BasicBlockStorage&) = delete;
    BasicBlockStorage& operator=(const BasicBlockStorage&) = delete;

private:
    std::vector<std::unique_ptr<BasicBlock>> blocks_;
};

} // namespace tiro::compiler

#endif // TIRO_COMPILER_CODEGEN_BASIC_BLOCK_HPP
