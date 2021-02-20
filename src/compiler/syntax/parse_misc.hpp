#ifndef TIRO_COMPILER_SYNTAX_PARSE_MISC_HPP
#define TIRO_COMPILER_SYNTAX_PARSE_MISC_HPP

#include "compiler/syntax/fwd.hpp"

namespace tiro::next {

extern const TokenSet VAR_DECL_FIRST;
extern const TokenSet BINDING_PATTERN_FIRST;

/// Parses function call arguments.
void parse_arg_list(Parser& p, const TokenSet& recovery);

/// Parses a variable declaration, e.g. `var a = 1, (b, c) = foo()`.
void parse_var_decl(Parser& p, const TokenSet& recovery);

/// Parses a binding pattern, the left hand side of a variable declaration.
void parse_binding_pattern(Parser& p, const TokenSet& recovery);

/// Parses a complete binding, i.e `pattern = expr`,
void parse_binding(Parser& p, const TokenSet& recovery);

} // namespace tiro::next

#endif // TIRO_COMPILER_SYNTAX_PARSE_MISC_HPP
