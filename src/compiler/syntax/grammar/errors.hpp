#ifndef TIRO_COMPILER_SYNTAX_GRAMMAR_ERRORS_HPP
#define TIRO_COMPILER_SYNTAX_GRAMMAR_ERRORS_HPP

#include "compiler/syntax/fwd.hpp"

namespace tiro {

extern const TokenSet NESTING_START;
extern const TokenSet NESTING_END;

/// Discard the entire content of a block delimited by { ... } or "".
/// Nested blocks will be discarded as well until the final end token is found, which is consumed as well.
///
/// Parser expects to be positioned at a { or ", which can be checked by using NESTING_START.
void discard_nested(Parser& p);

/// Discards tokens until one in `recovery` is found. Handles (and discards) nested blocks as well.
/// Note that the recovery token is *not* consumed.
///
/// The algorithm stops if
/// - the end of file is reached
/// - the next token is in `recovery`
void discard_input(Parser& p, const TokenSet& recovery);

} // namespace tiro

#endif // TIRO_COMPILER_SYNTAX_GRAMMAR_ERRORS_HPP
