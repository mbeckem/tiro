#include "compiler/syntax/dump.hpp"

#include "compiler/syntax/syntax_tree.hpp"
#include "compiler/syntax/syntax_type.hpp"
#include "compiler/syntax/token.hpp"

#include <nlohmann/json.hpp>

namespace tiro {

using ordered_json = nlohmann::ordered_json;

namespace {

class TreeDumper final {
public:
    explicit TreeDumper(const SyntaxTree& tree)
        : tree_(tree) {}

    ordered_json dump();

private:
    ordered_json dump_node(SyntaxNodeId node_id);
    ordered_json dump_child(const SyntaxChild& child);
    ordered_json dump_token(const Token& token);
    ordered_json dump_error(const SyntaxError& error);
    ordered_json dump_range(const SourceRange& range);

private:
    const SyntaxTree& tree_;
};

} // namespace

std::string dump(const SyntaxTree& tree) {
    TreeDumper dumper(tree);
    auto jv = dumper.dump();
    return jv.dump(4);
}

ordered_json TreeDumper::dump() {
    auto root_id = tree_.root_id();
    TIRO_CHECK(root_id, "syntax tree does not have a root");

    auto jv_errors = ordered_json::array();
    for (const auto& error : tree_.errors()) {
        jv_errors.push_back(dump_error(error));
    }

    auto jv_tree = ordered_json::object();
    jv_tree.emplace("root", dump_node(root_id));
    jv_tree.emplace("errors", std::move(jv_errors));
    return jv_tree;
}

ordered_json TreeDumper::dump_node(SyntaxNodeId node_id) {
    const auto& node_data = tree_[node_id];

    auto jv_children = ordered_json::array();
    for (const auto& child : node_data.children()) {
        jv_children.push_back(dump_child(child));
    }

    auto jv_node = ordered_json::object();
    jv_node.emplace("kind", "node");
    jv_node.emplace("type", to_string(node_data.type()));
    jv_node.emplace("has_error", node_data.has_error());
    jv_node.emplace("range", dump_range(node_data.range()));
    jv_node.emplace("children", std::move(jv_children));
    return jv_node;
}

ordered_json TreeDumper::dump_child(const SyntaxChild& child) {
    struct Visitor {
        TreeDumper& self_;

        ordered_json visit_node_id(const SyntaxNodeId& id) { return self_.dump_node(id); }
        ordered_json visit_token(const Token& token) { return self_.dump_token(token); }
    };

    return child.visit(Visitor{*this});
}

ordered_json TreeDumper::dump_token(const Token& token) {
    auto jv_token = ordered_json::object();
    jv_token.emplace("kind", "token");
    jv_token.emplace("type", to_string(token.type()));
    jv_token.emplace("range", dump_range(token.range()));
    jv_token.emplace("text", substring(tree_.source(), token.range()));
    return jv_token;
}

ordered_json TreeDumper::dump_error(const SyntaxError& error) {
    auto jv_error = ordered_json::object();
    jv_error.emplace("range", dump_range(error.range()));
    jv_error.emplace("message", error.message());
    return jv_error;
}

ordered_json TreeDumper::dump_range(const SourceRange& range) {
    return ordered_json::array({range.begin(), range.end()});
}

} // namespace tiro
