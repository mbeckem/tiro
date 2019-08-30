#include "hammer/compiler/utf8.hpp"

namespace hammer {

std::tuple<code_point, const char*> decode_code_point(const char* pos, const char* end) {
    if (pos == end) {
        return std::tuple(invalid_code_point, end);
    }

    // TODO
    return std::tuple(code_point(*pos), pos + 1);
}

/* TODO unicode aware (possibly for identifiers, but not for digits etc) */

bool is_alpha(code_point c) {
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
}

bool is_digit(code_point c) {
    return c >= '0' && c <= '9';
}

bool is_identifier_begin(code_point c) {
    return is_alpha(c) || c == '_';
}

bool is_identifier_part(code_point c) {
    return is_alpha(c) || is_digit(c) || c == '_';
}

bool is_whitespace(code_point c) {
    return ::isspace((char) c); // FIXME
}

std::string code_point_to_string(code_point cp) {
    // TODO utf8
    std::string result;
    result += (char) cp;
    return result;
}

void append_code_point(std::string& buffer, code_point cp) {
    // TODO utf8
    buffer += (char) cp;
}

} // namespace hammer
