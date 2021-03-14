#include "compiler/syntax/token_set.hpp"

namespace tiro {

void TokenSet::format(FormatStream& stream) const {
    stream.format("TokenSet{{");
    {
        bool first = true;
        for (TokenType type : *this) {
            if (!first)
                stream.format(", ");

            stream.format("{}", type);
            first = false;
        }
    }
    stream.format("}}");
}

} // namespace tiro
