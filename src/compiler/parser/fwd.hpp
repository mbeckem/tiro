#ifndef TIRO_COMPILER_PARSER_FWD_HPP
#define TIRO_COMPILER_PARSER_FWD_HPP

namespace tiro {

class Parser;
class Lexer;

enum class LexerMode;

template<typename Node>
class ParseResult;

template<typename Node>
struct PartialSyntaxError;

struct EmptySyntaxError;

} // namespace tiro

#endif // TIRO_COMPILER_PARSER_FWD_HPP
