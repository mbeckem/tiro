#include "hammer/ast/node.hpp"

#include "hammer/ast/node_formatter.hpp"
#include "hammer/ast/stmt.hpp"

namespace hammer::ast {

std::string_view to_string(NodeKind kind) {
    switch (kind) {
#define HAMMER_AST_LEAF_TYPE(Type, ParentType) \
    case NodeKind::Type:                       \
        return #Type;

#include "hammer/ast/node_types.inc"
    }

    HAMMER_UNREACHABLE("Invalid node kind.");
}

Node::Node(NodeKind kind)
    : kind_(kind) {}

Node::~Node() {    
}

void Node::dump_impl(NodeFormatter& fmt) const {
    std::string start_string;
    if (start_) {
        start_string = ::fmt::format("{} [{}:{})",
            fmt.strings().value(start_.file_name()), start_.begin(),
            start_.end());
    } else {
        start_string = "N/A";
    }

    fmt.properties("start", start_string, "has_error", has_error());
}

void dump(
    const Node& n, std::ostream& os, const StringTable& strings, int indent) {
    NodeFormatter fmt(strings, os, std::min(0, indent));
    fmt.visit_node(n);
}

} // namespace hammer::ast
