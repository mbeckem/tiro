#include "./syntax_assert.hpp"

#include "common/text/string_utils.hpp"
#include "compiler/syntax/lexer.hpp"
#include "compiler/syntax/parser.hpp"

#include <catch2/catch.hpp>

namespace tiro::test {

namespace {

class SyntaxNodeTypeMatcher final : public SyntaxTreeMatcher {
public:
    SyntaxNodeTypeMatcher(SyntaxType expected_type)
        : expected_type_(expected_type) {}

    void match(const SimpleSyntaxTree* tree) const override {
        if (!tree || tree->kind != SimpleSyntaxTree::NODE)
            FAIL("Expected a node");

        auto* node = static_cast<const SimpleSyntaxNode*>(tree);
        INFO("Expected: " << fmt::to_string(expected_type_));
        INFO("Actual: " << fmt::to_string(node->type));
        REQUIRE((node->type == expected_type_));
    }

private:
    SyntaxType expected_type_;
};

class SyntaxNodeChildrenMatcher final : public SyntaxTreeMatcher {
public:
    SyntaxNodeChildrenMatcher(std::vector<SyntaxTreeMatcherPtr> matchers)
        : matchers_(std::move(matchers)) {}

    void match(const SimpleSyntaxTree* tree) const override {
        if (!tree || tree->kind != SimpleSyntaxTree::NODE)
            FAIL("Expected a node");

        auto* node = static_cast<const SimpleSyntaxNode*>(tree);
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
    std::vector<SyntaxTreeMatcherPtr> matchers_;
};

class SyntaxTokenTypeMatcher final : public SyntaxTreeMatcher {
public:
    SyntaxTokenTypeMatcher(TokenType expected_type)
        : expected_type_(expected_type) {}

    void match(const SimpleSyntaxTree* tree) const override {
        if (!tree || tree->kind != SimpleSyntaxTree::TOKEN)
            FAIL("Expected a token");

        auto* token = static_cast<const SimpleSyntaxToken*>(tree);
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

    void match(const SimpleSyntaxTree* tree) const override {
        if (!tree || tree->kind != SimpleSyntaxTree::TOKEN)
            FAIL("Expected a token");

        auto* token = static_cast<const SimpleSyntaxToken*>(tree);
        INFO("Expected: " << fmt::to_string(expected_text_));
        INFO("Actual: " << fmt::to_string(token->text));
        REQUIRE(token->text == expected_text_);
    }

private:
    std::string expected_text_;
};

class CombinedSyntaxTreeMatcher final : public SyntaxTreeMatcher {
public:
    CombinedSyntaxTreeMatcher(std::vector<SyntaxTreeMatcherPtr> matchers)
        : matchers_(std::move(matchers)) {}

