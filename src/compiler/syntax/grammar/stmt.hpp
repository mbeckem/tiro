#ifndef TIRO_COMPILER_SYNTAX_GRAMMAR_STMT
#define TIRO_COMPILER_SYNTAX_GRAMMAR_STMT

#include "compiler/syntax/fwd.hpp"

#include <optional>

namespace tiro::next {

extern const TokenSet STMT_FIRST;

void parse_stmt(Parser& p, const TokenSet& recovery);

} // namespace tiro::next

#endif // TIRO_COMPILER_SYNTAX_GRAMMAR_STMT
