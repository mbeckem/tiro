#include "compiler/syntax/parser.hpp"

#include <utility>

namespace tiro::next {

static const TokenSet SKIP_CONSUME_ON_ERROR = {
    TokenType::LeftBrace,
    TokenType::RightBrace,
    TokenType::StringBlockStart,
    TokenType::StringBlockEnd,
};

Parser::Parser(Span<const Token> tokens)
    : tokens_(tokens) {
    events_.reserve(tokens.size());
}

TokenType Parser::current() const {
    return ahead(0);
}

TokenType Parser::ahead(size_t n) const {
    if (n >= tokens_.size() - pos_)
        return TokenType::Eof;

    return tokens_[pos_ + n].type();
}

bool Parser::at(TokenType type) const {
    return current() == type;
}

bool Parser::at_any(const TokenSet& tokens) const {
    return tokens.contains(current());
}

void Parser::advance() {
    if (pos_ >= tokens_.size())
        return;

    events_.emplace_back(tokens_[pos_++]);
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

    // TODO: Better error messages
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

Parser::Marker Parser::start() {
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

Parser::Marker::Marker(Parser& parser, size_t start)
    : parser_(&parser)
    , start_(start) {
    TIRO_DEBUG_ASSERT(start < parser_->events_.size(), "start index out of bounds.");
    TIRO_DEBUG_ASSERT(parser_->events_[start_].type() == ParserEventType::Tombstone,
        "Incomplete markers must point to a tombstone event.");
}

Parser::CompletedMarker Parser::Marker::complete(SyntaxType type) {
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

void Parser::Marker::abandon() {
    TIRO_DEBUG_ASSERT(parser_, "Marker has already been finished.");

    auto& events = parser_->events_;
    if (start_ == events.size() - 1) {
        auto& start_event = events[start_];
        TIRO_DEBUG_ASSERT(start_event.type() == ParserEventType::Tombstone,
            "Incomplete markers must point to a tombstone event.");
        events.pop_back();
    }

    parser_ = nullptr;
}

Parser::CompletedMarker::CompletedMarker(Parser& parser, size_t start, [[maybe_unused]] size_t end)
    : parser_(&parser)
    , start_(start) {}

Parser::Marker Parser::CompletedMarker::precede() {
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

} // namespace tiro::next
