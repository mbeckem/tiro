#ifndef TIRO_COMPILER_AST_VISIT_HPP
#define TIRO_COMPILER_AST_VISIT_HPP

#include "common/adt/not_null.hpp"
#include "common/type_traits.hpp"
#include "compiler/ast/node.hpp"
#include "compiler/ast/node_traits.hpp"

#include <type_traits>

namespace tiro {

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
template<typename Derived, typename... Args>
class DefaultNodeVisitor {
public:
#ifdef TIRO_DEBUG
#    define TIRO_DEBUG_VIRTUAL virtual
#else
#    define TIRO_DEBUG_VIRTUAL
#endif

    DefaultNodeVisitor() = default;
    TIRO_DEBUG_VIRTUAL ~DefaultNodeVisitor() = default;

    /* [[[cog
        from cog import outl
        from codegen.ast import walk_types
        for index, node_type in enumerate(walk_types()):
            if index > 0:
                outl()

            cpp_name = node_type.cpp_name
            visitor = node_type.visitor_name
            base_visitor = node_type.base.visitor_name if node_type.base is not None else None

            outl(f"TIRO_DEBUG_VIRTUAL void {visitor}(NotNull<{cpp_name}*> node, Args... args) {{")
            if base_visitor is not None:
                outl(f"    derived().{base_visitor}(node, std::forward<Args>(args)...);  ")
            else:
                outl(f"    unused(node, args...);")
            cog.outl(f"}}")
    ]]] */
    TIRO_DEBUG_VIRTUAL void visit_node(NotNull<AstNode*> node, Args... args) {
        unused(node, args...);
    }

    TIRO_DEBUG_VIRTUAL void visit_binding(NotNull<AstBinding*> node, Args... args) {
        derived().visit_node(node, std::forward<Args>(args)...);
    }

    TIRO_DEBUG_VIRTUAL void visit_binding_spec(NotNull<AstBindingSpec*> node, Args... args) {
        derived().visit_node(node, std::forward<Args>(args)...);
    }

    TIRO_DEBUG_VIRTUAL void
    visit_tuple_binding_spec(NotNull<AstTupleBindingSpec*> node, Args... args) {
        derived().visit_binding_spec(node, std::forward<Args>(args)...);
    }

    TIRO_DEBUG_VIRTUAL void visit_var_binding_spec(NotNull<AstVarBindingSpec*> node, Args... args) {
        derived().visit_binding_spec(node, std::forward<Args>(args)...);
    }

    TIRO_DEBUG_VIRTUAL void visit_decl(NotNull<AstDecl*> node, Args... args) {
        derived().visit_node(node, std::forward<Args>(args)...);
    }

    TIRO_DEBUG_VIRTUAL void visit_func_decl(NotNull<AstFuncDecl*> node, Args... args) {
        derived().visit_decl(node, std::forward<Args>(args)...);
    }

    TIRO_DEBUG_VIRTUAL void visit_import_decl(NotNull<AstImportDecl*> node, Args... args) {
        derived().visit_decl(node, std::forward<Args>(args)...);
    }

    TIRO_DEBUG_VIRTUAL void visit_param_decl(NotNull<AstParamDecl*> node, Args... args) {
        derived().visit_decl(node, std::forward<Args>(args)...);
    }

    TIRO_DEBUG_VIRTUAL void visit_var_decl(NotNull<AstVarDecl*> node, Args... args) {
        derived().visit_decl(node, std::forward<Args>(args)...);
    }

    TIRO_DEBUG_VIRTUAL void visit_expr(NotNull<AstExpr*> node, Args... args) {
        derived().visit_node(node, std::forward<Args>(args)...);
    }

    TIRO_DEBUG_VIRTUAL void visit_binary_expr(NotNull<AstBinaryExpr*> node, Args... args) {
        derived().visit_expr(node, std::forward<Args>(args)...);
    }

    TIRO_DEBUG_VIRTUAL void visit_block_expr(NotNull<AstBlockExpr*> node, Args... args) {
        derived().visit_expr(node, std::forward<Args>(args)...);
    }

    TIRO_DEBUG_VIRTUAL void visit_break_expr(NotNull<AstBreakExpr*> node, Args... args) {
        derived().visit_expr(node, std::forward<Args>(args)...);
    }

    TIRO_DEBUG_VIRTUAL void visit_call_expr(NotNull<AstCallExpr*> node, Args... args) {
        derived().visit_expr(node, std::forward<Args>(args)...);
    }

