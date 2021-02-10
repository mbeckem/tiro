#ifndef TIRO_COMPILER_SYNTAX_SYNTAX_TYPE_HPP
#define TIRO_COMPILER_SYNTAX_SYNTAX_TYPE_HPP

#include "common/format.hpp"

namespace tiro::next {

enum class SyntaxType : u8 {
    /* [[[cog
        from cog import outl
        from codegen.syntax import SyntaxTypes
        for syntax_type in SyntaxTypes:
            outl(f"{syntax_type},")
        outl(f"MAX_VALUE = {SyntaxTypes[-1]},")
    ]]] */
    Literal,
    MAX_VALUE = Literal,
    // [[[end]]]
};

std::string_view to_string(SyntaxType type);

} // namespace tiro::next

TIRO_ENABLE_FREE_TO_STRING(tiro::next::SyntaxType)

#endif // TIRO_COMPILER_SYNTAX_SYNTAX_TYPE_HPP
