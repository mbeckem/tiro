#include "compiler/syntax/parser_event.hpp"

#include "common/iter_tools.hpp"

#include <absl/container/inlined_vector.h>

namespace tiro::next {

/* [[[cog
    from cog import outl
    from codegen.unions import implement
    from codegen.syntax import ParserEvent
    implement(ParserEvent.tag)
    outl()
    implement(ParserEvent)
]]] */
std::string_view to_string(ParserEventType type) {
    switch (type) {
    case ParserEventType::Tombstone:
        return "Tombstone";
    case ParserEventType::Start:
        return "Start";
    case ParserEventType::Finish:
        return "Finish";
    case ParserEventType::Token:
        return "Token";
    case ParserEventType::Error:
        return "Error";
    }
    TIRO_UNREACHABLE("Invalid ParserEventType.");
}

ParserEvent ParserEvent::make_tombstone() {
    return {Tombstone{}};
}

ParserEvent ParserEvent::make_start(const SyntaxType& type, const size_t& forward_parent) {
    return {Start{type, forward_parent}};
}

ParserEvent ParserEvent::make_finish() {
    return {Finish{}};
}

ParserEvent ParserEvent::make_token(const Token& token) {
    return token;
}

ParserEvent ParserEvent::make_error(std::string message) {
    return {Error{std::move(message)}};
}

ParserEvent::ParserEvent(Tombstone tombstone)
    : type_(ParserEventType::Tombstone)
    , tombstone_(std::move(tombstone)) {}

ParserEvent::ParserEvent(Start start)
    : type_(ParserEventType::Start)
    , start_(std::move(start)) {}

ParserEvent::ParserEvent(Finish finish)
    : type_(ParserEventType::Finish)
    , finish_(std::move(finish)) {}

ParserEvent::ParserEvent(Token token)
    : type_(ParserEventType::Token)
    , token_(std::move(token)) {}

ParserEvent::ParserEvent(Error error)
    : type_(ParserEventType::Error)
    , error_(std::move(error)) {}

ParserEvent::~ParserEvent() {
    _destroy_value();
}

static_assert(
    std::is_nothrow_move_constructible_v<
        ParserEvent::Tombstone> && std::is_nothrow_move_assignable_v<ParserEvent::Tombstone>,
    "Only nothrow movable types are supported in generated unions.");
static_assert(std::is_nothrow_move_constructible_v<
                  ParserEvent::Start> && std::is_nothrow_move_assignable_v<ParserEvent::Start>,
    "Only nothrow movable types are supported in generated unions.");
static_assert(std::is_nothrow_move_constructible_v<
                  ParserEvent::Finish> && std::is_nothrow_move_assignable_v<ParserEvent::Finish>,
    "Only nothrow movable types are supported in generated unions.");
static_assert(std::is_nothrow_move_constructible_v<
                  ParserEvent::Token> && std::is_nothrow_move_assignable_v<ParserEvent::Token>,
    "Only nothrow movable types are supported in generated unions.");
static_assert(std::is_nothrow_move_constructible_v<
                  ParserEvent::Error> && std::is_nothrow_move_assignable_v<ParserEvent::Error>,
    "Only nothrow movable types are supported in generated unions.");

ParserEvent::ParserEvent(ParserEvent&& other) noexcept
    : type_(other.type()) {
    _move_construct_value(other);
}

ParserEvent& ParserEvent::operator=(ParserEvent&& other) noexcept {
    TIRO_DEBUG_ASSERT(this != &other, "Self move assignment is invalid.");
    if (type() == other.type()) {
        _move_assign_value(other);
    } else {
        _destroy_value();
        _move_construct_value(other);
        type_ = other.type();
    }
    return *this;
}

const ParserEvent::Tombstone& ParserEvent::as_tombstone() const {
    TIRO_DEBUG_ASSERT(
        type_ == ParserEventType::Tombstone, "Bad member access on ParserEvent: not a Tombstone.");
    return tombstone_;
}

ParserEvent::Tombstone& ParserEvent::as_tombstone() {
    return const_cast<Tombstone&>(const_cast<const ParserEvent*>(this)->as_tombstone());
}

const ParserEvent::Start& ParserEvent::as_start() const {
    TIRO_DEBUG_ASSERT(
        type_ == ParserEventType::Start, "Bad member access on ParserEvent: not a Start.");
    return start_;
}

ParserEvent::Start& ParserEvent::as_start() {
    return const_cast<Start&>(const_cast<const ParserEvent*>(this)->as_start());
}

const ParserEvent::Finish& ParserEvent::as_finish() const {
    TIRO_DEBUG_ASSERT(
        type_ == ParserEventType::Finish, "Bad member access on ParserEvent: not a Finish.");
    return finish_;
}

ParserEvent::Finish& ParserEvent::as_finish() {
    return const_cast<Finish&>(const_cast<const ParserEvent*>(this)->as_finish());
}

const ParserEvent::Token& ParserEvent::as_token() const {
    TIRO_DEBUG_ASSERT(
        type_ == ParserEventType::Token, "Bad member access on ParserEvent: not a Token.");
    return token_;
}

ParserEvent::Token& ParserEvent::as_token() {
    return const_cast<Token&>(const_cast<const ParserEvent*>(this)->as_token());
}

const ParserEvent::Error& ParserEvent::as_error() const {
    TIRO_DEBUG_ASSERT(
        type_ == ParserEventType::Error, "Bad member access on ParserEvent: not a Error.");
    return error_;
}

ParserEvent::Error& ParserEvent::as_error() {
    return const_cast<Error&>(const_cast<const ParserEvent*>(this)->as_error());
}

void ParserEvent::format(FormatStream& stream) const {
    struct FormatVisitor {
        FormatStream& stream;

        void visit_tombstone([[maybe_unused]] const Tombstone& tombstone) {
            stream.format("Tombstone");
        }

        void visit_start([[maybe_unused]] const Start& start) {
            stream.format("Start(type: {}, forward_parent: {})", start.type, start.forward_parent);
        }

        void visit_finish([[maybe_unused]] const Finish& finish) { stream.format("Finish"); }

        void visit_token([[maybe_unused]] const Token& token) { stream.format("{}", token); }

        void visit_error([[maybe_unused]] const Error& error) {
            stream.format("Error(message: {})", error.message);
        }
    };
    visit(FormatVisitor{stream});
}

void ParserEvent::_destroy_value() noexcept {
    struct DestroyVisitor {
        void visit_tombstone(Tombstone& tombstone) { tombstone.~Tombstone(); }

        void visit_start(Start& start) { start.~Start(); }

        void visit_finish(Finish& finish) { finish.~Finish(); }

        void visit_token(Token& token) { token.~Token(); }

        void visit_error(Error& error) { error.~Error(); }
    };
    visit(DestroyVisitor{});
}

void ParserEvent::_move_construct_value(ParserEvent& other) noexcept {
    struct ConstructVisitor {
        ParserEvent* self;

        void visit_tombstone(Tombstone& tombstone) {
            new (&self->tombstone_) Tombstone(std::move(tombstone));
        }

        void visit_start(Start& start) { new (&self->start_) Start(std::move(start)); }

        void visit_finish(Finish& finish) { new (&self->finish_) Finish(std::move(finish)); }

        void visit_token(Token& token) { new (&self->token_) Token(std::move(token)); }

        void visit_error(Error& error) { new (&self->error_) Error(std::move(error)); }
    };
    other.visit(ConstructVisitor{this});
}

void ParserEvent::_move_assign_value(ParserEvent& other) noexcept {
    struct AssignVisitor {
        ParserEvent* self;

        void visit_tombstone(Tombstone& tombstone) { self->tombstone_ = std::move(tombstone); }

        void visit_start(Start& start) { self->start_ = std::move(start); }

        void visit_finish(Finish& finish) { self->finish_ = std::move(finish); }

        void visit_token(Token& token) { self->token_ = std::move(token); }

        void visit_error(Error& error) { self->error_ = std::move(error); }
    };
    other.visit(AssignVisitor{this});
}

// [[[end]]]

ParserEventConsumer::~ParserEventConsumer() {}

void consume_events(Span<ParserEvent> events, ParserEventConsumer& consumer) {
    struct EventVisitor final {
        Span<ParserEvent> events;
        ParserEventConsumer& consumer;
        absl::InlinedVector<SyntaxType, 64> parents;

        EventVisitor(Span<ParserEvent> events_, ParserEventConsumer& consumer_)
            : events(events_)
            , consumer(consumer_) {}

        void visit_tombstone(ParserEvent::Tombstone&) {}

        void visit_start(ParserEvent::Start& start) {
            // Common case, no forward parents involved.
            if (start.forward_parent == 0) {
                consumer.start_node(start.type);
                return;
            }

            parents.push_back(start.type);
            size_t parent = start.forward_parent;
            while (parent) {
                ParserEvent& parent_event = events[parent];
                TIRO_DEBUG_ASSERT(parent_event.type() == ParserEventType::Start
                                      || parent_event.type() == ParserEventType::Tombstone,
                    "Invalid parent index.");

                if (parent_event.type() == ParserEventType::Start) {
                    auto& parent_start = parent_event.as_start();
                    parents.push_back(parent_start.type);
                    parent = parent_start.forward_parent;

                    // Ignore the start event once we reach it.
                    parent_event = ParserEvent::make_tombstone();
                }
            }

            for (const auto& type : reverse_view(parents))
                consumer.start_node(type);

            parents.clear();
        }

        void visit_token(ParserEvent::Token& token) { consumer.token(token); }

        void visit_error(ParserEvent::Error& error) { consumer.error(error.message); }

        void visit_finish(ParserEvent::Finish&) { consumer.finish_node(); }
    };

    EventVisitor visitor(events, consumer);
    for (auto& event : events) {
        event.visit(visitor);
    }
}

} // namespace tiro::next
