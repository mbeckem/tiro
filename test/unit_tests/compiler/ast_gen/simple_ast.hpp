#ifndef TIRO_TEST_COMPILER_AST_GEN_SIMPLE_AST_HPP
#define TIRO_TEST_COMPILER_AST_GEN_SIMPLE_AST_HPP

#include "common/text/string_table.hpp"
#include "compiler/ast/fwd.hpp"
#include "compiler/ast/node.hpp"

namespace tiro::next::test {

template<typename T>
struct SimpleAst {
    StringTable strings;
    AstPtr<T> root;
};

SimpleAst<AstNode> parse_expr_ast(std::string_view source);
SimpleAst<AstNode> parse_stmt_ast(std::string_view source);
SimpleAst<AstNode> parse_item_ast(std::string_view source);
SimpleAst<AstNode> parse_file_ast(std::string_view source);

} // namespace tiro::next::test

#endif // TIRO_TEST_COMPILER_AST_GEN_SIMPLE_AST_HPP
