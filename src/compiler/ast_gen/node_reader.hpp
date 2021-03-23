#ifndef TIRO_COMPILER_BUILD_AST_NODE_READER
#define TIRO_COMPILER_BUILD_AST_NODE_READER

#include "common/defs.hpp"
#include "compiler/ast_gen/typed_nodes.hpp"
#include "compiler/syntax/syntax_tree.hpp"
#include "compiler/syntax/syntax_type.hpp"

#include <type_traits>

namespace tiro::typed_syntax {

template<SyntaxType st>
using NodeType = typename SyntaxTypeToNodeType<st>::type;

class NodeReader final {
public:
    explicit NodeReader(const SyntaxTree& tree)
        : tree_(tree) {}

    template<SyntaxType st>
    std::optional<NodeType<st>> read(SyntaxNodeId node_id) const {
        using node_type = NodeType<st>;
        static_assert(!std::is_same_v<node_type, void>,
            "there is no node type registered for this syntax type.");

        TIRO_DEBUG_ASSERT(
            tree_[node_id]->type() == st, "requested syntax type does not match the node's type.");
        return node_type::read(node_id, tree_);
    }

private:
    const SyntaxTree& tree_;
};

} // namespace tiro::typed_syntax

#endif // TIRO_COMPILER_BUILD_AST_NODE_READER
