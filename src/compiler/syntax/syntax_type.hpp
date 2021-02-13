#ifndef TIRO_COMPILER_SYNTAX_SYNTAX_TYPE_HPP
#define TIRO_COMPILER_SYNTAX_SYNTAX_TYPE_HPP

#include "common/format.hpp"

namespace tiro::next {

enum class SyntaxType : u8 {
    /// Returned when no actual node type could be recognized.
    Error,

    Literal,
    ReturnExpr,
    ContinueExpr,
    BreakExpr,
    VarExpr,
    UnaryExpr,
    BinaryExpr,
    TupleExpr,
    RecordExpr,
    GroupedExpr, // ( expr )
    ArrayExpr,   // [a, b]
    MemberExpr,  // a.b
    IndexExpr,   // a[b]

    MAX_VALUE = IndexExpr,
};

std::string_view to_string(SyntaxType type);

} // namespace tiro::next

TIRO_ENABLE_FREE_TO_STRING(tiro::next::SyntaxType)

#endif // TIRO_COMPILER_SYNTAX_SYNTAX_TYPE_HPP
