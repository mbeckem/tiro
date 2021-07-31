#ifndef TIRO_TEST_COMPILER_SYNTAX_SIMPLE_SYNTAX_TREE_HPP
#define TIRO_TEST_COMPILER_SYNTAX_SIMPLE_SYNTAX_TREE_HPP

#include "common/text/string_utils.hpp"
#include "compiler/syntax/fwd.hpp"
#include "compiler/syntax/syntax_type.hpp"
#include "compiler/syntax/token.hpp"

#include <vector>

namespace tiro::test {

// Thrown when parse_* encounters an error
class BadSyntax final : public std::runtime_error {
public:
    using std::runtime_error::runtime_error;
};

class SimpleSyntaxTree {
public:
    enum Kind { TOKEN, NODE };

    const Kind kind;

    SimpleSyntaxTree(Kind kind_)
        : kind(kind_) {}

    virtual ~SimpleSyntaxTree() {}

    virtual std::string to_string() const = 0;
};

class SimpleSyntaxToken final : public SimpleSyntaxTree {
public:
    TokenType type;
    std::string text;

public:
    SimpleSyntaxToken(TokenType type_, std::string text_)
        : SimpleSyntaxTree(TOKEN)
        , type(std::move(type_))
        , text(std::move(text_)) {}

    std::string to_string() const override {
        return fmt::format("Token: {} \"{}\"", type, escape_string(text));
    }
};

class SimpleSyntaxNode final : public SimpleSyntaxTree {
public:
    SyntaxType type;
    std::vector<std::unique_ptr<SimpleSyntaxTree>> children;

public:
    SimpleSyntaxNode(SyntaxType type_)
        : SimpleSyntaxTree(NODE)
        , type(type_) {}

    std::string to_string() const override { return fmt::format("Node: {}", type); }
};

std::string dump_parse_tree(const SimpleSyntaxTree* root);

std::unique_ptr<SimpleSyntaxTree> parse_expr_syntax(std::string_view source);
std::unique_ptr<SimpleSyntaxTree> parse_stmt_syntax(std::string_view source);
std::unique_ptr<SimpleSyntaxTree> parse_item_syntax(std::string_view source);
std::unique_ptr<SimpleSyntaxTree> parse_file_syntax(std::string_view source);

} // namespace tiro::test

#endif // TIRO_TEST_COMPILER_SYNTAX_SIMPLE_SYNTAX_TREE_HPP
