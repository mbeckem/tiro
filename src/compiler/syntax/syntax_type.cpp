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

#undef TIRO_CASE
    }

    TIRO_UNREACHABLE("Invalid syntax type.");
}

} // namespace tiro::next
