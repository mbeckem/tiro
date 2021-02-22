#ifndef TIRO_COMPILER_SYNTAX_SYNTAX_TYPE_HPP
#define TIRO_COMPILER_SYNTAX_SYNTAX_TYPE_HPP

#include "common/format.hpp"

namespace tiro::next {

enum class SyntaxType : u8 {
    /// Returned when no actual node type could be recognized.
    Error,

    Name,      // Name node for functions and types
    Member,    // Member in MemberExpr
    Literal,   // Literal values, e.g. integer token inside
    Condition, // Condition in if/for/while nodes
    Modifiers, // List of modifiers before an item, e.g. "export"

    ReturnExpr,        // return [expr]
    ContinueExpr,      // literal continue
    BreakExpr,         // literal break
    VarExpr,           // identifier
    UnaryExpr,         // OP expr
    BinaryExpr,        // expr OP expr
    MemberExpr,        // a.b
    IndexExpr,         // a[b]
    CallExpr,          // expr(expr,...)
    ConstructExpr,     // ident { ... }    -- currently used for maps and sets
    GroupedExpr,       // "(" expr ")"
    TupleExpr,         // "(" expr,... ")"
    RecordExpr,        // "(" name: expr,... ")" TODO: Probably needs more structure
    ArrayExpr,         // [a, b]
    IfExpr,            // if expr block [else block | if-expr  ]
    BlockExpr,         // "{" stmt;... "}"
    FuncExpr,          // func
    StringExpr,        // "abc $var ${expr}"
    StringFormatItem,  // $var
    StringFormatBlock, // ${expr}

    DeferStmt,     // defer expr;
    AssertStmt,    // assert(expr[, message])
    ExprStmt,      // expr;
    VarStmt,       // var-decl;
    WhileStmt,     // while expr { ... }
    ForStmt,       // for (var i = 0; i < 10; i += 1) { ... }
    ForStmtHeader, // [var decl]; [expr]; [expr]
    ForEachStmt,   // for (var foo in bar) { ... }

    Var,          // var | const bindings...
    Binding,      // (BindingName | BindingTuple) [ "=" expr ]
    BindingName,  // Single identifier to bind to
    BindingTuple, // (a, b, ...) to bind to

    Func,      // func [ident] ArgList [ "=" ] { ... }
    ArgList,   // Argument list for function calls and assert statements
    ParamList, // List of named parameters in a function declaration

    ImportItem, // import a.b.c;
    VarItem,    // like var stmt, but with modifiers
    FuncItem,   // func at top level, with modifiers

    MAX_VALUE = FuncItem,
};

std::string_view to_string(SyntaxType type);

} // namespace tiro::next

TIRO_ENABLE_FREE_TO_STRING(tiro::next::SyntaxType)

#endif // TIRO_COMPILER_SYNTAX_SYNTAX_TYPE_HPP
