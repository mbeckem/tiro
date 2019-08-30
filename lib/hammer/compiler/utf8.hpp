#ifndef HAMMER_COMPILER_UTF8_HPP
#define HAMMER_COMPILER_UTF8_HPP

#include "hammer/core/defs.hpp"

#include <tuple>

namespace hammer {

using code_point = u32;

inline constexpr code_point invalid_code_point = 0; // FIXME

/**
 * Returns the next code point (at "pos") and the position just after that code point
 * to continue with the iteration. An invalid code point together with "end" will be returned
 * on error.
 *
 * TODO: decode utf8, this is just ascii for now.
 */
std::tuple<code_point, const char*> decode_code_point(const char* pos, const char* end);

bool is_alpha(code_point c);
bool is_digit(code_point c);

/// Returns true if the code point is a valid start for an identifier.
bool is_identifier_begin(code_point c);

/// Returns true if the code point can be part of an identifier.
bool is_identifier_part(code_point c);

bool is_whitespace(code_point c);

std::string code_point_to_string(code_point cp);

void append_code_point(std::string& buffer, code_point cp);

} // namespace hammer

#endif // HAMMER_COMPILER_UTF8_HPP
