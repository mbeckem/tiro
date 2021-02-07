#ifndef TIRO_COMPILER_SYNTAX_FWD_HPP
#define TIRO_COMPILER_SYNTAX_FWD_HPP

#include "common/defs.hpp"

namespace tiro::next {

class Lexer;
class SourceRange;

enum class TokenType : u8;
class Token;
class TokenSet;

} // namespace tiro::next

#endif // TIRO_COMPILER_SYNTAX_FWD_HPP
