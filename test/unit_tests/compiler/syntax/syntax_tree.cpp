#include "./syntax_tree.hpp"

#include "common/fix.hpp"
#include "compiler/syntax/grammar/expr.hpp"
#include "compiler/syntax/grammar/item.hpp"
#include "compiler/syntax/grammar/stmt.hpp"
#include "compiler/syntax/lexer.hpp"
#include "compiler/syntax/parser.hpp"

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

    std::unique_ptr<SyntaxTree> get_parse_tree();

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

std::unique_ptr<SyntaxTree> TestHelper::get_parse_tree() {
    struct Consumer : ParserEventConsumer {
        TestHelper& self;
        std::unique_ptr<SyntaxNode> root;
        std::vector<SyntaxNode*> parents;

        Consumer(TestHelper& self_)
            : self(self_) {}

        void start_node(SyntaxType type) override {
            auto node = std::make_unique<SyntaxNode>(type);
            auto node_addr = node.get();
            if (parents.empty()) {
                if (root)
                    throw std::runtime_error("Invalid start event after root has been finished.");

                root = std::move(node);
            } else {
                parents.back()->children.push_back(std::move(node));
            }

            parents.push_back(node_addr);
        }

        void token(Token& t) override {
            if (parents.empty())
                throw std::runtime_error("Invalid token event: no active node.");

            auto type = t.type();
            auto text = substring(self.source_, t.range());
            parents.back()->children.push_back(
                std::make_unique<SyntaxToken>(type, std::string(text)));
        }

        void error(std::string& message) override {
            if (parents.empty())
                throw std::runtime_error("Invalid error event: no active node.");

            std::string exception = "Parse error: ";
            exception += message;
            throw std::runtime_error(std::move(exception));
        }

        void finish_node() override {
            if (parents.empty())
                throw std::runtime_error("Invalid finish event: no active node.");

            parents.pop_back();
        }
    };

    Consumer consumer(*this);
    consume_events(parser_.take_events(), consumer);

    if (!consumer.root) {
        throw std::runtime_error("Empty syntax tree.");
    }
    return std::move(consumer.root);
}

std::string dump_parse_tree(const SyntaxTree* root) {
    StringFormatStream stream;

    int indent = 0;
    Fix dump = [&](auto& self, const SyntaxTree* tree) -> void {
        if (!tree) {
            stream.format("{}NULL\n", spaces(indent));
            return;
        }

        switch (tree->kind) {
        case SyntaxTree::TOKEN:
            stream.format(
                "{}{}\n", spaces(indent), static_cast<const SyntaxToken*>(tree)->to_string());
            break;
        case SyntaxToken::NODE: {
            auto node = static_cast<const SyntaxNode*>(tree);
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
static std::unique_ptr<SyntaxTree> run_parse(std::string_view source, Parse&& parse) {
    TestHelper helper(source);
    parse(helper.parser());

    auto tree = helper.get_parse_tree();
    if (helper.parser().current() != TokenType::Eof)
        FAIL("Parser did not reach the end of file.");

    return tree;
}

std::unique_ptr<SyntaxTree> parse_expr_syntax(std::string_view source) {
    return run_parse(source, [&](Parser& p) { parse_expr(p, {}); });
}

std::unique_ptr<SyntaxTree> parse_stmt_syntax(std::string_view source) {
    return run_parse(source, [&](Parser& p) { parse_stmt(p, {}); });
}

std::unique_ptr<SyntaxTree> parse_item_syntax(std::string_view source) {
    return run_parse(source, [&](Parser& p) { parse_item(p, {}); });
}

std::unique_ptr<SyntaxTree> parse_file_syntax(std::string_view source) {
    return run_parse(source, [&](Parser& p) { parse_file(p); });
}

} // namespace tiro::next::test
