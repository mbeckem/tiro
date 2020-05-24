#ifndef TIRO_AST_VISIT_HPP
#define TIRO_AST_VISIT_HPP

#include "tiro/ast/node.hpp"
#include "tiro/ast/node_traits.hpp"
#include "tiro/core/not_null.hpp"
#include "tiro/core/type_traits.hpp"

#include <type_traits>

namespace tiro {

namespace detail {

// Provides named visitor functions for better readability of node matchers.
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
    TIRO_VISIT(AstStringGroupExpr, visit_string_group_expr)
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

#ifdef TIRO_DEBUG
/// Use this macro to make sure that you overwrite the correct member function. There is
/// no overhead in release builds.
#    define TIRO_NODE_VISITOR_OVERRIDE override
#else
#    define TIRO_NODE_VISITOR_OVERRIDE
#endif

/// A default implementation for node visitation that can be used in conjunction with `visit(node, visitor)`.
/// The visitor function implementation for every possible node type simply forwards to the visitor function
/// for the node's base type. If not overwritten, `visit_node()` will ultimately be called.
template<typename Derived>
class DefaultNodeVisitor {
public:
    DefaultNodeVisitor() = default;

#ifdef TIRO_DEBUG
#    define TIRO_VISIT_FN virtual
#endif

    /* [[[cog
        from cog import outl
        from codegen.ast import walk_types
        for index, node_type in enumerate(walk_types()):
            if index > 0:
                outl()

            cpp_name = node_type.cpp_name
            visitor = node_type.visitor_name
            base_visitor = node_type.base.visitor_name if node_type.base is not None else None

            outl(f"TIRO_VISIT_FN void {visitor}(NotNull<{cpp_name}*> node) {{")
            if base_visitor is not None:
                outl(f"    derived().{base_visitor}(node);")
            else:
                outl(f"    (void) node;")
            cog.outl(f"}}")
    ]]] */
    TIRO_VISIT_FN void visit_node(NotNull<AstNode*> node) { (void) node; }

    TIRO_VISIT_FN void visit_binding(NotNull<AstBinding*> node) {
        derived().visit_node(node);
    }

    TIRO_VISIT_FN void visit_tuple_binding(NotNull<AstTupleBinding*> node) {
        derived().visit_binding(node);
    }

    TIRO_VISIT_FN void visit_var_binding(NotNull<AstVarBinding*> node) {
        derived().visit_binding(node);
    }

    TIRO_VISIT_FN void visit_decl(NotNull<AstDecl*> node) {
        derived().visit_node(node);
    }

    TIRO_VISIT_FN void visit_func_decl(NotNull<AstFuncDecl*> node) {
        derived().visit_decl(node);
    }

    TIRO_VISIT_FN void visit_param_decl(NotNull<AstParamDecl*> node) {
        derived().visit_decl(node);
    }

    TIRO_VISIT_FN void visit_var_decl(NotNull<AstVarDecl*> node) {
        derived().visit_decl(node);
    }

    TIRO_VISIT_FN void visit_expr(NotNull<AstExpr*> node) {
        derived().visit_node(node);
    }

    TIRO_VISIT_FN void visit_binary_expr(NotNull<AstBinaryExpr*> node) {
        derived().visit_expr(node);
    }

    TIRO_VISIT_FN void visit_block_expr(NotNull<AstBlockExpr*> node) {
        derived().visit_expr(node);
    }

    TIRO_VISIT_FN void visit_break_expr(NotNull<AstBreakExpr*> node) {
        derived().visit_expr(node);
    }

    TIRO_VISIT_FN void visit_call_expr(NotNull<AstCallExpr*> node) {
        derived().visit_expr(node);
    }

    TIRO_VISIT_FN void visit_continue_expr(NotNull<AstContinueExpr*> node) {
        derived().visit_expr(node);
    }

    TIRO_VISIT_FN void visit_element_expr(NotNull<AstElementExpr*> node) {
        derived().visit_expr(node);
    }

    TIRO_VISIT_FN void visit_func_expr(NotNull<AstFuncExpr*> node) {
        derived().visit_expr(node);
    }

    TIRO_VISIT_FN void visit_if_expr(NotNull<AstIfExpr*> node) {
        derived().visit_expr(node);
    }

    TIRO_VISIT_FN void visit_literal(NotNull<AstLiteral*> node) {
        derived().visit_expr(node);
    }

    TIRO_VISIT_FN void visit_array_literal(NotNull<AstArrayLiteral*> node) {
        derived().visit_literal(node);
    }

    TIRO_VISIT_FN void visit_boolean_literal(NotNull<AstBooleanLiteral*> node) {
        derived().visit_literal(node);
    }

    TIRO_VISIT_FN void visit_float_literal(NotNull<AstFloatLiteral*> node) {
        derived().visit_literal(node);
    }

    TIRO_VISIT_FN void visit_integer_literal(NotNull<AstIntegerLiteral*> node) {
        derived().visit_literal(node);
    }

    TIRO_VISIT_FN void visit_map_literal(NotNull<AstMapLiteral*> node) {
        derived().visit_literal(node);
    }

    TIRO_VISIT_FN void visit_null_literal(NotNull<AstNullLiteral*> node) {
        derived().visit_literal(node);
    }

    TIRO_VISIT_FN void visit_set_literal(NotNull<AstSetLiteral*> node) {
        derived().visit_literal(node);
    }

    TIRO_VISIT_FN void visit_string_literal(NotNull<AstStringLiteral*> node) {
        derived().visit_literal(node);
    }

    TIRO_VISIT_FN void visit_symbol_literal(NotNull<AstSymbolLiteral*> node) {
        derived().visit_literal(node);
    }

