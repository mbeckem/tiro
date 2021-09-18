#include "compiler/syntax/parser.hpp"

#include "common/error.hpp"

#include <utility>

namespace tiro {

static const TokenSet SKIP_CONSUME_ON_ERROR = {
    TokenType::LeftBrace,
    TokenType::RightBrace,
    TokenType::StringBlockStart,
    TokenType::StringBlockEnd,
};

Parser::Parser(std::string_view source, Span<const Token> tokens)
    : source_(source)
    , tokens_(tokens) {
    events_.reserve(tokens.size());
}

TokenType Parser::current() {
    return ahead(0);
}

TokenType Parser::ahead(size_t n) {
    on_inspection();

    if (n >= tokens_.size() - pos_)
        return TokenType::Eof;

    return tokens_[pos_ + n].type();
}

bool Parser::at(TokenType type) {
    return current() == type;
}

bool Parser::at_any(const TokenSet& tokens) {
    return tokens.contains(current());
}

bool Parser::at_source(std::string_view text) {
    on_inspection();

    auto range = pos_ < tokens_.size() ? tokens_[pos_].range() : SourceRange();
    return substring(source_, range) == text;
}

void Parser::advance() {
    if (pos_ >= tokens_.size())
        return;

    inspections_ = 0;
    events_.emplace_back(tokens_[pos_++]);
}

void Parser::advance_with_type(TokenType type) {
    if (pos_ >= tokens_.size())
        return;

    inspections_ = 0;
    const Token& current = tokens_[pos_++];
    events_.emplace_back(Token(type, current.range()));
}

bool Parser::accept(TokenType type) {
    if (at(type)) {
        advance();
        return true;
    }
    return false;
}

std::optional<TokenType> Parser::accept_any(const TokenSet& tokens) {
    auto type = current();
    if (tokens.contains(type)) {
        advance();
        return type;
    }
    return {};
}

bool Parser::expect(TokenType type) {
    if (accept(type))
        return true;

    error(fmt::format("expected {}", to_description(type)));
    return false;
}

void Parser::error_recover(std::string message, const TokenSet& recovery) {
    if (at_any(SKIP_CONSUME_ON_ERROR) || at_any(recovery)) {
        error(std::move(message));
        return;
    }

    auto m = start();
    error(std::move(message));
    advance();
    m.complete(SyntaxType::Error);
}

void Parser::error(std::string message) {
    events_.push_back(ParserEvent::make_error(std::move(message)));
}

void Parser::on_inspection() {
    if (++inspections_ >= 1024) {
        TIRO_ERROR(
            "The parser appears to be stuck. Please report this issue together with a source code "
            "snippet that reproduces the bug.");
    }
}

Marker Parser::start() {
    size_t start_pos = events_.size();
    events_.push_back(ParserEvent::make_tombstone());
    return Marker(*this, start_pos);
}

Span<const ParserEvent> Parser::events() const {
    return events_;
}

std::vector<ParserEvent> Parser::take_events() {
    return std::move(events_);
}

Marker::Marker(Parser& parser, size_t start)
    : parser_(&parser)
    , start_(start) {
    TIRO_DEBUG_ASSERT(start < parser_->events_.size(), "start index out of bounds.");
    TIRO_DEBUG_ASSERT(parser_->events_[start_].type() == ParserEventType::Tombstone,
        "Incomplete markers must point to a tombstone event.");
}

CompletedMarker Marker::complete(SyntaxType type) {
    TIRO_DEBUG_ASSERT(parser_, "Marker has already been finished.");

    auto& events = parser_->events_;
    auto& start_event = events[start_];
    TIRO_DEBUG_ASSERT(start_event.type() == ParserEventType::Tombstone,
        "Incomplete markers must point to a tombstone event.");
    start_event = ParserEvent::make_start(type, 0);

    size_t end = events.size();
    events.push_back(ParserEvent::make_finish());
    return CompletedMarker(*(std::exchange(parser_, nullptr)), start_, end);
}

void Marker::abandon() {
    TIRO_DEBUG_ASSERT(parser_, "Marker has already been finished.");

    auto& events = parser_->events_;
    if (start_ == events.size() - 1) {
        [[maybe_unused]] auto& start_event = events[start_];
        TIRO_DEBUG_ASSERT(start_event.type() == ParserEventType::Tombstone,
            "Incomplete markers must point to a tombstone event.");
        events.pop_back();
    }

    parser_ = nullptr;
}

CompletedMarker::CompletedMarker(Parser& parser, size_t start, [[maybe_unused]] size_t end)
    : parser_(&parser)
    , start_(start) {}

Marker CompletedMarker::precede() {
    TIRO_DEBUG_ASSERT(parser_, "CompletedMarker has been invalidated.");

    auto m = parser_->start();

    // Register m's start event as the forward parent of the current node.
    auto& events = parser_->events_;
    auto& start_event = events[start_];
    TIRO_DEBUG_ASSERT(
        start_event.as_start().forward_parent == 0, "Node must not already have a forward parent.");
    start_event.as_start().forward_parent = m.start_;

    parser_ = nullptr;
    return m;
}

} // namespace tiro
