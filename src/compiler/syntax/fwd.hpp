#ifndef TIRO_COMPILER_SYNTAX_FWD_HPP
#define TIRO_COMPILER_SYNTAX_FWD_HPP

#include "common/defs.hpp"

namespace tiro::next {

class Lexer;
class SourceRange;

enum class FunctionKind : u8;

enum class TokenType : u8;
class Token;
class TokenSet;

enum class ParserEventType : u8;
class ParserEvent;

class ParserEventConsumer;

class Parser;
class Marker;
class CompletedMarker;

enum class SyntaxType : u8;

class SyntaxNodeId;
class SyntaxNode;
class SyntaxError;
class SyntaxTree;

enum class SyntaxChildType : u8;
class SyntaxChild;

} // namespace tiro::next

#endif // TIRO_COMPILER_SYNTAX_FWD_HPP
