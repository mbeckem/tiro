#ifndef TIRO_TEST_COMPILER_SYNTAX_SYNTAX_ASSERT_HPP
#define TIRO_TEST_COMPILER_SYNTAX_SYNTAX_ASSERT_HPP

#include "compiler/syntax/fwd.hpp"
#include "compiler/syntax/syntax_type.hpp"
#include "compiler/syntax/token.hpp"

#include "./simple_syntax_tree.hpp"

#include <memory>
#include <string>
#include <vector>

namespace tiro::test {

class SyntaxTreeMatcher {
public:
    virtual ~SyntaxTreeMatcher() {}

    virtual void match(const SimpleSyntaxTree* tree) const = 0;
};

using SyntaxTreeMatcherPtr = std::shared_ptr<SyntaxTreeMatcher>;

SyntaxTreeMatcherPtr combine(std::vector<SyntaxTreeMatcherPtr> matchers);

SyntaxTreeMatcherPtr token_type(TokenType expected);

SyntaxTreeMatcherPtr token(TokenType expected, std::string expected_text);

SyntaxTreeMatcherPtr node_type(SyntaxType expected);

SyntaxTreeMatcherPtr node(SyntaxType expected, std::vector<SyntaxTreeMatcherPtr> children);

SyntaxTreeMatcherPtr name(std::string name);

SyntaxTreeMatcherPtr arg_list(std::vector<SyntaxTreeMatcherPtr> args, bool optional = false);

SyntaxTreeMatcherPtr param_list(std::vector<SyntaxTreeMatcherPtr> params);

SyntaxTreeMatcherPtr literal(TokenType expected);

SyntaxTreeMatcherPtr literal(TokenType expected, std::string text);

SyntaxTreeMatcherPtr unary_expr(TokenType op, SyntaxTreeMatcherPtr inner);

SyntaxTreeMatcherPtr binary_expr(TokenType op, SyntaxTreeMatcherPtr lhs, SyntaxTreeMatcherPtr rhs);

SyntaxTreeMatcherPtr var_expr(std::string varname);

SyntaxTreeMatcherPtr field_expr(SyntaxTreeMatcherPtr obj, std::string field, bool optional = false);

SyntaxTreeMatcherPtr tuple_field_expr(SyntaxTreeMatcherPtr obj, i64 index, bool optional = false);

SyntaxTreeMatcherPtr
index_expr(SyntaxTreeMatcherPtr obj, SyntaxTreeMatcherPtr index, bool optional = false);

SyntaxTreeMatcherPtr
call_expr(SyntaxTreeMatcherPtr func, std::vector<SyntaxTreeMatcherPtr> args, bool optional = false);

SyntaxTreeMatcherPtr string_content(std::string expected);

SyntaxTreeMatcherPtr simple_string(std::string expected);

SyntaxTreeMatcherPtr string_var(std::string var_name);

SyntaxTreeMatcherPtr string_block(SyntaxTreeMatcherPtr expr);

SyntaxTreeMatcherPtr full_string(std::vector<SyntaxTreeMatcherPtr> items);

SyntaxTreeMatcherPtr binding_name(std::string name);

SyntaxTreeMatcherPtr binding_tuple(std::vector<std::string> names);

SyntaxTreeMatcherPtr simple_binding(SyntaxTreeMatcherPtr elem);

SyntaxTreeMatcherPtr simple_binding(SyntaxTreeMatcherPtr elem, SyntaxTreeMatcherPtr init);

void assert_parse_tree(const SimpleSyntaxTree* actual, SyntaxTreeMatcherPtr expected);

inline void
assert_parse_tree(const std::unique_ptr<SimpleSyntaxTree>& actual, SyntaxTreeMatcherPtr expected) {
    return assert_parse_tree(actual.get(), expected);
}

} // namespace tiro::test

#endif // TIRO_TEST_COMPILER_SYNTAX_SYNTAX_ASSERT_HPP
