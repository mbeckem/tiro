#ifndef TIRO_COMPILER_SYNTAX_GRAMMAR_LITERALS_HPP
#define TIRO_COMPILER_SYNTAX_GRAMMAR_LITERALS_HPP

#include "common/defs.hpp"
#include "common/text/unicode.hpp"

#include <optional>

namespace tiro::next {

/// Attempts to parse the given code point as a digit with the given base.
/// Base must be 2, 8, 10 or 16.
std::optional<int> to_digit(CodePoint c, int base);

} // namespace tiro::next

#endif // TIRO_COMPILER_SYNTAX_GRAMMAR_LITERALS_HPP
