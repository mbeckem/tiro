#ifndef TIRO_TEST_COMPILER_AST_GEN_SIMPLE_AST_HPP
#define TIRO_TEST_COMPILER_AST_GEN_SIMPLE_AST_HPP

#include "common/text/string_table.hpp"
#include "compiler/ast/fwd.hpp"
#include "compiler/ast/node.hpp"

#include <string_view>
#include <vector>

namespace tiro::test {

template<typename T>
struct SimpleAst {
    StringTable strings;
    AstPtr<T> root;
};

SimpleAst<AstExpr> parse_expr_ast(std::string_view source);
SimpleAst<AstStmt> parse_stmt_ast(std::string_view source);
SimpleAst<AstStmt> parse_item_ast(std::string_view source);
SimpleAst<AstFile> parse_file_ast(std::string_view source);
SimpleAst<AstModule> parse_module_ast(const std::vector<std::string_view>& sources);

} // namespace tiro::test

#endif // TIRO_TEST_COMPILER_AST_GEN_SIMPLE_AST_HPP
