#include "tiro/compiler/utils.hpp"

namespace tiro::compiler {

static constexpr char hex_chars[] = {'0', '1', '2', '3', '4', '5', '6', '7',
    '8', '9', 'a', 'b', 'c', 'd', 'e', 'f'};

static_assert(sizeof(hex_chars) == 16);

static void append_hex(byte c, std::string& output) {
    output += hex_chars[(c >> 4) & 0x0f];
    output += hex_chars[c & 0x0f];
}

std::string escape_string(std::string_view input) {
    std::string output;
    output.reserve(input.size());

    for (char c : input) {
        // Should keep the list of escape characters in sync with the lexer.
        switch (c) {
        case '\n':
            output += "\\n";
            continue;
        case '\r':
            output += "\\r";
            continue;
        case '\t':
            output += "\\t";
            continue;
        case '\"':
            output += "\\\"";
            continue;
        case '\'':
            output += "\\'";
            continue;
        case '\\':
            output += "\\\\";
            continue;
        case '$':
            output += "\\$";
            continue;
        }

        if (c < 32 || c >= 127) {
            output += "\\x";
            append_hex(byte(c), output);
        } else {
            output += c;
        }
    }

    return output;
}

} // namespace tiro::compiler
