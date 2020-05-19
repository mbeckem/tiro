#ifndef TIRO_PARSER_FWD_HPP
#define TIRO_PARSER_FWD_HPP

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

#endif // TIRO_PARSER_FWD_HPP
