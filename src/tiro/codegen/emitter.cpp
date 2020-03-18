#include "tiro/codegen/emitter.hpp"

#include "tiro/codegen/basic_block.hpp"
#include "tiro/codegen/instructions.hpp"

namespace tiro {

void emit_code(NotNull<BasicBlock*> start, std::vector<byte>& out) {
    CodeBuilder builder(out);

    std::vector<NotNull<BasicBlock*>> stack;
    std::unordered_set<BasicBlock*> seen_blocks;

    auto seen = [&](BasicBlock* block) { return seen_blocks.count(block); };

    auto push = [&](NotNull<BasicBlock*> next) {
        auto inserted = seen_blocks.emplace(next).second;
        if (inserted) {
            stack.push_back(next);
        }
    };

    push(start);
    while (!stack.empty()) {
        auto block = stack.back();
        stack.pop_back();

        builder.define_block(block);

        auto code = block->code();
        for (NotNull<Instruction*> instr : code) {
            emit_instruction(instr, builder);
        }

        const auto& edge = block->edge();
        switch (edge.which()) {
        case BasicBlockEdge::Which::None:
            TIRO_ERROR("Block without a valid outgoing edge.");
        case BasicBlockEdge::Which::Jump: {
            auto target = edge.jump().target;
            if (seen(target)) {
                // Dont emit a jump if the block is the next on the stack.
                if (stack.empty() || stack.back() != target)
                    builder.jmp(target);
            } else {
                // Flatten the control flow - this block will be evaluated
                // in the next generation (without a jump).
                push(TIRO_NN(target));
            }
            break;
        }
        case BasicBlockEdge::Which::CondJump: {
            auto cond = edge.cond_jump();

            emit_instruction(cond.instr, cond.target, builder);
            push(TIRO_NN(cond.target));

            if (seen(cond.fallthrough)) {
                // Dont emit a jump if the block is the next on the stack.
                if (stack.empty() || stack.back() != cond.fallthrough)
                    builder.jmp(cond.fallthrough);
            } else {
                // Will be evaluated next.
                push(TIRO_NN(cond.fallthrough));
            }
            break;
        }

        case BasicBlockEdge::Which::AssertFail:
            builder.assert_fail();
            break;

        case BasicBlockEdge::Which::Never:
            break;

        case BasicBlockEdge::Which::Ret:
            builder.ret();
            break;
        }
    }

    builder.finish();
}

} // namespace tiro
