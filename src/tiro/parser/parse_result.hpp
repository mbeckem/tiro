#ifndef TIRO_PARSER_PARSE_RESULT_HPP
#define TIRO_PARSER_PARSE_RESULT_HPP

#include "tiro/ast/ptr.hpp"
#include "tiro/compiler/fwd.hpp"

#include <optional>
#include <type_traits>

namespace tiro {

/// Represents a syntax error with a partial result.
/// The parser must recover from the syntax error but can use
/// the partial data.
template<typename Node>
struct PartialSyntaxError {
    std::optional<Node> partial;
};

/// Represents a syntax error without any data. The parser must
/// recover from the error.
struct EmptySyntaxError {};

/// Represents the result of a parse step.
///
/// A successful parse operation always returns a valid ast node. A failed parse operation
/// *may* still return a partial node (which may be an error node or contain error nodes as children).
///
/// Errors that can be handled locally are not propagated through results: the parser
/// will recover on its own if it can do so (e.g. by seeking to a closing brace, or a semicolon).
/// Errors that cannot be handled locally are signaled by returning a parse failure. The caller must
/// attempt to recover from the syntax error or forward the error to its caller.
template<typename Node>
class [[nodiscard]] ParseResult final {
public:
    enum class Type {
        Success,
        SyntaxError,
    };

    ParseResult()
        : type_(Type::SyntaxError) {}

    /// Represents successful completion of a parsing operation.
    ParseResult(Node node)
        : type_(Type::Success)
        , node_(std::move(node)) {}

    ParseResult(PartialSyntaxError<Node> error)
        : type_(Type::SyntaxError)
        , node_(std::move(error.partial)) {}

    /// Parse failure without an AST node. Recovery by the caller is needed.
    ParseResult(EmptySyntaxError)
        : type_(Type::SyntaxError)
        , node_() {}

    /// True if no syntax error occurred. False if the parser must recover.
    explicit operator bool() const { return type_ == Type::Success; }

    /// True if a syntax error occurred, i.e. if recovery is necessary.
    bool is_error() const { return type_ == Type::SyntaxError; }

    /// Returns true if the result contains a valid node pointer. Note that the node may still
    /// have internal errors (such as invalid children or errors that the parser may have recovered from).
    bool has_node() const { return node_.has_value(); }

    /// Extracts the node from this result.
    std::optional<Node> take_node() { return std::move(node_); }

private:
    Type type_;
    std::optional<Node> node_;

    template<typename OtherNode>
    friend class ParseResult;
};

template<typename Node>
ParseResult<Node> parse_success(Node node) {
    return {std::move(node)};
}

template<typename Node>
PartialSyntaxError<Node> syntax_error(Node partial) {
    return {std::move(partial)};
}

template<typename Node>
PartialSyntaxError<Node> syntax_error(std::optional<Node> partial) {
    return {std::move(partial)};
}

inline EmptySyntaxError syntax_error() {
    return {};
}

} // namespace tiro

#endif // TIRO_PARSER_PARSE_RESULT_HPP
