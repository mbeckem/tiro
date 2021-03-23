#ifndef TIRO_COMPILER_AST_GEN_OPERATORS_HPP
#define TIRO_COMPILER_AST_GEN_OPERATORS_HPP

#include "compiler/ast/fwd.hpp"
#include "compiler/syntax/fwd.hpp"

#include <optional>

namespace tiro {

std::optional<UnaryOperator> to_unary_operator(TokenType t);

std::optional<BinaryOperator> to_binary_operator(TokenType t);

} // namespace tiro

#endif // TIRO_COMPILER_AST_GEN_OPERATORS_HPP