    TIRO_DEBUG_VIRTUAL void visit_continue_expr(NotNull<AstContinueExpr*> node, Args... args) {
        derived().visit_expr(node, std::forward<Args>(args)...);
    }

    TIRO_DEBUG_VIRTUAL void visit_element_expr(NotNull<AstElementExpr*> node, Args... args) {
        derived().visit_expr(node, std::forward<Args>(args)...);
    }

    TIRO_DEBUG_VIRTUAL void visit_error_expr(NotNull<AstErrorExpr*> node, Args... args) {
        derived().visit_expr(node, std::forward<Args>(args)...);
    }

    TIRO_DEBUG_VIRTUAL void visit_field_expr(NotNull<AstFieldExpr*> node, Args... args) {
        derived().visit_expr(node, std::forward<Args>(args)...);
    }

    TIRO_DEBUG_VIRTUAL void visit_func_expr(NotNull<AstFuncExpr*> node, Args... args) {
        derived().visit_expr(node, std::forward<Args>(args)...);
    }

    TIRO_DEBUG_VIRTUAL void visit_if_expr(NotNull<AstIfExpr*> node, Args... args) {
        derived().visit_expr(node, std::forward<Args>(args)...);
    }

    TIRO_DEBUG_VIRTUAL void visit_literal(NotNull<AstLiteral*> node, Args... args) {
        derived().visit_expr(node, std::forward<Args>(args)...);
    }

    TIRO_DEBUG_VIRTUAL void visit_array_literal(NotNull<AstArrayLiteral*> node, Args... args) {
        derived().visit_literal(node, std::forward<Args>(args)...);
    }

    TIRO_DEBUG_VIRTUAL void visit_boolean_literal(NotNull<AstBooleanLiteral*> node, Args... args) {
        derived().visit_literal(node, std::forward<Args>(args)...);
    }

    TIRO_DEBUG_VIRTUAL void visit_float_literal(NotNull<AstFloatLiteral*> node, Args... args) {
        derived().visit_literal(node, std::forward<Args>(args)...);
    }

    TIRO_DEBUG_VIRTUAL void visit_integer_literal(NotNull<AstIntegerLiteral*> node, Args... args) {
        derived().visit_literal(node, std::forward<Args>(args)...);
    }

    TIRO_DEBUG_VIRTUAL void visit_map_literal(NotNull<AstMapLiteral*> node, Args... args) {
        derived().visit_literal(node, std::forward<Args>(args)...);
    }

    TIRO_DEBUG_VIRTUAL void visit_null_literal(NotNull<AstNullLiteral*> node, Args... args) {
        derived().visit_literal(node, std::forward<Args>(args)...);
    }

    TIRO_DEBUG_VIRTUAL void visit_record_literal(NotNull<AstRecordLiteral*> node, Args... args) {
        derived().visit_literal(node, std::forward<Args>(args)...);
    }

    TIRO_DEBUG_VIRTUAL void visit_set_literal(NotNull<AstSetLiteral*> node, Args... args) {
        derived().visit_literal(node, std::forward<Args>(args)...);
    }

    TIRO_DEBUG_VIRTUAL void visit_string_literal(NotNull<AstStringLiteral*> node, Args... args) {
        derived().visit_literal(node, std::forward<Args>(args)...);
    }

    TIRO_DEBUG_VIRTUAL void visit_symbol_literal(NotNull<AstSymbolLiteral*> node, Args... args) {
        derived().visit_literal(node, std::forward<Args>(args)...);
    }

    TIRO_DEBUG_VIRTUAL void visit_tuple_literal(NotNull<AstTupleLiteral*> node, Args... args) {
        derived().visit_literal(node, std::forward<Args>(args)...);
    }

    TIRO_DEBUG_VIRTUAL void visit_return_expr(NotNull<AstReturnExpr*> node, Args... args) {
        derived().visit_expr(node, std::forward<Args>(args)...);
    }

    TIRO_DEBUG_VIRTUAL void visit_string_expr(NotNull<AstStringExpr*> node, Args... args) {
        derived().visit_expr(node, std::forward<Args>(args)...);
    }

    TIRO_DEBUG_VIRTUAL void visit_tuple_field_expr(NotNull<AstTupleFieldExpr*> node, Args... args) {
        derived().visit_expr(node, std::forward<Args>(args)...);
    }

    TIRO_DEBUG_VIRTUAL void visit_unary_expr(NotNull<AstUnaryExpr*> node, Args... args) {
        derived().visit_expr(node, std::forward<Args>(args)...);
    }

    TIRO_DEBUG_VIRTUAL void visit_var_expr(NotNull<AstVarExpr*> node, Args... args) {
        derived().visit_expr(node, std::forward<Args>(args)...);
    }

