#include <catch2/catch.hpp>

#include "common/fix.hpp"
#include "common/text/string_utils.hpp"
#include "compiler/syntax/lexer.hpp"
#include "compiler/syntax/parse_expr.hpp"
#include "compiler/syntax/parser.hpp"

#include <memory>

using namespace tiro;
using namespace tiro::next;

namespace {

class SyntaxTree {
public:
    enum Kind { TOKEN, NODE };

    const Kind kind;

    SyntaxTree(Kind kind_)
        : kind(kind_) {}

    virtual ~SyntaxTree() {}

    virtual std::string to_string() const = 0;
};

class SyntaxToken final : public SyntaxTree {
public:
    TokenType type;
    std::string text;

public:
    SyntaxToken(TokenType type_, std::string text_)
        : SyntaxTree(TOKEN)
        , type(std::move(type_))
        , text(std::move(text_)) {}

    std::string to_string() const override {
        return fmt::format("Token: {} \"{}\"", type, escape_string(text));
    }
};

class SyntaxNode final : public SyntaxTree {
public:
    SyntaxType type;
    std::vector<std::unique_ptr<SyntaxTree>> children;

public:
    SyntaxNode(SyntaxType type_)
        : SyntaxTree(NODE)
        , type(type_) {}

    std::string to_string() const override { return fmt::format("Node: {}", type); }
};

class SyntaxTreeMatcher {
public:
    virtual ~SyntaxTreeMatcher() {}

    virtual void match(const SyntaxTree* tree) const = 0;
};

class SyntaxNodeTypeMatcher final : public SyntaxTreeMatcher {
public:
    SyntaxNodeTypeMatcher(SyntaxType expected_type)
        : expected_type_(expected_type) {}

    void match(const SyntaxTree* tree) const override {
        if (!tree || tree->kind != SyntaxTree::NODE)
            FAIL("Expected a node");

        auto* node = static_cast<const SyntaxNode*>(tree);
        INFO("Expected: " << fmt::to_string(expected_type_));
        INFO("Actual: " << fmt::to_string(node->type));
        REQUIRE((node->type == expected_type_));
    }

private:
    SyntaxType expected_type_;
};

class SyntaxNodeChildrenMatcher final : public SyntaxTreeMatcher {
public:
    SyntaxNodeChildrenMatcher(std::vector<std::shared_ptr<SyntaxTreeMatcher>> matchers)
        : matchers_(std::move(matchers)) {}

    void match(const SyntaxTree* tree) const override {
        if (!tree || tree->kind != SyntaxTree::NODE)
            FAIL("Expected a node");

        auto* node = static_cast<const SyntaxNode*>(tree);
        if (matchers_.size() != node->children.size()) {
            INFO("Expected: " << matchers_.size() << " children");
            INFO("Actual: " << node->children.size() << " children");
            FAIL("Unexpected number of children");
        }

        for (size_t i = 0; i < matchers_.size(); ++i) {
            INFO(fmt::format("In {} [child {}]", node->type, i));
            matchers_[i]->match(node->children[i].get());
        }
    }

private:
    std::vector<std::shared_ptr<SyntaxTreeMatcher>> matchers_;
};

class SyntaxTokenTypeMatcher final : public SyntaxTreeMatcher {
public:
    SyntaxTokenTypeMatcher(TokenType expected_type)
        : expected_type_(expected_type) {}

    void match(const SyntaxTree* tree) const override {
        if (!tree || tree->kind != SyntaxTree::TOKEN)
            FAIL("Expected a token");

        auto* token = static_cast<const SyntaxToken*>(tree);
        INFO("Expected: " << fmt::to_string(expected_type_));
        INFO("Actual: " << fmt::to_string(token->type));
        REQUIRE((token->type == expected_type_));
    }

private:
    TokenType expected_type_;
};

class SyntaxTokenTextMatcher final : public SyntaxTreeMatcher {
public:
    SyntaxTokenTextMatcher(std::string expected_text)
        : expected_text_(std::move(expected_text)) {}

    void match(const SyntaxTree* tree) const override {
        if (!tree || tree->kind != SyntaxTree::TOKEN)
            FAIL("Expected a token");

        auto* token = static_cast<const SyntaxToken*>(tree);
        INFO("Expected: " << fmt::to_string(expected_text_));
        INFO("Actual: " << fmt::to_string(token->text));
        REQUIRE(token->text == expected_text_);
    }

private:
    std::string expected_text_;
};

class CombinedSyntaxTreeMatcher final : public SyntaxTreeMatcher {
public:
    CombinedSyntaxTreeMatcher(std::vector<std::shared_ptr<SyntaxTreeMatcher>> matchers)
        : matchers_(std::move(matchers)) {}

