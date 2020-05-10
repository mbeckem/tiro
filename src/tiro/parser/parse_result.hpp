#ifndef TIRO_PARSER_PARSE_RESULT_HPP
#define TIRO_PARSER_PARSE_RESULT_HPP

#include "tiro/ast/ptr.hpp"
#include "tiro/compiler/fwd.hpp"

#include <optional>
#include <type_traits>

namespace tiro {

template<typename Node>
struct ParsePartial {
    Node node;
};

struct ParseFailure {};

template<typename Node>
ParseResult<std::decay_t<Node>> parse_success(Node&& node) {
    return {std::forward<Node>(node)};
}

template<typename Node>
ParsePartial<std::decay_t<Node>> parse_partial(Node&& node) {
    return {std::forward<Node>(node)};
}

inline ParseFailure parse_failure() {
    return {};
}

/// Represents the result of a parse step.
/// A successful parse operation always returns a valid ast node. A failed parse operation
/// *may* still return a partial node (which should have the `error` flag set), if it was able to
/// understand some parts of the syntax.
///
/// Errors that can be handled locally are not propagated through results: the parser
/// will recover on its own if it can do so (e.g. by seeking to a closing brace, or a semicolon).
/// Errors that cannot be handled locally are signaled by returning `false` from `parser_ok()`. In that
/// case, the caller must attempt to recover (or bubble up the error).
///
/// The following implication is always true: `parser_ok() == true -> has_node() == true`.
template<typename Node>
class [[nodiscard]] ParseResult final {
public:
    enum Type { Success, Partial, Failure };

    ParseResult()
        : ParseResult(parse_failure()) {}

    /// Represents successful completion of a parsing operation.
    ParseResult(Node node)
        : type_(Success)
        , node_(std::move(node)) {}

    /// Represents partial failure (parser was able to produce an at least partially
    /// correct result but recovery by the caller is needed).
    ParseResult(ParsePartial<Node> partial)
        : type_(Partial)
        , node_(std::move(partial.node)) {}

    /// Parse failure without an AST node. Recovery by the caller is needed.
    ParseResult(ParseFailure)
        : type_(Partial)
        , node_() {}

    /// True if no parse error occurred. False if the parser must recover.
    explicit operator bool() const { return parser_ok(); }

    /// True if no parse error occurred. False if the parser must recover.
    bool parser_ok() const { return type_ == Success; }

    Type type() const { return type_; }

    /// Returns true if the result contains a valid node pointer. Note that the node may still
    /// have internal errors (such as invalid children or errors that the parser may have recovered from).
    ///
    /// If parser_ok() is true, has_node() is always true as well (unless the node has been moved).
    /// If parser_ok() is false, has_node() may still be true for partial results.
    bool has_node() const { return node_.has_value(); }

    /// Extracts the node from this result.
    std::optional<Node> take_node() { return std::move(node_); }

private:
    Type type_;
    std::optional<Node> node_;

    template<typename OtherNode>
    friend class ParseResult;
};

} // namespace tiro

#endif // TIRO_PARSER_PARSE_RESULT_HPP
