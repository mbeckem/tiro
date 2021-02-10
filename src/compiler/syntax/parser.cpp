#include "compiler/syntax/parser.hpp"

#include <utility>

namespace tiro::next {

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

std::vector<ParserEvent> Parser::take_events() {
    return std::move(events_);
}

Parser::Marker::Marker(NotNull<Parser*> parser, size_t start)
    : parser_(parser)
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
    return CompletedMarker(TIRO_NN(std::exchange(parser_, nullptr)), start_, end);
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

Parser::CompletedMarker::CompletedMarker(NotNull<Parser*> parser, size_t start, size_t end)
    : parser_(parser)
    , start_(start)
    , end_(end) {}

} // namespace tiro::next