    TIRO_DEBUG_VIRTUAL void visit_file(NotNull<AstFile*> node, Args... args) {
        derived().visit_node(node, std::forward<Args>(args)...);
    }

    TIRO_DEBUG_VIRTUAL void visit_identifier(NotNull<AstIdentifier*> node, Args... args) {
        derived().visit_node(node, std::forward<Args>(args)...);
    }

    TIRO_DEBUG_VIRTUAL void visit_map_item(NotNull<AstMapItem*> node, Args... args) {
        derived().visit_node(node, std::forward<Args>(args)...);
    }

    TIRO_DEBUG_VIRTUAL void visit_modifier(NotNull<AstModifier*> node, Args... args) {
        derived().visit_node(node, std::forward<Args>(args)...);
    }

    TIRO_DEBUG_VIRTUAL void visit_export_modifier(NotNull<AstExportModifier*> node, Args... args) {
        derived().visit_modifier(node, std::forward<Args>(args)...);
    }

    TIRO_DEBUG_VIRTUAL void visit_module(NotNull<AstModule*> node, Args... args) {
        derived().visit_node(node, std::forward<Args>(args)...);
    }

    TIRO_DEBUG_VIRTUAL void visit_record_item(NotNull<AstRecordItem*> node, Args... args) {
        derived().visit_node(node, std::forward<Args>(args)...);
    }

    TIRO_DEBUG_VIRTUAL void visit_stmt(NotNull<AstStmt*> node, Args... args) {
        derived().visit_node(node, std::forward<Args>(args)...);
    }

    TIRO_DEBUG_VIRTUAL void visit_assert_stmt(NotNull<AstAssertStmt*> node, Args... args) {
        derived().visit_stmt(node, std::forward<Args>(args)...);
    }

    TIRO_DEBUG_VIRTUAL void visit_decl_stmt(NotNull<AstDeclStmt*> node, Args... args) {
        derived().visit_stmt(node, std::forward<Args>(args)...);
    }

    TIRO_DEBUG_VIRTUAL void visit_defer_stmt(NotNull<AstDeferStmt*> node, Args... args) {
        derived().visit_stmt(node, std::forward<Args>(args)...);
    }

    TIRO_DEBUG_VIRTUAL void visit_empty_stmt(NotNull<AstEmptyStmt*> node, Args... args) {
        derived().visit_stmt(node, std::forward<Args>(args)...);
    }

    TIRO_DEBUG_VIRTUAL void visit_error_stmt(NotNull<AstErrorStmt*> node, Args... args) {
        derived().visit_stmt(node, std::forward<Args>(args)...);
    }

    TIRO_DEBUG_VIRTUAL void visit_expr_stmt(NotNull<AstExprStmt*> node, Args... args) {
        derived().visit_stmt(node, std::forward<Args>(args)...);
    }

    TIRO_DEBUG_VIRTUAL void visit_for_each_stmt(NotNull<AstForEachStmt*> node, Args... args) {
        derived().visit_stmt(node, std::forward<Args>(args)...);
    }

    TIRO_DEBUG_VIRTUAL void visit_for_stmt(NotNull<AstForStmt*> node, Args... args) {
        derived().visit_stmt(node, std::forward<Args>(args)...);
    }

    TIRO_DEBUG_VIRTUAL void visit_while_stmt(NotNull<AstWhileStmt*> node, Args... args) {
        derived().visit_stmt(node, std::forward<Args>(args)...);
    }
    // [[[end]]]

#undef TIRO_VISIT_FN

private:
    Derived& derived() { return static_cast<Derived&>(*this); }

