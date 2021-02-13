#include "compiler/syntax/syntax_type.hpp"

#include "common/assert.hpp"

namespace tiro::next {

std::string_view to_string(SyntaxType type) {
    switch (type) {
#define TIRO_CASE(type)    \
    case SyntaxType::type: \
        return #type;

        TIRO_CASE(Error)
        TIRO_CASE(Literal)
        TIRO_CASE(ReturnExpr)
        TIRO_CASE(ContinueExpr)
        TIRO_CASE(BreakExpr)
        TIRO_CASE(VarExpr)
        TIRO_CASE(UnaryExpr)
        TIRO_CASE(BinaryExpr)

#undef TIRO_CASE
    }

    TIRO_UNREACHABLE("Invalid syntax type.");
}

} // namespace tiro::next
