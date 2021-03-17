#ifndef TIRO_COMPILER_SYNTAX_SYNTAX_TYPE_HPP
#define TIRO_COMPILER_SYNTAX_SYNTAX_TYPE_HPP

#include "common/format.hpp"

namespace tiro {

enum class SyntaxType : u8 {
    /// Virtual root node. Never emitted by the parser, but used in the syntax tree.
    Root,

    /// Returned when no actual node type could be recognized.
    Error,

    File,       // Contains a series of items
    Name,       // Name node for functions and types
    Literal,    // Literal values, e.g. integer token inside
    Condition,  // Condition in if/while nodes
    Modifiers,  // List of modifiers before an item, e.g. "export"
    RecordItem, // Name : Expr
    MapItem,    // Expr : Expr

    Var,          // var | const bindings...
    Binding,      // (BindingName | BindingTuple) [ "=" expr ]
    BindingName,  // Single identifier to bind to
    BindingTuple, // (a, b, ...) to bind to

    Func,      // [Modifiers] func [Name] ParamList [ "=" ] { ... }
    ArgList,   // Argument list for function calls and assert statements
    ParamList, // List of named parameters in a function declaration

    // Expressions
    ReturnExpr,        // return [expr]
    ContinueExpr,      // literal continue
    BreakExpr,         // literal break
    VarExpr,           // identifier
    UnaryExpr,         // OP expr
    BinaryExpr,        // expr OP expr
    FieldExpr,         // a.b
    TupleFieldExpr,    // a.0
    IndexExpr,         // a[b]
    CallExpr,          // expr arglist
    GroupedExpr,       // "(" expr ")"
    TupleExpr,         // "(" expr,... ")"
    RecordExpr,        // "(" name: expr,... ")" TODO: Probably needs more structure
    ArrayExpr,         // [a, b]
    SetExpr,           // "set" { expr... }
    MapExpr,           // "map" { mapitem... }
    IfExpr,            // if Condition block [else block | if-expr  ]
    BlockExpr,         // "{" stmt;... "}"
    FuncExpr,          // func
    StringExpr,        // "abc $var ${expr}"
    StringFormatItem,  // $var
    StringFormatBlock, // ${expr}
    StringGroup,       // StringExpr+

    // Statements
    DeferStmt,     // defer expr;
    AssertStmt,    // assert(expr[, message])
    ExprStmt,      // expr[;]
    VarStmt,       // var-decl;
    WhileStmt,     // while Condition { ... }
    ForStmt,       // for ForStmtHeader { ... }
    ForStmtHeader, // [var decl]; [expr]; [expr]
    ForEachStmt,   // for (BindingName | BindingTuple) in Expr { ... }

    // Items (at top level)
    ImportItem, // import a.b.c;
    VarItem,    // like var stmt, but with modifiers
    FuncItem,   // func at top level, with modifiers

    MAX_VALUE = FuncItem,
};

std::string_view to_string(SyntaxType type);

} // namespace tiro

TIRO_ENABLE_FREE_TO_STRING(tiro::SyntaxType)

#endif // TIRO_COMPILER_SYNTAX_SYNTAX_TYPE_HPP
