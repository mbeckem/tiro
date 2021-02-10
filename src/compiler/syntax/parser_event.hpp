#ifndef TIRO_COMPILER_SYNTAX_PARSER_EVENT_HPP
#define TIRO_COMPILER_SYNTAX_PARSER_EVENT_HPP

#include "common/assert.hpp"
#include "common/defs.hpp"
#include "common/format.hpp"
#include "compiler/syntax/syntax_type.hpp"
#include "compiler/syntax/token.hpp"

namespace tiro::next {

/* [[[cog
    from cog import outl
    from codegen.unions import define, implement_inlines
    from codegen.syntax import ParserEvent
    define(ParserEvent.tag)
    outl()
    define(ParserEvent)
    outl()
    implement_inlines(ParserEvent)
]]] */
/// Represents the type of a ParserEvent.
enum class ParserEventType : u8 {
    Tombstone,
    Start,
    Finish,
    Token,
    Error,
};

std::string_view to_string(ParserEventType type);

/// ParserEvents are emitted by the parser in order to start and finish nodes
/// or to add tokens to the current node.
///
/// Events are emitted as a simple stream of values that form an implicit tree structure.
/// This design is inspired by the Kotlin compiler and the rust-analyzer project.
class ParserEvent final {
public:
    /// This event does nothing. The following events are added to the current node instead.
    /// Tombstones are used before the type of a node is known of when syntax nodes become abandoned.
    struct Tombstone final {};

    /// Marks the start of a syntax node. Every start event is followed by a matching finish event.
    ///
    /// Some syntax rules will emit a parent node *after* the child has been observed.
    /// This is the case, for example, in function call expressions like `EXPR(ARGS...)` where
    /// a new function call node becomes the parent of the fully parsed EXPR node.
    ///
    /// To enable this pattern, every start event may have a `forward_parent` pointing to a later parent node's
    /// start event using its index.
    /// Nodes that do not need a forward parent leave its value at `0`, which is never a valid index for a forward parent.
    struct Start final {
        /// The node's syntax type.
        SyntaxType type;

        /// The forward parent node's index, or 0 if there is none.
        size_t forward_parent;

        Start(const SyntaxType& type_, const size_t& forward_parent_)
            : type(type_)
            , forward_parent(forward_parent_) {}
    };

    /// The finish event ends the current node.
    struct Finish final {};

    /// Tokens emitted between the start and finish events of a node belong to that node.
    using Token = tiro::next::Token;

    /// Represents an error encountered while parsing the current node
    struct Error final {
        /// The error message. TODO: message class / message ids.
        std::string message;

        explicit Error(std::string message_)
            : message(std::move(message_)) {}
    };

    static ParserEvent make_tombstone();
    static ParserEvent make_start(const SyntaxType& type, const size_t& forward_parent);
    static ParserEvent make_finish();
    static ParserEvent make_token(const Token& token);
    static ParserEvent make_error(std::string message);

    ParserEvent(Tombstone tombstone);
    ParserEvent(Start start);
    ParserEvent(Finish finish);
    ParserEvent(Token token);
    ParserEvent(Error error);

    ~ParserEvent();

    ParserEvent(ParserEvent&& other) noexcept;
    ParserEvent& operator=(ParserEvent&& other) noexcept;

    ParserEventType type() const noexcept { return type_; }

    void format(FormatStream& stream) const;

    const Tombstone& as_tombstone() const;
    Tombstone& as_tombstone();

    const Start& as_start() const;
    Start& as_start();

    const Finish& as_finish() const;
    Finish& as_finish();

    const Token& as_token() const;
    Token& as_token();

    const Error& as_error() const;
    Error& as_error();

    template<typename Visitor, typename... Args>
    TIRO_FORCE_INLINE decltype(auto) visit(Visitor&& vis, Args&&... args) {
        return visit_impl(*this, std::forward<Visitor>(vis), std::forward<Args>(args)...);
    }

    template<typename Visitor, typename... Args>
    TIRO_FORCE_INLINE decltype(auto) visit(Visitor&& vis, Args&&... args) const {
        return visit_impl(*this, std::forward<Visitor>(vis), std::forward<Args>(args)...);
    }

private:
    void _destroy_value() noexcept;
    void _move_construct_value(ParserEvent& other) noexcept;
    void _move_assign_value(ParserEvent& other) noexcept;

    template<typename Self, typename Visitor, typename... Args>
    static TIRO_FORCE_INLINE decltype(auto) visit_impl(Self&& self, Visitor&& vis, Args&&... args);

private:
    ParserEventType type_;
    union {
        Tombstone tombstone_;
        Start start_;
        Finish finish_;
        Token token_;
        Error error_;
    };
};

template<typename Self, typename Visitor, typename... Args>
decltype(auto) ParserEvent::visit_impl(Self&& self, Visitor&& vis, Args&&... args) {
    switch (self.type()) {
    case ParserEventType::Tombstone:
        return vis.visit_tombstone(self.tombstone_, std::forward<Args>(args)...);
    case ParserEventType::Start:
        return vis.visit_start(self.start_, std::forward<Args>(args)...);
    case ParserEventType::Finish:
        return vis.visit_finish(self.finish_, std::forward<Args>(args)...);
    case ParserEventType::Token:
        return vis.visit_token(self.token_, std::forward<Args>(args)...);
    case ParserEventType::Error:
        return vis.visit_error(self.error_, std::forward<Args>(args)...);
    }
    TIRO_UNREACHABLE("Invalid ParserEvent type.");
}
// [[[end]]]

}; // namespace tiro::next

TIRO_ENABLE_FREE_TO_STRING(tiro::next::ParserEventType)
TIRO_ENABLE_MEMBER_FORMAT(tiro::next::ParserEvent)

#endif // TIRO_COMPILER_SYNTAX_PARSER_EVENT_HPP