    TIRO_VISIT_FN void visit_tuple_literal(NotNull<AstTupleLiteral*> node) {
        derived().visit_literal(node);
    }

    TIRO_VISIT_FN void visit_property_expr(NotNull<AstPropertyExpr*> node) {
        derived().visit_expr(node);
    }

    TIRO_VISIT_FN void visit_return_expr(NotNull<AstReturnExpr*> node) {
        derived().visit_expr(node);
    }

    TIRO_VISIT_FN void visit_string_expr(NotNull<AstStringExpr*> node) {
        derived().visit_expr(node);
    }

    TIRO_VISIT_FN void
    visit_string_group_expr(NotNull<AstStringGroupExpr*> node) {
        derived().visit_expr(node);
    }

    TIRO_VISIT_FN void visit_unary_expr(NotNull<AstUnaryExpr*> node) {
        derived().visit_expr(node);
    }

    TIRO_VISIT_FN void visit_var_expr(NotNull<AstVarExpr*> node) {
        derived().visit_expr(node);
    }

    TIRO_VISIT_FN void visit_file(NotNull<AstFile*> node) {
        derived().visit_node(node);
    }

    TIRO_VISIT_FN void visit_identifier(NotNull<AstIdentifier*> node) {
        derived().visit_node(node);
    }

    TIRO_VISIT_FN void
    visit_numeric_identifier(NotNull<AstNumericIdentifier*> node) {
        derived().visit_identifier(node);
    }

    TIRO_VISIT_FN void
    visit_string_identifier(NotNull<AstStringIdentifier*> node) {
        derived().visit_identifier(node);
    }

    TIRO_VISIT_FN void visit_item(NotNull<AstItem*> node) {
        derived().visit_node(node);
    }

    TIRO_VISIT_FN void visit_empty_item(NotNull<AstEmptyItem*> node) {
        derived().visit_item(node);
    }

    TIRO_VISIT_FN void visit_func_item(NotNull<AstFuncItem*> node) {
        derived().visit_item(node);
    }

    TIRO_VISIT_FN void visit_import_item(NotNull<AstImportItem*> node) {
        derived().visit_item(node);
    }

    TIRO_VISIT_FN void visit_var_item(NotNull<AstVarItem*> node) {
        derived().visit_item(node);
    }

    TIRO_VISIT_FN void visit_map_item(NotNull<AstMapItem*> node) {
        derived().visit_node(node);
    }

    TIRO_VISIT_FN void visit_stmt(NotNull<AstStmt*> node) {
        derived().visit_node(node);
    }

    TIRO_VISIT_FN void visit_assert_stmt(NotNull<AstAssertStmt*> node) {
        derived().visit_stmt(node);
    }

    TIRO_VISIT_FN void visit_empty_stmt(NotNull<AstEmptyStmt*> node) {
        derived().visit_stmt(node);
    }

    TIRO_VISIT_FN void visit_expr_stmt(NotNull<AstExprStmt*> node) {
        derived().visit_stmt(node);
    }

    TIRO_VISIT_FN void visit_for_stmt(NotNull<AstForStmt*> node) {
        derived().visit_stmt(node);
    }

    TIRO_VISIT_FN void visit_var_stmt(NotNull<AstVarStmt*> node) {
        derived().visit_stmt(node);
    }

    TIRO_VISIT_FN void visit_while_stmt(NotNull<AstWhileStmt*> node) {
        derived().visit_stmt(node);
    }
    // [[[end]]]

#undef TIRO_VISIT_FN

private:
    Derived& derived() { return static_cast<Derived&>(*this); }
};

/// This interface must be implemented by callers that wish to modify the AST.
/// The visitor will be invoked for every child slot within a parent node.
/// Note that the default implementations of the visit_* functions do nothing.
class MutableAstVisitor {
public:
    MutableAstVisitor();
    virtual ~MutableAstVisitor();

    /* [[[cog
        from cog import outl
        from codegen.ast import slot_types

        for member in slot_types():
            outl(f"virtual void {member.visitor_name}({member.cpp_type}& {member.name});")
    ]]] */
    virtual void visit_binding_list(AstNodeList<AstBinding>& bindings);
    virtual void visit_expr_list(AstNodeList<AstExpr>& args);
    virtual void visit_item_list(AstNodeList<AstItem>& items);
    virtual void visit_map_item_list(AstNodeList<AstMapItem>& items);
    virtual void visit_param_decl_list(AstNodeList<AstParamDecl>& params);
    virtual void visit_stmt_list(AstNodeList<AstStmt>& stmts);
    virtual void visit_string_expr_list(AstNodeList<AstStringExpr>& strings);
    virtual void visit_expr(AstPtr<AstExpr>& init);
    virtual void visit_func_decl(AstPtr<AstFuncDecl>& decl);
    virtual void visit_identifier(AstPtr<AstIdentifier>& property);
    virtual void visit_var_decl(AstPtr<AstVarDecl>& decl);
    // [[[end]]]
};

/// Invokes `visitor(node)`, where with the node casted to its most derived type.
/// Returns the result of calling the visitor.
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
        TIRO_VISIT(AstStringGroupExpr)
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

/// Invokes `visitor.visit_TYPE_NAME(node)` and returns the result of calling that function.
template<typename Node, typename Visitor,
    std::enable_if_t<std::is_base_of_v<AstNode, Node>>* = nullptr>
decltype(auto) visit(NotNull<Node*> node, Visitor&& visitor) {
    return match(node, detail::NamedNodeVisitor<Node, Visitor>(visitor));
}

} // namespace tiro

#endif // TIRO_AST_VISIT_HPP
