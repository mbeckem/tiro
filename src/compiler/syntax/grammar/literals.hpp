#ifndef TIRO_COMPILER_SYNTAX_GRAMMAR_LITERALS_HPP
#define TIRO_COMPILER_SYNTAX_GRAMMAR_LITERALS_HPP

#include "common/adt/function_ref.hpp"
#include "common/defs.hpp"
#include "common/text/unicode.hpp"

#include <optional>
#include <string_view>

namespace tiro::next {

/// Attempts to parse the given code point as a digit with the given base.
/// Base must be 2, 8, 10 or 16.
std::optional<int> to_digit(CodePoint c, int base);

/// Attempts to parse the given symbol's source code (e.g. '#foo') into a symbol name ('foo').
std::optional<std::string_view>
parse_symbol_name(std::string_view symbol_source, FunctionRef<void(std::string_view)> error_sink);

/// Attempts to parse the given integer's source code (e.g. '123' or '0xff') into an integer value.
std::optional<i64> parse_integer_value(
    std::string_view integer_source, FunctionRef<void(std::string_view)> error_sink);

/// Attempts to parse the given float's source code (e.g. '123.4' or '0xff.a') into a floating point value.
std::optional<f64>
parse_float_value(std::string_view float_source, FunctionRef<void(std::string_view)> error_sink);

} // namespace tiro::next

#endif // TIRO_COMPILER_SYNTAX_GRAMMAR_LITERALS_HPP
