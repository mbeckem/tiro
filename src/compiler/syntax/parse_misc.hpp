#ifndef TIRO_COMPILER_SYNTAX_PARSE_MISC_HPP
#define TIRO_COMPILER_SYNTAX_PARSE_MISC_HPP

#include "compiler/syntax/fwd.hpp"

namespace tiro::next {

// Parses function call arguments.
void parse_arg_list(Parser& p, const TokenSet& recovery);

} // namespace tiro::next

#endif // TIRO_COMPILER_SYNTAX_PARSE_MISC_HPP
