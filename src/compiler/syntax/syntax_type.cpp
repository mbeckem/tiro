#include "compiler/syntax/syntax_type.hpp"

#include "common/assert.hpp"

namespace tiro::next {

std::string_view to_string(SyntaxType type) {
    switch (type) {
    /* [[[cog
            from cog import outl
            from codegen.syntax import SyntaxTypes
            for syntax_type in SyntaxTypes:
                outl(f"case SyntaxType::{syntax_type}: return \"{syntax_type}\";")
        ]]] */
    case SyntaxType::Literal:
        return "Literal";
        // [[[end]]]
    }

    TIRO_UNREACHABLE("Invalid syntax type.");
}

} // namespace tiro::next
