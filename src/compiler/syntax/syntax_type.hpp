#ifndef TIRO_COMPILER_SYNTAX_SYNTAX_TYPE_HPP
#define TIRO_COMPILER_SYNTAX_SYNTAX_TYPE_HPP

#include "common/format.hpp"

namespace tiro::next {

enum class SyntaxType : u8 {
    /// Returned when no actual node type could be recognized.
    Error,

    Name,      // Variable name, e.g. in expression context
    Member,    // Member in MemberExpr
    Literal,   // Literal values, e.g. integer token inside
    Condition, // Condition in if/for/while nodes
    ArgList,   // Argument list for function calls and asset statements

    ReturnExpr,        // return [expr]
    ContinueExpr,      // literal continue
    BreakExpr,         // literal break
    UnaryExpr,         // OP expr
    BinaryExpr,        // expr OP expr
    MemberExpr,        // a.b
    IndexExpr,         // a[b]
    CallExpr,          // expr(expr,...)
    GroupedExpr,       // "(" expr ")"
    TupleExpr,         // "(" expr,... ")"
    RecordExpr,        // "(" name: expr,... ")" TODO: Probably needs more structure
    ArrayExpr,         // [a, b]
    IfExpr,            // if expr block [else block | if-expr  ]
    BlockExpr,         // "{" stmt;... "}"
    StringExpr,        // "abc $var ${expr}"
    StringFormatItem,  // $var
    StringFormatBlock, // ${expr}

    DeferStmt,
    AssertStmt,
    ExprStmt,
    VarDeclStmt,

    MAX_VALUE = VarDeclStmt,
};

std::string_view to_string(SyntaxType type);

} // namespace tiro::next

TIRO_ENABLE_FREE_TO_STRING(tiro::next::SyntaxType)

#endif // TIRO_COMPILER_SYNTAX_SYNTAX_TYPE_HPP
