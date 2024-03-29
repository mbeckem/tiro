#ifndef TIRO_COMPILER_SYNTAX_GRAMMAR_MISC_HPP
#define TIRO_COMPILER_SYNTAX_GRAMMAR_MISC_HPP

#include "compiler/syntax/fwd.hpp"

#include <optional>

namespace tiro {

enum class FunctionKind : u8 {
    Error,
    BlockBody,     // Normal braced body, e.g. `func foo() { ... }`
    ShortExprBody, // Non block expression body, e.g. `func foo() = 3`
};

enum class VarKind : u8 {
    // For normal variable declarations
    Default,

    // Block forbidden in variable initializers (for loop header)
    NoBlock
};

extern const TokenSet VAR_FIRST;
extern const TokenSet BINDING_PATTERN_FIRST;

/// Parses a name (a single identifier is expected).
void parse_name(Parser& p, const TokenSet& recovery);

/// Parses function call arguments (concrete expressions, for function calls).
void parse_arg_list(Parser& p, const TokenSet& recovery);

/// Parses braced function parameter names (for function declarations).
void parse_param_list(Parser& p, const TokenSet& recovery);

/// Parses a function.
FunctionKind
parse_func(Parser& p, const TokenSet& recovery, std::optional<CompletedMarker> modifiers);

/// Parses a variable declaration, e.g. `var a = 1, (b, c) = foo()`.
void parse_var(
    Parser& p, VarKind kind, const TokenSet& recovery, std::optional<CompletedMarker> modifiers);

/// Parses a binding pattern, the left hand side of a variable declaration.
void parse_binding_pattern(Parser& p, const TokenSet& recovery);

/// Parses the condition expression in "while" statements and "if" expressions.
void parse_condition(Parser& p, const TokenSet& recovery);

} // namespace tiro

#endif // TIRO_COMPILER_SYNTAX_GRAMMAR_MISC_HPP
