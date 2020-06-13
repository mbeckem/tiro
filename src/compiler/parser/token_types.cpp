#include "compiler/parser/token_types.hpp"

namespace tiro {

std::string TokenTypes::to_string() const {
    fmt::memory_buffer buf;

    fmt::format_to(buf, "TokenTypes{{");
    {
        bool first = true;
        for (TokenType type : *this) {
            if (!first)
                fmt::format_to(buf, ", ");

            fmt::format_to(buf, "{}", to_token_name(type));
            first = false;
        }
    }
    fmt::format_to(buf, "}}");

    return fmt::to_string(buf);
}

} // namespace tiro