    void match(const SyntaxTree* tree) const override {
        for (const auto& matcher : matchers_)
            matcher->match(tree);
    }

private:
    std::vector<std::shared_ptr<SyntaxTreeMatcher>> matchers_;
};

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

}; // namespace

static std::shared_ptr<SyntaxTreeMatcher>
combine(std::vector<std::shared_ptr<SyntaxTreeMatcher>> matchers) {
    return std::make_unique<CombinedSyntaxTreeMatcher>(std::move(matchers));
}

static std::shared_ptr<SyntaxTreeMatcher> token_type(TokenType expected) {
    return std::make_unique<SyntaxTokenTypeMatcher>(expected);
}

static std::shared_ptr<SyntaxTreeMatcher> token(TokenType expected, std::string expected_text) {
    return combine({
        token_type(expected),
        std::make_unique<SyntaxTokenTextMatcher>(std::move(expected_text)),
    });
}

static std::shared_ptr<SyntaxTreeMatcher> node_type(SyntaxType expected) {
    return std::make_unique<SyntaxNodeTypeMatcher>(expected);
}

static std::shared_ptr<SyntaxTreeMatcher>
node(SyntaxType expected, std::vector<std::shared_ptr<SyntaxTreeMatcher>> children) {
    return combine({
        node_type(expected),
        std::make_unique<SyntaxNodeChildrenMatcher>(std::move(children)),
    });
}

static std::shared_ptr<SyntaxTreeMatcher> literal(TokenType expected) {
    return node(SyntaxType::Literal, {token_type(expected)});
}

static std::shared_ptr<SyntaxTreeMatcher> literal(TokenType expected, std::string text) {
    return node(SyntaxType::Literal, {token(expected, std::move(text))});
}

static std::shared_ptr<SyntaxTreeMatcher>
unary_expr(TokenType op, std::shared_ptr<SyntaxTreeMatcher> inner) {
    return node(SyntaxType::UnaryExpr, {token_type(op), std::move(inner)});
}

static std::shared_ptr<SyntaxTreeMatcher> binary_expr(
    TokenType op, std::shared_ptr<SyntaxTreeMatcher> lhs, std::shared_ptr<SyntaxTreeMatcher> rhs) {
    return node(SyntaxType::BinaryExpr, {std::move(lhs), token_type(op), std::move(rhs)});
}

static std::shared_ptr<SyntaxTreeMatcher> var_expr(std::string varname) {
    return node(SyntaxType::VarExpr, {token(TokenType::Identifier, varname)});
}

static std::shared_ptr<SyntaxTreeMatcher>
member_expr(std::shared_ptr<SyntaxTreeMatcher> obj, std::string varname, bool optional = false) {
    return node(SyntaxType::MemberExpr, //
        {
            std::move(obj),
            token_type(optional ? TokenType::QuestionDot : TokenType::Dot),
            token(TokenType::Identifier, std::move(varname)),
        });
}

static std::shared_ptr<SyntaxTreeMatcher> index_expr(std::shared_ptr<SyntaxTreeMatcher> obj,
    std::shared_ptr<SyntaxTreeMatcher> index, bool optional = false) {
    return node(SyntaxType::IndexExpr, //
        {
            std::move(obj),
            token_type(optional ? TokenType::QuestionLeftBracket : TokenType::LeftBracket),
            std::move(index),
            token_type(TokenType::RightBracket),
        });
}

