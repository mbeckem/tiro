#include <catch2/catch.hpp>

#include "compiler/syntax/build_syntax_tree.hpp"
#include "compiler/syntax/parser.hpp"

using namespace tiro;
using namespace tiro::next;

static SyntaxNodeId must_node_id(const SyntaxChild& child) {
    if (child.type() != SyntaxChildType::NodeId)
        throw std::logic_error("Expected a node id.");
    return child.as_node_id();
}

static Token must_token(const SyntaxChild& child) {
    if (child.type() != SyntaxChildType::Token)
        throw std::logic_error("Expected a token.");
    return child.as_token();
}

TEST_CASE("Syntax tree should reflect the parser events", "[syntax]") {
    std::vector<ParserEvent> events;
    auto push = [&](auto&& event) { events.push_back(std::move(event)); };

    push(ParserEvent::make_start(SyntaxType::BinaryExpr, 0));

    push(ParserEvent::make_start(SyntaxType::Literal, 0));
    push(ParserEvent::make_token(Token(TokenType::Integer, SourceRange(0, 5))));
    push(ParserEvent::make_finish()); // Literal

    push(ParserEvent::make_token(Token(TokenType::Plus, SourceRange(5, 6))));

    push(ParserEvent::make_start(SyntaxType::VarExpr, 0));
    push(ParserEvent::make_token(Token(TokenType::Identifier, SourceRange(6, 7))));
    push(ParserEvent::make_finish()); // VarExpr

    push(ParserEvent::make_error("WHOOPS!"));
    push(ParserEvent::make_finish()); // BinaryExpr

    SyntaxTree tree = build_syntax_tree("", events);
    REQUIRE(tree.root_id());

    const auto root_id = tree.root_id();
    const auto root_data = tree[tree.root_id()];
    REQUIRE(root_data->type() == SyntaxType::Root);
    REQUIRE(!root_data->parent());
    REQUIRE((root_data->range() == SourceRange(0, 7)));
    REQUIRE(root_data->errors().empty());
    REQUIRE(root_data->children().size() == 1); // Single BinaryExpr

    const auto binary_id = must_node_id(root_data->children()[0]);
    const auto binary_data = tree[binary_id];
    REQUIRE(binary_data->type() == SyntaxType::BinaryExpr);
    REQUIRE(binary_data->parent() == root_id);
    REQUIRE((binary_data->range() == root_data->range()));
    REQUIRE(binary_data->errors().size() == 1);
    REQUIRE(binary_data->errors()[0] == "WHOOPS!");
    REQUIRE(binary_data->children().size() == 3); // Two operands and a plus operator

    const auto literal_id = must_node_id(binary_data->children()[0]);
    const auto literal_data = tree[literal_id];
    REQUIRE(literal_data->type() == SyntaxType::Literal);
    REQUIRE(literal_data->parent() == binary_id);
    REQUIRE((literal_data->range() == SourceRange(0, 5)));
    REQUIRE(literal_data->errors().empty());
    REQUIRE(literal_data->children().size() == 1);
    REQUIRE(must_token(literal_data->children()[0]).type() == TokenType::Integer);

    const auto plus = must_token(binary_data->children()[1]);
    REQUIRE(plus.type() == TokenType::Plus);
    REQUIRE((plus.range() == SourceRange(5, 6)));

    const auto var_id = must_node_id(binary_data->children()[2]);
    const auto var_data = tree[var_id];
    REQUIRE(var_data->type() == SyntaxType::VarExpr);
    REQUIRE(var_data->parent() == binary_id);
    REQUIRE((var_data->range() == SourceRange(6, 7)));
    REQUIRE(var_data->errors().empty());
    REQUIRE(var_data->children().size() == 1);
    REQUIRE(must_token(var_data->children()[0]).type() == TokenType::Identifier);
}
