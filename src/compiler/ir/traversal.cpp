#include "compiler/ir/traversal.hpp"

#include "compiler/ir/terminator.hpp"

namespace tiro::ir {

static std::vector<BlockId> dfs_preorder(const Function& func) {
    std::vector<bool> visited(func.block_count());
    std::vector<BlockId> order;
    std::vector<BlockId> visit_stack;
    std::vector<BlockId> successors; // TODO small vec, few successors!

    auto visit = [&](BlockId node) {
        TIRO_DEBUG_ASSERT(node, "Visited node must be valid.");
        const auto index = node.value();
        if (!visited[index]) {
            visited[index] = true;
            visit_stack.emplace_back(node);
            return true;
        }
        return false;
    };

    visit(func.entry());
    while (!visit_stack.empty()) {
        auto id = visit_stack.back();
        visit_stack.pop_back();

        const auto& block = func[id];
        order.push_back(id);

        successors.clear();
        visit_targets(block.terminator(), [&](auto succ) { successors.push_back(succ); });
        for (auto succ : reverse_view(successors)) {
            visit(succ);
        }
    }

    return order;
}

static std::vector<BlockId> dfs_postorder(const Function& func) {
    std::vector<bool> visited(func.block_count());
    std::vector<BlockId> order;
    std::vector<std::pair<BlockId, bool>> visit_stack;
    std::vector<BlockId> successors; // TODO small vec, few successors!

    auto visit = [&](BlockId node) {
        TIRO_DEBUG_ASSERT(node, "Visited node must be valid.");
        const auto index = node.value();
        if (!visited[index]) {
            visited[index] = true;
            visit_stack.emplace_back(node, true);
            return true;
        }
        return false;
    };

    visit(func.entry());
    while (!visit_stack.empty()) {
        auto& [id, first] = visit_stack.back();
        if (first) {
            first = false;

            const auto& block = func[id];
            successors.clear();
            visit_targets(block.terminator(), [&](auto succ) { successors.push_back(succ); });

            for (auto succ : reverse_view(successors)) {
                visit(succ);
            }
        } else {
            visit_stack.pop_back();
            order.push_back(id);
        }
    }

    return order;
}

PreorderTraversal::PreorderTraversal(const Function& func)
    : func_(func)
    , blocks_(dfs_preorder(func)) {}

PostorderTraversal::PostorderTraversal(const Function& func)
    : func_(func)
    , blocks_(dfs_postorder(func)) {}

ReversePostorderTraversal::ReversePostorderTraversal(const Function& func)
    : postorder_(func) {}

} // namespace tiro::ir
