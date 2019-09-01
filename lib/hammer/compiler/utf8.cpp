#include "hammer/compiler/utf8.hpp"

namespace hammer {

std::tuple<CodePoint, const char*> decode_code_point(
    const char* pos, const char* end) {
    if (pos == end) {
        return std::tuple(invalid_code_point, end);
    }

    // TODO
    return std::tuple(CodePoint(*pos), pos + 1);
}

/* TODO unicode aware (possibly for identifiers, but not for digits etc) */

bool is_alpha(CodePoint c) {
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
}

bool is_digit(CodePoint c) {
    return c >= '0' && c <= '9';
}

bool is_identifier_begin(CodePoint c) {
    return is_alpha(c) || c == '_';
}

bool is_identifier_part(CodePoint c) {
    return is_alpha(c) || is_digit(c) || c == '_';
}

bool is_whitespace(CodePoint c) {
    return ::isspace((char) c); // FIXME
}

std::string code_point_to_string(CodePoint cp) {
    // TODO utf8
    std::string result;
    result += (char) cp;
    return result;
}

void append_code_point(std::string& buffer, CodePoint cp) {
    // TODO utf8
    buffer += (char) cp;
}

} // namespace hammer
