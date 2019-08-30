/*
 * This file describes the ast::node class hierarchy. It must be kept in sync
 * with the actual class definitions. There must be exactly one TYPE entry for every
 * node class (base or leaf type) and an additional RANGE entry for every base type.
 * 
 * The RANGE macro must list all valid child types (they must form a contiguous range).
 * 
 * It would be nice if this file could be auto-generated in the future.
 */

#ifndef HAMMER_AST_TYPE
#    define HAMMER_AST_TYPE(Type, ParentType)
#endif

#ifndef HAMMER_AST_LEAF_TYPE
#    define HAMMER_AST_LEAF_TYPE(Type, ParentType) HAMMER_AST_TYPE(Type, ParentType)
#endif

#ifndef HAMMER_AST_ABSTRACT_TYPE
#    define HAMMER_AST_ABSTRACT_TYPE(Type, ParentType) HAMMER_AST_TYPE(Type, ParentType)
#endif

#ifndef HAMMER_AST_TYPE_RANGE
#    define HAMMER_AST_TYPE_RANGE(Type, FirstChildType, LastChildType)
#endif

// clang-format off

HAMMER_AST_LEAF_TYPE(Root, Node)
HAMMER_AST_LEAF_TYPE(File, Node)

HAMMER_AST_ABSTRACT_TYPE(Decl, Node)
    HAMMER_AST_LEAF_TYPE(FuncDecl, Decl)
    HAMMER_AST_LEAF_TYPE(VarDecl, Decl)
    HAMMER_AST_LEAF_TYPE(ParamDecl, Decl)
    HAMMER_AST_LEAF_TYPE(ImportDecl, Decl)
HAMMER_AST_TYPE_RANGE(Decl, FuncDecl, ImportDecl)

HAMMER_AST_ABSTRACT_TYPE(Stmt, Node)
    HAMMER_AST_LEAF_TYPE(EmptyStmt, Stmt)
    HAMMER_AST_LEAF_TYPE(WhileStmt, Stmt)
    HAMMER_AST_LEAF_TYPE(ForStmt, Stmt)
    HAMMER_AST_LEAF_TYPE(DeclStmt, Stmt)
    HAMMER_AST_LEAF_TYPE(ExprStmt, Stmt)
HAMMER_AST_TYPE_RANGE(Stmt, EmptyStmt, ExprStmt)

HAMMER_AST_ABSTRACT_TYPE(Expr, Node)
    HAMMER_AST_LEAF_TYPE(UnaryExpr, Expr)
    HAMMER_AST_LEAF_TYPE(BinaryExpr, Expr)
    HAMMER_AST_LEAF_TYPE(VarExpr, Expr)
    HAMMER_AST_LEAF_TYPE(DotExpr, Expr)
    HAMMER_AST_LEAF_TYPE(CallExpr, Expr)
    HAMMER_AST_LEAF_TYPE(IndexExpr, Expr)
    HAMMER_AST_LEAF_TYPE(IfExpr, Expr)
    HAMMER_AST_LEAF_TYPE(ReturnExpr, Expr)
    HAMMER_AST_LEAF_TYPE(ContinueExpr, Expr)
    HAMMER_AST_LEAF_TYPE(BreakExpr, Expr)
    HAMMER_AST_LEAF_TYPE(BlockExpr, Expr)

    HAMMER_AST_ABSTRACT_TYPE(Literal, Expr)
        HAMMER_AST_LEAF_TYPE(NullLiteral, Literal)
        HAMMER_AST_LEAF_TYPE(BooleanLiteral, Literal)
        HAMMER_AST_LEAF_TYPE(IntegerLiteral, Literal)
        HAMMER_AST_LEAF_TYPE(FloatLiteral, Literal)
        HAMMER_AST_LEAF_TYPE(StringLiteral, Literal)
        HAMMER_AST_LEAF_TYPE(ArrayLiteral, Literal)
        HAMMER_AST_LEAF_TYPE(TupleLiteral, Literal)
        HAMMER_AST_LEAF_TYPE(MapLiteral, Literal)
        HAMMER_AST_LEAF_TYPE(SetLiteral, Literal)
        HAMMER_AST_LEAF_TYPE(FuncLiteral, Literal)
    HAMMER_AST_TYPE_RANGE(Literal, NullLiteral, FuncLiteral)

HAMMER_AST_TYPE_RANGE(Expr, UnaryExpr, FuncLiteral)

HAMMER_AST_TYPE_RANGE(Node, Root, FuncLiteral)

// clang-format on

#undef HAMMER_AST_TYPE
#undef HAMMER_AST_LEAF_TYPE
#undef HAMMER_AST_ABSTRACT_TYPE
#undef HAMMER_AST_TYPE_RANGE
