#include "compiler/syntax/dump.hpp"

#include "common/error.hpp"
#include "common/text/string_utils.hpp"
#include "compiler/source_map.hpp"
#include "compiler/syntax/syntax_tree.hpp"
#include "compiler/syntax/syntax_type.hpp"
#include "compiler/syntax/token.hpp"

#include <nlohmann/json.hpp>

namespace tiro {

namespace {

struct PrintRange {
    SourceRange range;
    const SourceMap& map;

    void format(FormatStream& stream) const {
        if (range.empty()) {
            auto pos = map.cursor_pos(range.begin());
            stream.format("{}:{}", pos.line(), pos.column());
        } else {
            auto [start, end] = map.cursor_pos(range);
            stream.format("{}:{}..{}:{}", start.line(), start.column(), end.line(), end.column());
        }
    }
};

class TreeWriter final {
public:
    explicit TreeWriter(const SyntaxTree& tree, const SourceMap& map, FormatStream& stream)
        : tree_(tree)
        , map_(map)
        , stream_(stream) {}

    void dump();

private:
    void dump_node(SyntaxNodeId node_id);
    void dump_child(const SyntaxChild& child);
    void dump_token(const Token& token);
    void dump_error(const SyntaxError& error);

    void inc_depth() { depth_ += 1; }
    void dec_depth() {
        TIRO_DEBUG_ASSERT(depth_ > 0, "depth must not be negative");
        depth_ -= 1;
    }

    PrintRange range(const SourceRange& range) { return PrintRange{range, map_}; }

    void indent_line() { stream_.format("{}", repeat(' ', depth_ * 2)); }

private:
    const SyntaxTree& tree_;
    const SourceMap& map_;
    FormatStream& stream_;
    int depth_ = 0;
};

} // namespace

std::string dump(const SyntaxTree& tree, const SourceMap& map) {
    StringFormatStream stream;
    TreeWriter dumper(tree, map, stream);
    dumper.dump();
    return stream.take_str();
}

void TreeWriter::dump() {
    auto root_id = tree_.root_id();
    TIRO_CHECK(root_id, "syntax tree does not have a root");

    dump_node(root_id);
    if (!tree_.errors().empty()) {
        stream_.format("\nErrors:\n");

        inc_depth();
        for (const auto& err : tree_.errors())
            dump_error(err);
        dec_depth();
    }
}

void TreeWriter::dump_node(SyntaxNodeId node_id) {
    const auto& node_data = tree_[node_id];

    indent_line();
    stream_.format("{}@{}\n", node_data.type(), range(node_data.range()));

    inc_depth();
    for (const auto& child : node_data.children())
        dump_child(child);
    dec_depth();
}

void TreeWriter::dump_child(const SyntaxChild& child) {
    struct Visitor {
        TreeWriter& self_;

        void visit_node_id(const SyntaxNodeId& id) { return self_.dump_node(id); }
        void visit_token(const Token& token) { return self_.dump_token(token); }
    };

    child.visit(Visitor{*this});
}

void TreeWriter::dump_token(const Token& token) {
    const auto source_range = token.range();
    const auto source_view = substring(tree_.source(), source_range);

    indent_line();
    stream_.format("{}@{} \"{}\"\n", token.type(), range(source_range), escape_string(source_view));
}

void TreeWriter::dump_error(const SyntaxError& error) {
    indent_line();
    stream_.format("{} {}\n", range(error.range()), error.message());
}

} // namespace tiro

TIRO_ENABLE_MEMBER_FORMAT(tiro::PrintRange);