    void match(const SimpleSyntaxTree* tree) const override {
        for (const auto& matcher : matchers_)
            matcher->match(tree);
    }

private:
    std::vector<SyntaxTreeMatcherPtr> matchers_;
};

} // namespace

SyntaxTreeMatcherPtr combine(std::vector<SyntaxTreeMatcherPtr> matchers) {
    return std::make_unique<CombinedSyntaxTreeMatcher>(std::move(matchers));
}

SyntaxTreeMatcherPtr token_type(TokenType expected) {
    return std::make_unique<SyntaxTokenTypeMatcher>(expected);
}

SyntaxTreeMatcherPtr token(TokenType expected, std::string expected_text) {
    return combine({
        token_type(expected),
        std::make_unique<SyntaxTokenTextMatcher>(std::move(expected_text)),
    });
}

SyntaxTreeMatcherPtr node_type(SyntaxType expected) {
    return std::make_unique<SyntaxNodeTypeMatcher>(expected);
}

SyntaxTreeMatcherPtr node(SyntaxType expected, std::vector<SyntaxTreeMatcherPtr> children) {
    return combine({
        node_type(expected),
        std::make_unique<SyntaxNodeChildrenMatcher>(std::move(children)),
    });
}

SyntaxTreeMatcherPtr name(std::string name) {
    return node(SyntaxType::Name, {token(TokenType::Identifier, std::move(name))});
}

SyntaxTreeMatcherPtr arg_list(std::vector<SyntaxTreeMatcherPtr> args, bool optional) {
    std::vector<SyntaxTreeMatcherPtr> arg_list;
    arg_list.push_back(token_type(optional ? TokenType::QuestionLeftParen : TokenType::LeftParen));

    bool first_arg = true;
    for (auto& arg : args) {
        if (!first_arg)
            arg_list.push_back(token_type(TokenType::Comma));
        arg_list.push_back(std::move(arg));
        first_arg = false;
    }

    arg_list.push_back(token_type(TokenType::RightParen));

    return node(SyntaxType::ArgList, std::move(arg_list));
}

SyntaxTreeMatcherPtr param_list(std::vector<SyntaxTreeMatcherPtr> params) {
    std::vector<SyntaxTreeMatcherPtr> arg_list;
    arg_list.push_back(token_type(TokenType::LeftParen));

    bool first_arg = true;
    for (auto& arg : params) {
        if (!first_arg)
            arg_list.push_back(token_type(TokenType::Comma));
        arg_list.push_back(std::move(arg));
        first_arg = false;
    }

    arg_list.push_back(token_type(TokenType::RightParen));

    return node(SyntaxType::ParamList, std::move(arg_list));
}

SyntaxTreeMatcherPtr literal(TokenType expected) {
    return node(SyntaxType::Literal, {token_type(expected)});
}

SyntaxTreeMatcherPtr literal(TokenType expected, std::string text) {
    return node(SyntaxType::Literal, {token(expected, std::move(text))});
}

SyntaxTreeMatcherPtr unary_expr(TokenType op, SyntaxTreeMatcherPtr inner) {
    return node(SyntaxType::UnaryExpr, {token_type(op), std::move(inner)});
}

SyntaxTreeMatcherPtr binary_expr(TokenType op, SyntaxTreeMatcherPtr lhs, SyntaxTreeMatcherPtr rhs) {
    return node(SyntaxType::BinaryExpr, {std::move(lhs), token_type(op), std::move(rhs)});
}

SyntaxTreeMatcherPtr var_expr(std::string varname) {
    return node(SyntaxType::VarExpr, {token(TokenType::Identifier, std::move(varname))});
}

SyntaxTreeMatcherPtr field_expr(SyntaxTreeMatcherPtr obj, std::string field, bool optional) {
    return node(SyntaxType::FieldExpr, //
        {
            std::move(obj),
            token_type(optional ? TokenType::QuestionDot : TokenType::Dot),
            token(TokenType::Identifier, std::move(field)),
        });
}

SyntaxTreeMatcherPtr tuple_field_expr(SyntaxTreeMatcherPtr obj, i64 index, bool optional) {
    return node(SyntaxType::TupleFieldExpr, //
        {
            std::move(obj),
            token_type(optional ? TokenType::QuestionDot : TokenType::Dot),
            token(TokenType::TupleField, std::to_string(index)),
        });
}

SyntaxTreeMatcherPtr
index_expr(SyntaxTreeMatcherPtr obj, SyntaxTreeMatcherPtr index, bool optional) {
    return node(SyntaxType::IndexExpr, //
        {
            std::move(obj),
            token_type(optional ? TokenType::QuestionLeftBracket : TokenType::LeftBracket),
            std::move(index),
            token_type(TokenType::RightBracket),
        });
}

SyntaxTreeMatcherPtr
call_expr(SyntaxTreeMatcherPtr func, std::vector<SyntaxTreeMatcherPtr> args, bool optional) {
    return node(SyntaxType::CallExpr, {std::move(func), arg_list(std::move(args), optional)});
}

SyntaxTreeMatcherPtr string_content(std::string expected) {
    return token(TokenType::StringContent, std::move(expected));
}

SyntaxTreeMatcherPtr simple_string(std::string expected) {
    return node(SyntaxType::StringExpr, //
        {
            token_type(TokenType::StringStart),
            string_content(std::move(expected)),
            token_type(TokenType::StringEnd),
        });
}

SyntaxTreeMatcherPtr string_var(std::string var_name) {
    return node(SyntaxType::StringFormatItem, //
        {
            token_type(TokenType::StringVar), // $
            var_expr(std::move(var_name)),
        });
}

SyntaxTreeMatcherPtr string_block(SyntaxTreeMatcherPtr expr) {
    return node(SyntaxType::StringFormatBlock, //
        {
            token_type(TokenType::StringBlockStart), // ${
            std::move(expr),
            token_type(TokenType::StringBlockEnd), // }
        });
}

SyntaxTreeMatcherPtr full_string(std::vector<SyntaxTreeMatcherPtr> items) {
    std::vector<SyntaxTreeMatcherPtr> full_items;
    full_items.push_back(token_type(TokenType::StringStart));
    full_items.insert(full_items.end(), std::make_move_iterator(items.begin()),
        std::make_move_iterator(items.end()));
    full_items.push_back(token_type(TokenType::StringEnd));
    return node(SyntaxType::StringExpr, std::move(full_items));
}

SyntaxTreeMatcherPtr binding_name(std::string name) {
    return node(SyntaxType::BindingName, {token(TokenType::Identifier, std::move(name))});
}

SyntaxTreeMatcherPtr binding_tuple(std::vector<std::string> names) {
    std::vector<SyntaxTreeMatcherPtr> elems;
    elems.push_back(token_type(TokenType::LeftParen));

    bool first = true;
    for (auto& name : names) {
        if (!first)
            elems.push_back(token_type(TokenType::Comma));
        first = false;
        elems.push_back(token(TokenType::Identifier, std::move(name)));
    }

    elems.push_back(token_type(TokenType::RightParen));
    return node(SyntaxType::BindingTuple, std::move(elems));
}

SyntaxTreeMatcherPtr simple_binding(SyntaxTreeMatcherPtr elem) {
    return node(SyntaxType::Binding, {std::move(elem)});
}

SyntaxTreeMatcherPtr simple_binding(SyntaxTreeMatcherPtr elem, SyntaxTreeMatcherPtr init) {
    return node(SyntaxType::Binding, //
        {
            std::move(elem),
            token_type(TokenType::Equals),
            std::move(init),
        });
}

void assert_parse_tree(const SimpleSyntaxTree* actual, SyntaxTreeMatcherPtr expected) {
    INFO("Parse tree:\n" << dump_parse_tree(actual) << "\n");
    expected->match(actual);
}

} // namespace tiro::test
