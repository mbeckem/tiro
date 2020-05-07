#ifndef TIRO_PARSER_PARSE_RESULT_HPP
#define TIRO_PARSER_PARSE_RESULT_HPP

#include "tiro/ast/ptr.hpp"
#include "tiro/compiler/fwd.hpp"

namespace tiro {

class ParseFailureTag {};

constexpr inline ParseFailureTag parse_failure;

/// Represents the result of a parse step.
/// A successful parse operation always returns a valid ast node. A failed parse operation
/// *may* still return a partial node (which should have the `error` flag set), if it was able to
/// understand some parts of the syntax.
///
/// Errors that can be handled locally are not propagated through results: If the parser
/// will recover on its own if it can do so (e.g. by seeking to a closing brace, or a semicolon).
/// Errors that cannot be handled locally are signaled by returning `false` from `parse_ok()`. In that
/// case, the caller must attempt to recover (or bubble up the error).
///
/// The following implication is always true: `parse_ok() == true -> has_node() == true`.
template<typename Node>
class [[nodiscard]] ParseResult final {
public:
    static_assert(std::is_base_of_v<Node, Node>);

    using value_type = AstPtr<Node>;

    /// Parse failure without an AST node.
    ParseResult(ParseFailureTag)
        : node_(nullptr)
        , parse_ok_(false) {}

    /// Constructs a result. If `parse_ok` is true, the node must not be null.
    template<typename OtherNode,
        std::enable_if_t<std::is_base_of_v<Node, OtherNode>>* = nullptr>
    ParseResult(AstPtr<OtherNode> && node, bool parse_ok = true)
        : node_(std::move(node))
        , parse_ok_(node_ != nullptr && parse_ok) {

        TIRO_DEBUG_ASSERT(!parse_ok_ || node_ != nullptr,
            "Node must be non-null if parsing succeeded.");
    }

    /// Converts the result from a compatible result type.
    template<typename OtherNode,
        std::enable_if_t<!std::is_same_v<OtherNode,
                             Node> && std::is_base_of_v<Node, OtherNode>>* =
            nullptr>
    ParseResult(ParseResult<OtherNode> && other)
        : node_(std::move(other.node_))
        , parse_ok_(other.parse_ok_) {
        TIRO_DEBUG_ASSERT(!parse_ok_ || node_ != nullptr,
            "Node must be non-null if parsing succeeded.");
    }

    /// True if no parse error occurred. False if the parser must synchronize.
    explicit operator bool() const { return parse_ok_; }

    /// True if no parse error occurred. False if the parser must synchronize.
    bool parse_ok() const { return parse_ok_; }

    /// Returns true if the result contains a valid node pointer. Note that the node may still
    /// have internal errors (such as invalid children or errors that the parser may have recovered from).
    ///
    /// If parse_ok() is true, has_node() is always true as well (unless the node has been moved).
    /// If parse_ok() is false, has_node() may still be true for partial results.
    bool has_node() const { return node_ != nullptr; }

    /// Extracts the node from this result.
    /// May be a completely parsed node, a partial node (with has_error() == true) or null.
    AstPtr<Node> take_node() { return std::move(node_); }

    /// Calls the function if this result holds a non-null node.
    template<typename F>
    void with_node(F && function) {
        if (node_)
            function(node_);
    }

private:
    // The result of the parse operation (or null).
    value_type node_;

    // True if parsing failed and we have to seek to a synchronizing token
    bool parse_ok_ = false;

    template<typename OtherNode>
    friend class ParseResult;
};

} // namespace tiro

#endif // TIRO_PARSER_PARSE_RESULT_HPP
