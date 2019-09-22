#ifndef HAMMER_COMPILER_UTF8_HPP
#define HAMMER_COMPILER_UTF8_HPP

#include "hammer/core/defs.hpp"

#include <tuple>

namespace hammer {

using CodePoint = u32;

inline constexpr CodePoint invalid_code_point = 0; // FIXME

/**
 * Returns the next code point (at "pos") and the position just after that code point
 * to continue with the iteration. An invalid code point together with "end" will be returned
 * on error.
 *
 * TODO: decode utf8, this is just ascii for now.
 */
std::tuple<CodePoint, const char*>
decode_code_point(const char* pos, const char* end);

bool is_alpha(CodePoint c);
bool is_digit(CodePoint c);

/// Returns true if the code point is a valid start for an identifier.
bool is_identifier_begin(CodePoint c);

/// Returns true if the code point can be part of an identifier.
bool is_identifier_part(CodePoint c);

bool is_whitespace(CodePoint c);

std::string code_point_to_string(CodePoint cp);

void append_code_point(std::string& buffer, CodePoint cp);

} // namespace hammer

#endif // HAMMER_COMPILER_UTF8_HPP
