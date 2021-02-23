#ifndef TIRO_TEST_COMPILER_SYNTAX_SYNTAX_TREE_HPP
#define TIRO_TEST_COMPILER_SYNTAX_SYNTAX_TREE_HPP

#include "common/text/string_utils.hpp"
#include "compiler/syntax/fwd.hpp"
#include "compiler/syntax/syntax_type.hpp"
#include "compiler/syntax/token.hpp"

namespace tiro::next::test {

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

std::string dump_parse_tree(const SyntaxTree* root);

std::unique_ptr<SyntaxTree> parse_expr_syntax(std::string_view source);
std::unique_ptr<SyntaxTree> parse_stmt_syntax(std::string_view source);
std::unique_ptr<SyntaxTree> parse_item_syntax(std::string_view source);
std::unique_ptr<SyntaxTree> parse_file_syntax(std::string_view source);

} // namespace tiro::next::test

#endif // TIRO_TEST_COMPILER_SYNTAX_SYNTAX_TREE_HPP