    template<typename... T>
    void unused(T&&...) {}
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
    virtual void visit_file_list(AstNodeList<AstFile>& files);
    virtual void visit_identifier_list(AstNodeList<AstIdentifier>& names);
    virtual void visit_map_item_list(AstNodeList<AstMapItem>& items);
    virtual void visit_modifier_list(AstNodeList<AstModifier>& modifiers);
    virtual void visit_param_decl_list(AstNodeList<AstParamDecl>& params);
    virtual void visit_record_item_list(AstNodeList<AstRecordItem>& items);
    virtual void visit_stmt_list(AstNodeList<AstStmt>& stmts);
    virtual void visit_binding_spec(AstPtr<AstBindingSpec>& spec);
    virtual void visit_decl(AstPtr<AstDecl>& decl);
    virtual void visit_expr(AstPtr<AstExpr>& init);
    virtual void visit_func_decl(AstPtr<AstFuncDecl>& decl);
    virtual void visit_identifier(AstPtr<AstIdentifier>& name);
    virtual void visit_var_decl(AstPtr<AstVarDecl>& decl);
    // [[[end]]]
};

/// Invokes `visitor(node)`, where with the node casted to its most derived type.
/// Additional arguments are passed to the visitor as-is.
/// Returns the result of calling the visitor.
template<typename Node, typename Visitor, typename... Args,
    std::enable_if_t<std::is_base_of_v<AstNode, Node>>* = nullptr>
decltype(auto) match(NotNull<Node*> node, Visitor&& visitor, Args&&... args) {
    switch (node->type()) {

// User only has to define visit function for possible types
#define TIRO_VISIT(TypeName)                                                              \
case AstNodeTraits<TypeName>::type_id:                                                    \
    if constexpr (std::is_base_of_v<Node, TypeName>) {                                    \
        using Target = preserve_const_t<TypeName, Node>;                                  \
        return visitor(static_not_null_cast<Target*>(node), std::forward<Args>(args)...); \
    } else {                                                                              \
        TIRO_UNREACHABLE("Logic error: inconsistent type ids.");                          \
    }

        /* [[[cog
            import cog
            from codegen.ast import walk_concrete_types
            for node_type in walk_concrete_types():
                cog.outl(f"TIRO_VISIT({node_type.cpp_name})")
        ]]] */
        TIRO_VISIT(AstBinding)
        TIRO_VISIT(AstTupleBindingSpec)
        TIRO_VISIT(AstVarBindingSpec)
        TIRO_VISIT(AstFuncDecl)
        TIRO_VISIT(AstImportDecl)
        TIRO_VISIT(AstParamDecl)
        TIRO_VISIT(AstVarDecl)
        TIRO_VISIT(AstBinaryExpr)
        TIRO_VISIT(AstBlockExpr)
        TIRO_VISIT(AstBreakExpr)
        TIRO_VISIT(AstCallExpr)
        TIRO_VISIT(AstContinueExpr)
        TIRO_VISIT(AstElementExpr)
        TIRO_VISIT(AstErrorExpr)
        TIRO_VISIT(AstFieldExpr)
        TIRO_VISIT(AstFuncExpr)
        TIRO_VISIT(AstIfExpr)
        TIRO_VISIT(AstArrayLiteral)
        TIRO_VISIT(AstBooleanLiteral)
        TIRO_VISIT(AstFloatLiteral)
        TIRO_VISIT(AstIntegerLiteral)
        TIRO_VISIT(AstMapLiteral)
        TIRO_VISIT(AstNullLiteral)
        TIRO_VISIT(AstRecordLiteral)
        TIRO_VISIT(AstSetLiteral)
        TIRO_VISIT(AstStringLiteral)
        TIRO_VISIT(AstSymbolLiteral)
        TIRO_VISIT(AstTupleLiteral)
        TIRO_VISIT(AstReturnExpr)
        TIRO_VISIT(AstStringExpr)
        TIRO_VISIT(AstTupleFieldExpr)
        TIRO_VISIT(AstUnaryExpr)
        TIRO_VISIT(AstVarExpr)
        TIRO_VISIT(AstFile)
        TIRO_VISIT(AstIdentifier)
        TIRO_VISIT(AstMapItem)
        TIRO_VISIT(AstExportModifier)
        TIRO_VISIT(AstModule)
        TIRO_VISIT(AstRecordItem)
        TIRO_VISIT(AstAssertStmt)
        TIRO_VISIT(AstDeclStmt)
        TIRO_VISIT(AstDeferStmt)
        TIRO_VISIT(AstEmptyStmt)
        TIRO_VISIT(AstErrorStmt)
        TIRO_VISIT(AstExprStmt)
        TIRO_VISIT(AstForEachStmt)
        TIRO_VISIT(AstForStmt)
        TIRO_VISIT(AstWhileStmt)
        // [[[end]]]

#undef TIRO_VISIT
    }

    TIRO_UNREACHABLE("Invalid node type id.");
}

/// Invokes `visitor.visit_TYPE_NAME(node)` and returns the result of calling that function.
/// Additional arguments are passed to the visitor as-is.
template<typename Node, typename Visitor, typename... Args,
    std::enable_if_t<std::is_base_of_v<AstNode, Node>>* = nullptr>
decltype(auto) visit(NotNull<Node*> node, Visitor&& visitor, Args&&... args) {
    switch (node->type()) {

// User only has to define visit function for possible types
#define TIRO_VISIT(TypeName, VisitorName)                                      \
case AstNodeTraits<TypeName>::type_id:                                         \
    if constexpr (std::is_base_of_v<Node, TypeName>) {                         \
        using Target = preserve_const_t<TypeName, Node>;                       \
        return visitor.VisitorName(                                            \
            static_not_null_cast<Target*>(node), std::forward<Args>(args)...); \
    } else {                                                                   \
        TIRO_UNREACHABLE("Logic error: inconsistent type ids.");               \
    }

        /* [[[cog
            import cog
            from codegen.ast import walk_concrete_types
            types = list((node_type.cpp_name, node_type.visitor_name) 
                            for node_type in walk_concrete_types())
            for (cpp_name, visitor_name) in types:
                cog.outl(f"TIRO_VISIT({cpp_name}, {visitor_name})")
        ]]] */
        TIRO_VISIT(AstBinding, visit_binding)
        TIRO_VISIT(AstTupleBindingSpec, visit_tuple_binding_spec)
        TIRO_VISIT(AstVarBindingSpec, visit_var_binding_spec)
        TIRO_VISIT(AstFuncDecl, visit_func_decl)
        TIRO_VISIT(AstImportDecl, visit_import_decl)
        TIRO_VISIT(AstParamDecl, visit_param_decl)
        TIRO_VISIT(AstVarDecl, visit_var_decl)
        TIRO_VISIT(AstBinaryExpr, visit_binary_expr)
        TIRO_VISIT(AstBlockExpr, visit_block_expr)
        TIRO_VISIT(AstBreakExpr, visit_break_expr)
        TIRO_VISIT(AstCallExpr, visit_call_expr)
        TIRO_VISIT(AstContinueExpr, visit_continue_expr)
        TIRO_VISIT(AstElementExpr, visit_element_expr)
        TIRO_VISIT(AstErrorExpr, visit_error_expr)
        TIRO_VISIT(AstFieldExpr, visit_field_expr)
        TIRO_VISIT(AstFuncExpr, visit_func_expr)
        TIRO_VISIT(AstIfExpr, visit_if_expr)
        TIRO_VISIT(AstArrayLiteral, visit_array_literal)
        TIRO_VISIT(AstBooleanLiteral, visit_boolean_literal)
        TIRO_VISIT(AstFloatLiteral, visit_float_literal)
        TIRO_VISIT(AstIntegerLiteral, visit_integer_literal)
        TIRO_VISIT(AstMapLiteral, visit_map_literal)
        TIRO_VISIT(AstNullLiteral, visit_null_literal)
        TIRO_VISIT(AstRecordLiteral, visit_record_literal)
        TIRO_VISIT(AstSetLiteral, visit_set_literal)
        TIRO_VISIT(AstStringLiteral, visit_string_literal)
        TIRO_VISIT(AstSymbolLiteral, visit_symbol_literal)
        TIRO_VISIT(AstTupleLiteral, visit_tuple_literal)
        TIRO_VISIT(AstReturnExpr, visit_return_expr)
        TIRO_VISIT(AstStringExpr, visit_string_expr)
        TIRO_VISIT(AstTupleFieldExpr, visit_tuple_field_expr)
        TIRO_VISIT(AstUnaryExpr, visit_unary_expr)
        TIRO_VISIT(AstVarExpr, visit_var_expr)
        TIRO_VISIT(AstFile, visit_file)
        TIRO_VISIT(AstIdentifier, visit_identifier)
        TIRO_VISIT(AstMapItem, visit_map_item)
        TIRO_VISIT(AstExportModifier, visit_export_modifier)
        TIRO_VISIT(AstModule, visit_module)
        TIRO_VISIT(AstRecordItem, visit_record_item)
        TIRO_VISIT(AstAssertStmt, visit_assert_stmt)
        TIRO_VISIT(AstDeclStmt, visit_decl_stmt)
        TIRO_VISIT(AstDeferStmt, visit_defer_stmt)
        TIRO_VISIT(AstEmptyStmt, visit_empty_stmt)
        TIRO_VISIT(AstErrorStmt, visit_error_stmt)
        TIRO_VISIT(AstExprStmt, visit_expr_stmt)
        TIRO_VISIT(AstForEachStmt, visit_for_each_stmt)
        TIRO_VISIT(AstForStmt, visit_for_stmt)
        TIRO_VISIT(AstWhileStmt, visit_while_stmt)
        // [[[end]]]
    }

    TIRO_UNREACHABLE("Invalid ast node type.");
}

} // namespace tiro

#endif // TIRO_COMPILER_AST_VISIT_HPP
