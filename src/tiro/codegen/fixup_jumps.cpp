#include "tiro/codegen/fixup_jumps.hpp"

#include "tiro/codegen/basic_block.hpp"
#include "tiro/codegen/instructions.hpp"
#include "tiro/core/safe_int.hpp"

#include <vector>

namespace tiro {

void fixup_jumps(InstructionStorage& storage, NotNull<BasicBlock*> start) {
    struct BlockData {
        i64 enter_balance = 0;
        i64 balance_diff = 0; // Includes POPs by edge transitions
    };

    struct BlockEdge {
        i64 enter_balance;
        NotNull<BasicBlock*> target;
    };

    std::unordered_map<NotNull<BasicBlock*>, BlockData> known_blocks;
    std::vector<BlockEdge> stack;

    auto push = [&](i64 enter_balance, NotNull<BasicBlock*> target) {
        stack.push_back({enter_balance, target});
    };

    // This loop computes stack balances for all reachable basic blocks.
    push(0, start);
    while (!stack.empty()) {
        const auto [enter_balance, block] = stack.back();
        stack.pop_back();

        TIRO_ASSERT(
            enter_balance >= 0, "Invalid input balance, must always be >= 0.");

        BlockData* data = nullptr;
        bool is_new_block = true;
        if (auto [pos, inserted] = known_blocks.try_emplace(block, BlockData());
            !inserted) {
            is_new_block = false;
            data = &pos->second;
        } else {
            data = &pos->second;
        }

        // Compute or reuse the stack difference made by the instructions in this block.
        if (!is_new_block) {
            if (enter_balance >= data->enter_balance) {
                // We have seen this block before. Evaluate this input only if the input balance is lower.
                // This can happen if we saw an origin place deep in the stack before the current path,
                // like in a break expression in a nested expression.
                continue;
            }

            data->enter_balance = enter_balance;
        } else {
            SafeInt balance = enter_balance;
            for (auto instr : block->code()) {
                const auto args = stack_arguments(instr);
                TIRO_CHECK(balance.value() >= args,
                    "Not enough arguments for the instruction of type {}, "
                    "requires {} arguments but stack holds only {}.",
                    to_string(instr->type()), args, balance.value());

                balance -= stack_arguments(instr);
                balance += stack_results(instr);
            }

            if (block->edge().which() == BasicBlockEdge::Which::CondJump) {
                const auto instr = block->edge().cond_jump().instr;
                const auto args = stack_arguments(instr);
                TIRO_CHECK(balance.value() >= args,
                    "Not enough arguments for the instruction of type {}, "
                    "requires {} arguments but stack holds only {}.",
                    to_string(instr), args, balance.value());

                balance -= args;
            }

            data->enter_balance = enter_balance;
            data->balance_diff = (balance - enter_balance).value();
        }

        const i64 exit_balance = data->enter_balance + data->balance_diff;

        // Visit the blocks reachable by this block.
        const auto& edge = block->edge();
        switch (edge.which()) {
        case BasicBlockEdge::Which::Jump:
            push(exit_balance, TIRO_NN(edge.jump().target));
            break;
        case BasicBlockEdge::Which::CondJump:
            push(exit_balance, TIRO_NN(edge.cond_jump().target));
            push(exit_balance, TIRO_NN(edge.cond_jump().fallthrough));
            break;
        case BasicBlockEdge::Which::None:
        case BasicBlockEdge::Which::AssertFail:
        case BasicBlockEdge::Which::Never:
        case BasicBlockEdge::Which::Ret:
            break;
        }
    }

    const auto block_data = [&](NotNull<BasicBlock*> block) -> BlockData* {
        auto pos = known_blocks.find(block);
        TIRO_ASSERT(pos != known_blocks.end(),
            "Block must be known by this stage in the algorithm.");
        return &pos->second;
    };

    // Fixup jump origins that have too many values on their stack.
    for (const auto& pair : known_blocks) {
        const NotNull<BasicBlock*> block = pair.first;
        const BlockData* data = &pair.second;

        const i64 exit_balance = data->enter_balance + data->balance_diff;
        const i64 target_balance = [&]() {
            const auto& edge = block->edge();
            switch (edge.which()) {
            case BasicBlockEdge::Which::Jump: {
                auto target_data = block_data(TIRO_NN(edge.jump().target));
                return target_data->enter_balance;
            }
            case BasicBlockEdge::Which::CondJump: {
                auto target_data = block_data(TIRO_NN(edge.cond_jump().target));
                auto fallthrough_data = block_data(
                    TIRO_NN(edge.cond_jump().fallthrough));
                TIRO_CHECK(target_data->enter_balance
                               == fallthrough_data->enter_balance,
                    "Both target blocks of a conditional jump must expect the "
                    "same stack balance.");
                return target_data->enter_balance;
            }
            case BasicBlockEdge::Which::None:
            case BasicBlockEdge::Which::AssertFail:
            case BasicBlockEdge::Which::Never:
            case BasicBlockEdge::Which::Ret:
                return exit_balance;
            }
            TIRO_UNREACHABLE("Invalid basic block edge type.");
        }();

        if (exit_balance < target_balance) {
            TIRO_ERROR(
                "Codegen bug: not enough values on the stack to satisfy the "
                "target block.");
        }

        if (exit_balance > target_balance) {
            const u32 diff = checked_cast<u32>(
                (SafeInt(exit_balance) - target_balance).value());
            if (diff == 1) {
                block->append(storage.make<Pop>());
            } else {
                block->append(storage.make<PopN>(diff));
            }
        }
    }
}

} // namespace tiro