static std::string dump_parse_tree(const SyntaxTree* root) {
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

static std::unique_ptr<SyntaxTree> parse_expr_syntax(std::string_view source) {
    TestHelper helper(source);

    tiro::next::parse_expr(helper.parser(), {});
    if (helper.parser().current() != TokenType::Eof)
        FAIL("Parser did not reach the end of file.");

    return helper.get_parse_tree();
}

static void
assert_parse_tree(const SyntaxTree* actual, std::shared_ptr<SyntaxTreeMatcher> expected) {
    INFO("Parse tree:\n" << dump_parse_tree(actual) << "\n");

    expected->match(actual);
}

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

TEST_CASE("Parser should parse plain literals", "[syntax]") {
    struct Test {
        std::string_view source;
        TokenType expected_type;
    };

    Test tests[] = {
        {"true", TokenType::KwTrue},
        {"false", TokenType::KwFalse},
        {"null", TokenType::KwNull},
        {"#abc", TokenType::Symbol},
        {"1234", TokenType::Integer},
        {"123.456", TokenType::Float},
    };

    for (const auto& spec : tests) {
        auto tree = parse_expr_syntax(spec.source);
        assert_parse_tree(tree.get(), literal(spec.expected_type, std::string(spec.source)));
    }
}

TEST_CASE("Parser should respect arithmetic operator precedence", "[syntax]") {
    std::string_view source = "-4**2 + 1234 * 2.34 - 1";

    auto tree = parse_expr_syntax(source);
    assert_parse_tree(tree.get(),                //
        binary_expr(TokenType::Minus,            //
            binary_expr(TokenType::Plus,         //
                binary_expr(TokenType::StarStar, //
                    unary_expr(TokenType::Minus, literal(TokenType::Integer, "4")),
                    literal(TokenType::Integer, "2")),
                binary_expr(TokenType::Star, //
                    literal(TokenType::Integer, "1234"), literal(TokenType::Float, "2.34"))),
            literal(TokenType::Integer, "1")));
}

TEST_CASE("Parser should respect operator precedence in assignments", "[syntax]") {
    std::string_view source = "a = b = 3 && 4";

    auto tree = parse_expr_syntax(source);
    assert_parse_tree(tree.get(),      //
        binary_expr(TokenType::Equals, // a =
            var_expr("a"),
            binary_expr(TokenType::Equals, // b =
                var_expr("b"),
                binary_expr(TokenType::LogicalAnd, // 3 && 4
                    literal(TokenType::Integer, "3"), literal(TokenType::Integer, "4")))));
}

TEST_CASE("Parser should support binary assignment operators", "[syntax]") {
    auto tree = parse_expr_syntax("3 + (c = b -= 4 ** 2)");
    assert_parse_tree(tree.get(),    //
        binary_expr(TokenType::Plus, //
            literal(TokenType::Integer, "3"),
            node(SyntaxType::GroupedExpr, //
                {
                    token_type(TokenType::LeftParen),
                    binary_expr(TokenType::Equals, //
                        var_expr("c"),
                        binary_expr(TokenType::MinusEquals, //
                            var_expr("b"),
                            binary_expr(TokenType::StarStar, //
                                literal(TokenType::Integer, "4"),
                                literal(TokenType::Integer, "2")))),
                    token_type(TokenType::RightParen),
                })));
}

TEST_CASE("Parser should support the null coalescing operator", "[syntax]") {
    auto tree = parse_expr_syntax("x.y ?? 3");
    assert_parse_tree(tree.get(),                //
        binary_expr(TokenType::QuestionQuestion, //
            member_expr(var_expr("x"), "y"), literal(TokenType::Integer)));
}

TEST_CASE("Parser should respect the low precedence of the null coalescing operator", "[syntax]") {
    auto tree = parse_expr_syntax("x ?? 3 - 4");
    assert_parse_tree(tree.get(),                //
        binary_expr(TokenType::QuestionQuestion, //
            var_expr("x"),
            binary_expr(TokenType::Minus, //
                literal(TokenType::Integer, "3"), literal(TokenType::Integer, "4"))));
}

TEST_CASE("Parser handles grouped expressions", "[syntax]") {
    std::string_view source = "(a + b * 2)";

    auto tree = parse_expr_syntax(source);
    assert_parse_tree(tree.get(),     //
        node(SyntaxType::GroupedExpr, //
            {
                token_type(TokenType::LeftParen),
                binary_expr(TokenType::Plus, //
                    var_expr("a"),
                    binary_expr(TokenType::Star, //
                        var_expr("b"), literal(TokenType::Integer, "2"))),
                token_type(TokenType::RightParen),
            }));
}

TEST_CASE("Parser handles empty tuple literals", "[syntax]") {
    auto tree = parse_expr_syntax("()");
    assert_parse_tree(tree.get(),   //
        node(SyntaxType::TupleExpr, //
            {
                token_type(TokenType::LeftParen),
                token_type(TokenType::RightParen),
            }));
}

TEST_CASE("Parser handles single-element tuple literals", "[syntax]") {
    auto tree = parse_expr_syntax("(1,)");
    assert_parse_tree(tree.get(),   //
        node(SyntaxType::TupleExpr, //
            {
                token_type(TokenType::LeftParen),
                literal(TokenType::Integer, "1"),
                token_type(TokenType::Comma),
                token_type(TokenType::RightParen),
            }));
}

TEST_CASE("Parser handles tuple literals", "[syntax]") {
    auto tree = parse_expr_syntax("(1, 2, 3)");
    assert_parse_tree(tree.get(),   //
        node(SyntaxType::TupleExpr, //
            {
                token_type(TokenType::LeftParen),
                literal(TokenType::Integer, "1"),
                token_type(TokenType::Comma),
                literal(TokenType::Integer, "2"),
                token_type(TokenType::Comma),
                literal(TokenType::Integer, "3"),
                token_type(TokenType::RightParen),
            }));
}

TEST_CASE("Parser handles tuple literals with trailing commas", "[syntax]") {
    auto tree = parse_expr_syntax("(1, 2, 3,)");
    assert_parse_tree(tree.get(),   //
        node(SyntaxType::TupleExpr, //
            {
                token_type(TokenType::LeftParen),
                literal(TokenType::Integer, "1"),
                token_type(TokenType::Comma),
                literal(TokenType::Integer, "2"),
                token_type(TokenType::Comma),
                literal(TokenType::Integer, "3"),
                token_type(TokenType::Comma),
                token_type(TokenType::RightParen),
            }));
}

TEST_CASE("Parser handles empty record literals", "[syntax]") {
    auto tree = parse_expr_syntax("(:)");
    assert_parse_tree(tree.get(),    //
        node(SyntaxType::RecordExpr, //
            {
                token_type(TokenType::LeftParen),
                token_type(TokenType::Colon),
                token_type(TokenType::RightParen),
            }));
}

TEST_CASE("Parser handles record literals", "[syntax]") {
    auto tree = parse_expr_syntax("(a: b, c: 1)");
    assert_parse_tree(tree.get(),    //
        node(SyntaxType::RecordExpr, //
            {
                token_type(TokenType::LeftParen),
                var_expr("a"),
                token_type(TokenType::Colon),
                var_expr("b"),
                token_type(TokenType::Comma),
                var_expr("c"),
                token_type(TokenType::Colon),
                literal(TokenType::Integer, "1"),
                token_type(TokenType::RightParen),
            }));
}

TEST_CASE("Parser handles record literals with trailing comma", "[syntax]") {
    auto tree = parse_expr_syntax("(a: b, c: 1,)");
    assert_parse_tree(tree.get(),    //
        node(SyntaxType::RecordExpr, //
            {
                token_type(TokenType::LeftParen),
                var_expr("a"),
                token_type(TokenType::Colon),
                var_expr("b"),
                token_type(TokenType::Comma),
                var_expr("c"),
                token_type(TokenType::Colon),
                literal(TokenType::Integer, "1"),
                token_type(TokenType::Comma),
                token_type(TokenType::RightParen),
            }));
}

TEST_CASE("Parser handles empty array literals", "[syntax]") {
    auto tree = parse_expr_syntax("[]");
    assert_parse_tree(tree.get(),   //
        node(SyntaxType::ArrayExpr, //
            {
                token_type(TokenType::LeftBracket),
                token_type(TokenType::RightBracket),
            }));
}

TEST_CASE("Parser handles array literals", "[syntax]") {
    auto tree = parse_expr_syntax("[1, 2]");
    assert_parse_tree(tree.get(),   //
        node(SyntaxType::ArrayExpr, //
            {
                token_type(TokenType::LeftBracket),
                literal(TokenType::Integer, "1"),
                token_type(TokenType::Comma),
                literal(TokenType::Integer, "2"),
                token_type(TokenType::RightBracket),
            }));
}

TEST_CASE("Parser handles array literals with trailing comma", "[syntax]") {
    auto tree = parse_expr_syntax("[1, 2,]");
    assert_parse_tree(tree.get(),   //
        node(SyntaxType::ArrayExpr, //
            {
                token_type(TokenType::LeftBracket),
                literal(TokenType::Integer, "1"),
                token_type(TokenType::Comma),
                literal(TokenType::Integer, "2"),
                token_type(TokenType::Comma),
                token_type(TokenType::RightBracket),
            }));
}

TEST_CASE("Parser handles member access", "[syntax]") {
    auto tree = parse_expr_syntax("a?.b.c");
    assert_parse_tree(tree.get(), //
        member_expr(member_expr(var_expr("a"), "b", true), "c"));
}

TEST_CASE("Parser handles array access", "[syntax]") {
    auto tree = parse_expr_syntax("a[b?[c]]");
    assert_parse_tree(tree.get(), //
        index_expr(               //
            var_expr("a"),        //
            index_expr(           //
                var_expr("b"),    //
                var_expr("c"), true)));
}
