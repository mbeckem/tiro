#ifndef TIRO_AST_VISIT_HPP
#define TIRO_AST_VISIT_HPP

#include "tiro/ast/node.hpp"
#include "tiro/ast/node_traits.hpp"
#include "tiro/core/not_null.hpp"
#include "tiro/core/type_traits.hpp"

#include <type_traits>

namespace tiro {

namespace detail {

// Provides named visitor functions for better readablity of node matchers.
template<typename Node, typename Visitor>
struct NamedNodeVisitor {
    Visitor& visitor;

    template<typename Target>
    using TargetType = preserve_const_t<Target, Node>;

    NamedNodeVisitor(Visitor& visitor_)
        : visitor(visitor_) {}

#define TIRO_VISIT(TypeName, VisitFunction)                          \
    decltype(auto) operator()(NotNull<TargetType<TypeName>*> n) {    \
        if constexpr (std::is_base_of_v<Node, TypeName>) {           \
            return visitor.VisitFunction(n);                         \
        } else {                                                     \
            TIRO_UNREACHABLE("Logic error: inconsistent type ids."); \
        }                                                            \
    }

    /* [[[cog
        import cog
        from codegen.ast import walk_concrete_types
        types = list((node_type.cpp_name, node_type.visitor_name) 
                        for node_type in walk_concrete_types())
        for (cpp_name, visitor_name) in types:
            cog.outl(f"TIRO_VISIT({cpp_name}, {visitor_name})")
    ]]] */
    TIRO_VISIT(AstTupleBinding, visit_tuple_binding)
    TIRO_VISIT(AstVarBinding, visit_var_binding)
    TIRO_VISIT(AstFuncDecl, visit_func_decl)
    TIRO_VISIT(AstParamDecl, visit_param_decl)
    TIRO_VISIT(AstVarDecl, visit_var_decl)
    TIRO_VISIT(AstBinaryExpr, visit_binary_expr)
    TIRO_VISIT(AstBlockExpr, visit_block_expr)
    TIRO_VISIT(AstBreakExpr, visit_break_expr)
    TIRO_VISIT(AstCallExpr, visit_call_expr)
    TIRO_VISIT(AstContinueExpr, visit_continue_expr)
    TIRO_VISIT(AstElementExpr, visit_element_expr)
    TIRO_VISIT(AstFuncExpr, visit_func_expr)
    TIRO_VISIT(AstIfExpr, visit_if_expr)
    TIRO_VISIT(AstArrayLiteral, visit_array_literal)
    TIRO_VISIT(AstBooleanLiteral, visit_boolean_literal)
    TIRO_VISIT(AstFloatLiteral, visit_float_literal)
    TIRO_VISIT(AstIntegerLiteral, visit_integer_literal)
    TIRO_VISIT(AstMapLiteral, visit_map_literal)
    TIRO_VISIT(AstNullLiteral, visit_null_literal)
    TIRO_VISIT(AstSetLiteral, visit_set_literal)
    TIRO_VISIT(AstStringLiteral, visit_string_literal)
    TIRO_VISIT(AstSymbolLiteral, visit_symbol_literal)
    TIRO_VISIT(AstTupleLiteral, visit_tuple_literal)
    TIRO_VISIT(AstPropertyExpr, visit_property_expr)
    TIRO_VISIT(AstReturnExpr, visit_return_expr)
    TIRO_VISIT(AstStringExpr, visit_string_expr)
    TIRO_VISIT(AstUnaryExpr, visit_unary_expr)
    TIRO_VISIT(AstVarExpr, visit_var_expr)
    TIRO_VISIT(AstFile, visit_file)
    TIRO_VISIT(AstNumericIdentifier, visit_numeric_identifier)
    TIRO_VISIT(AstStringIdentifier, visit_string_identifier)
    TIRO_VISIT(AstEmptyItem, visit_empty_item)
    TIRO_VISIT(AstFuncItem, visit_func_item)
    TIRO_VISIT(AstImportItem, visit_import_item)
    TIRO_VISIT(AstVarItem, visit_var_item)
    TIRO_VISIT(AstMapItem, visit_map_item)
    TIRO_VISIT(AstAssertStmt, visit_assert_stmt)
    TIRO_VISIT(AstEmptyStmt, visit_empty_stmt)
    TIRO_VISIT(AstExprStmt, visit_expr_stmt)
    TIRO_VISIT(AstForStmt, visit_for_stmt)
    TIRO_VISIT(AstVarStmt, visit_var_stmt)
    TIRO_VISIT(AstWhileStmt, visit_while_stmt)
    // [[[end]]]
};

} // namespace detail

template<typename Node, typename Visitor,
    std::enable_if_t<std::is_base_of_v<AstNode, Node>>* = nullptr>
decltype(auto) match(NotNull<Node*> node, Visitor&& visitor) {

    switch (node->type()) {

// User only has to define visit function for possible types
#define TIRO_VISIT(TypeName)                                     \
case AstNodeTraits<TypeName>::type_id:                           \
    if constexpr (std::is_base_of_v<Node, TypeName>) {           \
        using Target = preserve_const_t<TypeName, Node>;         \
        return visitor(static_not_null_cast<Target*>(node));     \
    } else {                                                     \
        TIRO_UNREACHABLE("Logic error: inconsistent type ids."); \
    }

        /* [[[cog
            import cog
            from codegen.ast import walk_concrete_types
            for node_type in walk_concrete_types():
                cog.outl(f"TIRO_VISIT({node_type.cpp_name})")
        ]]] */
        TIRO_VISIT(AstTupleBinding)
        TIRO_VISIT(AstVarBinding)
        TIRO_VISIT(AstFuncDecl)
        TIRO_VISIT(AstParamDecl)
        TIRO_VISIT(AstVarDecl)
        TIRO_VISIT(AstBinaryExpr)
        TIRO_VISIT(AstBlockExpr)
        TIRO_VISIT(AstBreakExpr)
        TIRO_VISIT(AstCallExpr)
        TIRO_VISIT(AstContinueExpr)
        TIRO_VISIT(AstElementExpr)
        TIRO_VISIT(AstFuncExpr)
        TIRO_VISIT(AstIfExpr)
        TIRO_VISIT(AstArrayLiteral)
        TIRO_VISIT(AstBooleanLiteral)
        TIRO_VISIT(AstFloatLiteral)
        TIRO_VISIT(AstIntegerLiteral)
        TIRO_VISIT(AstMapLiteral)
        TIRO_VISIT(AstNullLiteral)
        TIRO_VISIT(AstSetLiteral)
        TIRO_VISIT(AstStringLiteral)
        TIRO_VISIT(AstSymbolLiteral)
        TIRO_VISIT(AstTupleLiteral)
        TIRO_VISIT(AstPropertyExpr)
        TIRO_VISIT(AstReturnExpr)
        TIRO_VISIT(AstStringExpr)
        TIRO_VISIT(AstUnaryExpr)
        TIRO_VISIT(AstVarExpr)
        TIRO_VISIT(AstFile)
        TIRO_VISIT(AstNumericIdentifier)
        TIRO_VISIT(AstStringIdentifier)
        TIRO_VISIT(AstEmptyItem)
        TIRO_VISIT(AstFuncItem)
        TIRO_VISIT(AstImportItem)
        TIRO_VISIT(AstVarItem)
        TIRO_VISIT(AstMapItem)
        TIRO_VISIT(AstAssertStmt)
        TIRO_VISIT(AstEmptyStmt)
        TIRO_VISIT(AstExprStmt)
        TIRO_VISIT(AstForStmt)
        TIRO_VISIT(AstVarStmt)
        TIRO_VISIT(AstWhileStmt)
        // [[[end]]]

#undef TIRO_VISIT
    }

    TIRO_UNREACHABLE("Invalid node type id.");
}

template<typename Node, typename Visitor,
    std::enable_if_t<std::is_base_of_v<AstNode, Node>>* = nullptr>
decltype(auto) visit(NotNull<Node*> node, Visitor&& visitor) {
    return match(node, detail::NamedNodeVisitor<Node, Visitor>(visitor));
}

} // namespace tiro

#endif // TIRO_AST_VISIT_HPP
