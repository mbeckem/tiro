#ifndef TIRO_COMPILER_SYNTAX_PARSER_HPP
#define TIRO_COMPILER_SYNTAX_PARSER_HPP

#include "common/adt/not_null.hpp"
#include "common/adt/span.hpp"
#include "common/assert.hpp"
#include "common/defs.hpp"
#include "compiler/syntax/parser_event.hpp"
#include "compiler/syntax/syntax_type.hpp"
#include "compiler/syntax/token.hpp"
#include "compiler/syntax/token_set.hpp"

#include <optional>
#include <vector>

namespace tiro::next {

class Parser final {
public:
    class Marker;
    class CompletedMarker;

    explicit Parser(Span<const Token> tokens);

    Parser(const Parser&) = delete;
    Parser& operator=(const Parser&) = delete;

    /// Returns the type of the current token.
    TokenType current() const;

    /// Returns the token type of the nth token from the current position.
    /// `ahead(0)` is equivalent to `current()`.
    TokenType ahead(size_t n) const;

    /// Returns true iff `current() == type`.
    bool at(TokenType type) const;

    /// Unconditionally advances to the next token.
    void advance();

    /// Advances to the next token if the current token's type matches `type`.
    /// Returns true if the parser advanced.
    bool accept(TokenType type);

    /// Advances to the next token if the current token's type matches `type`.
    /// Returns the matched token type if the parser advanced.
    std::optional<TokenType> accept_any(const TokenSet& tokens);

    /// Finishes parsing and returns the vector of events by move.
    std::vector<ParserEvent> take_events();

private:
    friend Marker;
    friend CompletedMarker;

private:
    Span<const Token> tokens_;
    size_t pos_ = 0;
    std::vector<ParserEvent> events_;
};

class Parser::Marker final {
public:
    /// Marks the current syntax node as completed.
    /// The returned marker may be used to wrap the node with a new parent.
    ///
    /// This marker must not be used anymore after it has been completed.
    CompletedMarker complete(SyntaxType type);

    /// Abandons the current node.
    /// All its children will become children of the parent instead.
    ///
    /// This marker must not be used anymore after it has been abandoned.
    void abandon();

private:
    friend Parser;

    explicit Marker(NotNull<Parser*> parser, size_t start);

private:
    // The owning parser. Set to nullptr if completed or abandoned.
    Parser* parser_;

    // Position of the tombstone node. When the marker is completed,
    // that tombstone node will become a start node with the actual node type.
    size_t start_;
};

class Parser::CompletedMarker final {
private:
    friend Marker;

    explicit CompletedMarker(NotNull<Parser*> parser, size_t start, size_t end);

private:
    Parser* parser_;
    size_t start_; // Points to the start node
    size_t end_;   // Points to the finish node
};

} // namespace tiro::next

#endif // TIRO_COMPILER_SYNTAX_PARSER_HPP
