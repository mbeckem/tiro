#include "tiro/mir/traversal.hpp"

namespace tiro::compiler::mir {

static std::vector<mir::BlockID> dfs_preorder(const Function& func) {
    std::vector<bool> visited(func.block_count());
    std::vector<mir::BlockID> order;
    std::vector<mir::BlockID> visit_stack;
    std::vector<mir::BlockID> successors; // TODO small vec, few successors!

    auto visit = [&](mir::BlockID node) {
        TIRO_ASSERT(node, "Visited node must be valid.");
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

        auto block = func[id];
        order.push_back(id);

        successors.clear();
        visit_targets(block->terminator(),
            [&](auto succ) { successors.push_back(succ); });
        for (auto succ : reverse_view(successors)) {
            visit(succ);
        }
    }

    return order;
}

static std::vector<mir::BlockID> dfs_postorder(const Function& func) {
    std::vector<bool> visited(func.block_count());
    std::vector<mir::BlockID> order;
    std::vector<std::pair<mir::BlockID, bool>> visit_stack;
    std::vector<mir::BlockID> successors; // TODO small vec, few successors!

    auto visit = [&](mir::BlockID node) {
        TIRO_ASSERT(node, "Visited node must be valid.");
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

            auto block = func[id];
            successors.clear();
            visit_targets(block->terminator(),
                [&](auto succ) { successors.push_back(succ); });

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

PreorderTraversal::PreorderTraversal(const mir::Function& func)
    : func_(func)
    , blocks_(dfs_preorder(func)) {}

PostorderTraversal::PostorderTraversal(const mir::Function& func)
    : func_(func)
    , blocks_(dfs_postorder(func)) {}

ReversePostorderTraversal::ReversePostorderTraversal(const mir::Function& func)
    : postorder_(func) {}

} // namespace tiro::compiler::mir
