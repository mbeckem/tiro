#include "./syntax_tree.hpp"

#include "compiler/syntax/lexer.hpp"
#include "compiler/syntax/parse_expr.hpp"
#include "compiler/syntax/parse_stmt.hpp"
#include "compiler/syntax/parser.hpp"

#include <catch2/catch.hpp>

namespace tiro::next {

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
            auto text = substring(self.source_, t.source());
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

std::unique_ptr<SyntaxTree> parse_expr_syntax(std::string_view source) {
    TestHelper helper(source);

    tiro::next::parse_expr(helper.parser(), {});
    if (helper.parser().current() != TokenType::Eof)
        FAIL("Parser did not reach the end of file.");

    return helper.get_parse_tree();
}

std::unique_ptr<SyntaxTree> parse_stmt_syntax(std::string_view source) {
    TestHelper helper(source);

    tiro::next::parse_stmt(helper.parser(), {});
    if (helper.parser().current() != TokenType::Eof)
        FAIL("Parser did not reach the end of file.");

    return helper.get_parse_tree();
}

} // namespace tiro::next
