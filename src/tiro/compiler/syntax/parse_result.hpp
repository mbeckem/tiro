#ifndef TIRO_COMPILER_SYNTAX_PARSE_RESULT_HPP
#define TIRO_COMPILER_SYNTAX_PARSE_RESULT_HPP

#include "tiro/compiler/fwd.hpp"

namespace tiro::compiler {

class ParseFailureTag {};

constexpr inline ParseFailureTag parse_failure;

// The only logical implication in this class is
// parse_ok() == true -> has_node() == true.
template<typename Node>
class [[nodiscard]] ParseResult final {
public:
    static_assert(std::is_base_of_v<Node, Node>);

    using value_type = NodePtr<Node>;

    // Failure and no node value at all.
    ParseResult(ParseFailureTag)
        : node_(nullptr)
        , parse_ok_(false) {}

    // Constructs a result. If `parse_ok` is true, the node must not be null.
    template<typename OtherNode,
        std::enable_if_t<std::is_base_of_v<Node, OtherNode>>* = nullptr>
    ParseResult(NodePtr<OtherNode> && node, bool parse_ok = true)
        : node_(std::move(node))
        , parse_ok_(node_ != nullptr && parse_ok) {

        TIRO_ASSERT(!parse_ok_ || node_ != nullptr,
            "Node must be non-null if parsing succeeded.");
    }

    // Converts the result from a compatible result type.
    template<typename OtherNode,
        std::enable_if_t<!std::is_same_v<OtherNode,
                             Node> && std::is_base_of_v<Node, OtherNode>>* =
            nullptr>
    ParseResult(ParseResult<OtherNode> && other)
        : node_(std::move(other.node_))
        , parse_ok_(other.parse_ok_) {
        TIRO_ASSERT(!parse_ok_ || node_ != nullptr,
            "Node must be non-null if parsing succeeded.");
    }

    // True if no parse error occurred. False if the parser must synchronize.
    explicit operator bool() const { return parse_ok_; }

    // True if no parse error occurred. False if the parser must synchronize.
    bool parse_ok() const { return parse_ok_; }

    // If parse_ok() is true, has_node() is always true as well (unless the node has been moved).
    // If parse_ok() is false, has_node() may still be true for partial results.
    bool has_node() const { return node_ != nullptr; }

    // May be completely parsed node, a partial node (with has_error() == true) or null.
    NodePtr<Node> take_node() { return std::move(node_); }

    // Calls the function if this result holds a non-null node.
    template<typename F>
    void with_node(F && function) {
        if (node_)
            function(std::move(node_));
    }

private:
    // The result of the parse operation (or null).
    value_type node_;

    // True if parsing failed and we have to seek to a synchronizing token
    bool parse_ok_ = false;

    template<typename OtherNode>
    friend class ParseResult;
};

} // namespace tiro::compiler

#endif // TIRO_COMPILER_SYNTAX_PARSE_RESULT_HPP
