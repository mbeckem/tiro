#include "./simple_syntax_tree.hpp"

#include "common/fix.hpp"
#include "compiler/syntax/build_syntax_tree.hpp"
#include "compiler/syntax/grammar/expr.hpp"
#include "compiler/syntax/grammar/item.hpp"
#include "compiler/syntax/grammar/stmt.hpp"
#include "compiler/syntax/lexer.hpp"
#include "compiler/syntax/parser.hpp"
#include "compiler/syntax/syntax_tree.hpp"

#include <catch2/catch.hpp>

namespace tiro::next::test {

namespace {

struct TestHelper {
public:
    TestHelper(std::string_view source)
        : source_(source)
        , tokens_(tokenize(source))
        , parser_(tokens_) {}

    Parser& parser() { return parser_; }

    std::unique_ptr<SimpleSyntaxTree> get_parse_tree();

private:
    static std::vector<Token> tokenize(std::string_view source) {
        Lexer lexer(source);
        lexer.ignore_comments(true);

        std::vector<Token> tokens;
        while (1) {
            auto token = lexer.next();
            tokens.push_back(token);
            if (token.type() == TokenType::Eof)
                break;
        }
        return tokens;
    }

private:
    std::string_view source_;
    std::vector<Token> tokens_;
    Parser parser_;
};

} // namespace

std::unique_ptr<SimpleSyntaxTree> TestHelper::get_parse_tree() {
    if (!parser_.at(TokenType::Eof))
        FAIL_CHECK("Parser did not reach the end of file.");

    SyntaxTree full_tree = build_syntax_tree(source_, parser_.take_events());
    const auto root_id = full_tree.root_id();
    TIRO_CHECK(root_id, "Syntax tree does not have a root.");

    const auto& errors = full_tree.errors();
    for (const auto& error : errors) {
        UNSCOPED_INFO(error.message());
    }
    REQUIRE(errors.empty());

    // Full syntax node to simple tree node mapping.
    // The simple nodes are inefficient but easier to work with in tests.
    Fix map_node = [&](auto& self, SyntaxNodeId node_id) -> std::unique_ptr<SimpleSyntaxNode> {
        auto node_data = full_tree[node_id];

        if (node_data->has_error())
            TIRO_ERROR("Syntax error");

        auto simple_node = std::make_unique<SimpleSyntaxNode>(node_data->type());
        for (const auto& child : node_data->children()) {
            switch (child.type()) {
            case SyntaxChildType::Token: {
                auto& token = child.as_token();
                std::string_view text = substring(source_, token.range());
                simple_node->children.push_back(
                    std::make_unique<SimpleSyntaxToken>(token.type(), std::string(text)));
                break;
            }
            case SyntaxChildType::NodeId:
                simple_node->children.push_back(self(child.as_node_id()));
                break;
            default:
                TIRO_UNREACHABLE("Invalid child type.");
            }
        }
        return simple_node;
    };

    // Don't return the root node to the unit tests
    auto node = map_node(root_id);
    TIRO_CHECK(node->children.size() == 1, "Root node must have exactly one child in tests.");
    return std::move(node->children[0]);
}

std::string dump_parse_tree(const SimpleSyntaxTree* root) {
    StringFormatStream stream;

    int indent = 0;
    Fix dump = [&](auto& self, const SimpleSyntaxTree* tree) -> void {
        if (!tree) {
            stream.format("{}NULL\n", spaces(indent));
            return;
        }

        switch (tree->kind) {
        case SimpleSyntaxTree::TOKEN:
            stream.format(
                "{}{}\n", spaces(indent), static_cast<const SimpleSyntaxToken*>(tree)->to_string());
            break;
        case SimpleSyntaxTree::NODE: {
            auto node = static_cast<const SimpleSyntaxNode*>(tree);
            stream.format("{}{}\n", spaces(indent), node->to_string());

            indent += 2;
            for (const auto& child : node->children) {
                self(child.get());
            }
            indent -= 2;
            break;
        }
        }
    };
    dump(root);
    return stream.take_str();
}

template<typename Parse>
static std::unique_ptr<SimpleSyntaxTree> run_parse(std::string_view source, Parse&& parse) {
    TestHelper helper(source);
    parse(helper.parser());

    auto tree = helper.get_parse_tree();
    if (helper.parser().current() != TokenType::Eof)
        FAIL("Parser did not reach the end of file.");

    return tree;
}

std::unique_ptr<SimpleSyntaxTree> parse_expr_syntax(std::string_view source) {
    return run_parse(source, [&](Parser& p) { parse_expr(p, {}); });
}

std::unique_ptr<SimpleSyntaxTree> parse_stmt_syntax(std::string_view source) {
    return run_parse(source, [&](Parser& p) { parse_stmt(p, {}); });
}

std::unique_ptr<SimpleSyntaxTree> parse_item_syntax(std::string_view source) {
    return run_parse(source, [&](Parser& p) { parse_item(p, {}); });
}

std::unique_ptr<SimpleSyntaxTree> parse_file_syntax(std::string_view source) {
    return run_parse(source, [&](Parser& p) { parse_file(p); });
}

} // namespace tiro::next::test
