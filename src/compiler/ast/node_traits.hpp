#ifndef TIRO_COMPILER_AST_NODE_TRAITS_HPP
#define TIRO_COMPILER_AST_NODE_TRAITS_HPP

#include "compiler/ast/node.hpp"

namespace tiro {

template<typename NodeType>
struct AstNodeTraits;

#define TIRO_DEFINE_AST_BASE(Name, FirstChildId, LastChildId)  \
    template<>                                                 \
    struct AstNodeTraits<Name> {                               \
        static constexpr bool is_base = true;                  \
        static constexpr auto first_child_id = (FirstChildId); \
        static constexpr auto last_child_id = (LastChildId);   \
    };

#define TIRO_DEFINE_AST_LEAF(Name, TypeId)        \
    template<>                                    \
    struct AstNodeTraits<Name> {                  \
        static constexpr bool is_base = false;    \
        static constexpr auto type_id = (TypeId); \
    };

/* [[[cog
    import cog
    from codegen.ast import walk_types
    for node_type in walk_types():
        cpp_name = node_type.cpp_name
        if node_type.final:
            type_id = f"AstNodeType::{node_type.name}"
            trait = f"TIRO_DEFINE_AST_LEAF({cpp_name}, {type_id});"
        else:
            first_child_id = f"AstNodeType::First{node_type.name}"
            last_child_id = f"AstNodeType::Last{node_type.name}"
            trait = f"TIRO_DEFINE_AST_BASE({cpp_name}, {first_child_id}, {last_child_id})"
        cog.outl(trait);
]]] */
TIRO_DEFINE_AST_BASE(AstNode, AstNodeType::FirstNode, AstNodeType::LastNode)
TIRO_DEFINE_AST_LEAF(AstBinding, AstNodeType::Binding);
TIRO_DEFINE_AST_BASE(AstBindingSpec, AstNodeType::FirstBindingSpec, AstNodeType::LastBindingSpec)
TIRO_DEFINE_AST_LEAF(AstTupleBindingSpec, AstNodeType::TupleBindingSpec);
TIRO_DEFINE_AST_LEAF(AstVarBindingSpec, AstNodeType::VarBindingSpec);
TIRO_DEFINE_AST_BASE(AstDecl, AstNodeType::FirstDecl, AstNodeType::LastDecl)
TIRO_DEFINE_AST_LEAF(AstFuncDecl, AstNodeType::FuncDecl);
TIRO_DEFINE_AST_LEAF(AstImportDecl, AstNodeType::ImportDecl);
TIRO_DEFINE_AST_LEAF(AstParamDecl, AstNodeType::ParamDecl);
TIRO_DEFINE_AST_LEAF(AstVarDecl, AstNodeType::VarDecl);
TIRO_DEFINE_AST_BASE(AstExpr, AstNodeType::FirstExpr, AstNodeType::LastExpr)
TIRO_DEFINE_AST_LEAF(AstBinaryExpr, AstNodeType::BinaryExpr);
TIRO_DEFINE_AST_LEAF(AstBlockExpr, AstNodeType::BlockExpr);
TIRO_DEFINE_AST_LEAF(AstBreakExpr, AstNodeType::BreakExpr);
TIRO_DEFINE_AST_LEAF(AstCallExpr, AstNodeType::CallExpr);
TIRO_DEFINE_AST_LEAF(AstContinueExpr, AstNodeType::ContinueExpr);
TIRO_DEFINE_AST_LEAF(AstElementExpr, AstNodeType::ElementExpr);
TIRO_DEFINE_AST_LEAF(AstFuncExpr, AstNodeType::FuncExpr);
TIRO_DEFINE_AST_LEAF(AstIfExpr, AstNodeType::IfExpr);
TIRO_DEFINE_AST_BASE(AstLiteral, AstNodeType::FirstLiteral, AstNodeType::LastLiteral)
TIRO_DEFINE_AST_LEAF(AstArrayLiteral, AstNodeType::ArrayLiteral);
TIRO_DEFINE_AST_LEAF(AstBooleanLiteral, AstNodeType::BooleanLiteral);
TIRO_DEFINE_AST_LEAF(AstFloatLiteral, AstNodeType::FloatLiteral);
TIRO_DEFINE_AST_LEAF(AstIntegerLiteral, AstNodeType::IntegerLiteral);
TIRO_DEFINE_AST_LEAF(AstMapLiteral, AstNodeType::MapLiteral);
TIRO_DEFINE_AST_LEAF(AstNullLiteral, AstNodeType::NullLiteral);
TIRO_DEFINE_AST_LEAF(AstRecordLiteral, AstNodeType::RecordLiteral);
TIRO_DEFINE_AST_LEAF(AstSetLiteral, AstNodeType::SetLiteral);
TIRO_DEFINE_AST_LEAF(AstStringLiteral, AstNodeType::StringLiteral);
TIRO_DEFINE_AST_LEAF(AstSymbolLiteral, AstNodeType::SymbolLiteral);
TIRO_DEFINE_AST_LEAF(AstTupleLiteral, AstNodeType::TupleLiteral);
TIRO_DEFINE_AST_LEAF(AstPropertyExpr, AstNodeType::PropertyExpr);
TIRO_DEFINE_AST_LEAF(AstReturnExpr, AstNodeType::ReturnExpr);
TIRO_DEFINE_AST_LEAF(AstStringExpr, AstNodeType::StringExpr);
TIRO_DEFINE_AST_LEAF(AstStringGroupExpr, AstNodeType::StringGroupExpr);
TIRO_DEFINE_AST_LEAF(AstUnaryExpr, AstNodeType::UnaryExpr);
TIRO_DEFINE_AST_LEAF(AstVarExpr, AstNodeType::VarExpr);
TIRO_DEFINE_AST_LEAF(AstFile, AstNodeType::File);
TIRO_DEFINE_AST_BASE(AstIdentifier, AstNodeType::FirstIdentifier, AstNodeType::LastIdentifier)
TIRO_DEFINE_AST_LEAF(AstNumericIdentifier, AstNodeType::NumericIdentifier);
TIRO_DEFINE_AST_LEAF(AstStringIdentifier, AstNodeType::StringIdentifier);
TIRO_DEFINE_AST_LEAF(AstMapItem, AstNodeType::MapItem);
TIRO_DEFINE_AST_BASE(AstModifier, AstNodeType::FirstModifier, AstNodeType::LastModifier)
TIRO_DEFINE_AST_LEAF(AstExportModifier, AstNodeType::ExportModifier);
TIRO_DEFINE_AST_LEAF(AstRecordItem, AstNodeType::RecordItem);
TIRO_DEFINE_AST_BASE(AstStmt, AstNodeType::FirstStmt, AstNodeType::LastStmt)
TIRO_DEFINE_AST_LEAF(AstAssertStmt, AstNodeType::AssertStmt);
TIRO_DEFINE_AST_LEAF(AstDeclStmt, AstNodeType::DeclStmt);
TIRO_DEFINE_AST_LEAF(AstDeferStmt, AstNodeType::DeferStmt);
TIRO_DEFINE_AST_LEAF(AstEmptyStmt, AstNodeType::EmptyStmt);
TIRO_DEFINE_AST_LEAF(AstExprStmt, AstNodeType::ExprStmt);
TIRO_DEFINE_AST_LEAF(AstForEachStmt, AstNodeType::ForEachStmt);
TIRO_DEFINE_AST_LEAF(AstForStmt, AstNodeType::ForStmt);
TIRO_DEFINE_AST_LEAF(AstWhileStmt, AstNodeType::WhileStmt);
// [[[end]]]

#undef TIRO_DEFINE_AST_BASE
#undef TIRO_DEFINE_AST_LEAF

} // namespace tiro

#endif // TIRO_COMPILER_AST_NODE_TRAITS_HPP
