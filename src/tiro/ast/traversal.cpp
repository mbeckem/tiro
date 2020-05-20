#include "tiro/ast/traversal.hpp"

#include "tiro/ast/ast.hpp"

namespace tiro {

namespace {

class FieldVisitor final {
public:
    explicit FieldVisitor(FunctionRef<void(AstNode*)> callback)
        : cb_(std::move(callback)) {}

    /* [[[cog
        from cog import outl
        from codegen.ast import NodeMember, NodeListMember, walk_types

        for index, type in enumerate(walk_types()):
            if index > 0:
                outl()

            children = [m for m in type.members if isinstance(m, NodeMember) or isinstance(m, NodeListMember)]

            outl(f"void {type.visitor_name}({type.cpp_name}* n) {{")

            if type.base and type.walk_order == "base_first":
                outl(f"    {type.base.visitor_name}(n);")

            for member in children:
                if isinstance(member, NodeMember):
                    outl(f"    child(n->{member.name}());")
                elif isinstance(member, NodeListMember):
                    outl(f"    children(n->{member.name}());")
            
            if type.base and type.walk_order == "derived_first":
                outl(f"    {type.base.visitor_name}(n);")

            if type.base is None and len(children) == 0:
                outl(f"(void) n;")
                

            outl(f"}}")            
    ]]] */
    void visit_node(AstNode* n) { (void) n; }

    void visit_binding(AstBinding* n) {
        visit_node(n);
        child(n->init());
    }

    void visit_tuple_binding(AstTupleBinding* n) { visit_binding(n); }

    void visit_var_binding(AstVarBinding* n) { visit_binding(n); }

    void visit_decl(AstDecl* n) { visit_node(n); }

    void visit_func_decl(AstFuncDecl* n) {
        visit_decl(n);
        children(n->params());
        child(n->body());
    }

    void visit_param_decl(AstParamDecl* n) { visit_decl(n); }

    void visit_var_decl(AstVarDecl* n) {
        visit_decl(n);
        children(n->bindings());
    }

    void visit_expr(AstExpr* n) { visit_node(n); }

    void visit_binary_expr(AstBinaryExpr* n) {
        visit_expr(n);
        child(n->left());
        child(n->right());
    }

    void visit_block_expr(AstBlockExpr* n) {
        visit_expr(n);
        children(n->stmts());
    }

    void visit_break_expr(AstBreakExpr* n) { visit_expr(n); }

    void visit_call_expr(AstCallExpr* n) {
        visit_expr(n);
        child(n->func());
        children(n->args());
    }

    void visit_continue_expr(AstContinueExpr* n) { visit_expr(n); }

    void visit_element_expr(AstElementExpr* n) {
        visit_expr(n);
        child(n->instance());
        child(n->element());
    }

    void visit_func_expr(AstFuncExpr* n) {
        visit_expr(n);
        child(n->decl());
    }

    void visit_if_expr(AstIfExpr* n) {
        visit_expr(n);
        child(n->cond());
        child(n->then_branch());
        child(n->else_branch());
    }

    void visit_literal(AstLiteral* n) { visit_expr(n); }

    void visit_array_literal(AstArrayLiteral* n) {
        visit_literal(n);
        children(n->items());
    }

    void visit_boolean_literal(AstBooleanLiteral* n) { visit_literal(n); }

    void visit_float_literal(AstFloatLiteral* n) { visit_literal(n); }

    void visit_integer_literal(AstIntegerLiteral* n) { visit_literal(n); }

    void visit_map_literal(AstMapLiteral* n) {
        visit_literal(n);
        children(n->items());
    }

    void visit_null_literal(AstNullLiteral* n) { visit_literal(n); }

    void visit_set_literal(AstSetLiteral* n) {
        visit_literal(n);
        children(n->items());
    }

    void visit_string_literal(AstStringLiteral* n) { visit_literal(n); }

    void visit_symbol_literal(AstSymbolLiteral* n) { visit_literal(n); }

    void visit_tuple_literal(AstTupleLiteral* n) {
        visit_literal(n);
        children(n->items());
    }

    void visit_property_expr(AstPropertyExpr* n) {
        visit_expr(n);
        child(n->instance());
        child(n->property());
    }

    void visit_return_expr(AstReturnExpr* n) {
        visit_expr(n);
        child(n->value());
    }

    void visit_string_expr(AstStringExpr* n) {
        visit_expr(n);
        children(n->items());
    }

    void visit_string_group_expr(AstStringGroupExpr* n) {
        visit_expr(n);
        children(n->strings());
    }

    void visit_unary_expr(AstUnaryExpr* n) {
        visit_expr(n);
        child(n->inner());
    }

    void visit_var_expr(AstVarExpr* n) { visit_expr(n); }

    void visit_file(AstFile* n) {
        visit_node(n);
        children(n->items());
    }

    void visit_identifier(AstIdentifier* n) { visit_node(n); }

    void visit_numeric_identifier(AstNumericIdentifier* n) {
        visit_identifier(n);
    }

    void visit_string_identifier(AstStringIdentifier* n) {
        visit_identifier(n);
    }

    void visit_item(AstItem* n) { visit_node(n); }

    void visit_empty_item(AstEmptyItem* n) { visit_item(n); }

    void visit_func_item(AstFuncItem* n) {
        visit_item(n);
        child(n->decl());
    }

    void visit_import_item(AstImportItem* n) { visit_item(n); }

    void visit_var_item(AstVarItem* n) {
        visit_item(n);
        child(n->decl());
    }

    void visit_map_item(AstMapItem* n) {
        visit_node(n);
        child(n->key());
        child(n->value());
    }

    void visit_stmt(AstStmt* n) { visit_node(n); }

    void visit_assert_stmt(AstAssertStmt* n) {
        visit_stmt(n);
        child(n->cond());
        child(n->message());
    }

    void visit_empty_stmt(AstEmptyStmt* n) { visit_stmt(n); }

    void visit_expr_stmt(AstExprStmt* n) {
        visit_stmt(n);
        child(n->expr());
    }

    void visit_for_stmt(AstForStmt* n) {
        visit_stmt(n);
        child(n->decl());
        child(n->cond());
        child(n->step());
        child(n->body());
    }

    void visit_var_stmt(AstVarStmt* n) {
        visit_stmt(n);
        child(n->decl());
    }

    void visit_while_stmt(AstWhileStmt* n) {
        visit_stmt(n);
        child(n->cond());
        child(n->body());
    }
    // [[[end]]]

private:
    template<typename Node>
    void child(Node* node) {
        cb_(node);
    }

    template<typename Node>
    void children(const AstNodeList<Node>& list) {
        for (Node* node : list.items())
            cb_(node);
    }

private:
    FunctionRef<void(AstNode*)> cb_;
};

class MutableFieldVisitor final {
public:
    explicit MutableFieldVisitor(MutableAstVisitor& visitor)
        : visitor_(visitor) {}

    template<typename Node, typename Setter>
    void field(Node* current, Setter&& setter) {}

private:
    MutableAstVisitor& visitor_;
};

} // namespace

MutableAstVisitor::MutableAstVisitor() = default;

MutableAstVisitor::~MutableAstVisitor() = default;

/* [[[cog
    from cog import outl
    from codegen.ast import slot_types

    for (index, node_type) in enumerate(slot_types()):
        if index > 0:
            outl()
        
        outl(f"void MutableAstVisitor::{node_type.visitor_name}(MutableChild<{node_type.cpp_name}> child) {{ (void) child; }}")
]]] */
void MutableAstVisitor::visit_binding(MutableChild<AstBinding> child) {
    (void) child;
}

void MutableAstVisitor::visit_expr(MutableChild<AstExpr> child) {
    (void) child;
}

void MutableAstVisitor::visit_func_decl(MutableChild<AstFuncDecl> child) {
    (void) child;
}

void MutableAstVisitor::visit_identifier(MutableChild<AstIdentifier> child) {
    (void) child;
}

void MutableAstVisitor::visit_item(MutableChild<AstItem> child) {
    (void) child;
}

void MutableAstVisitor::visit_map_item(MutableChild<AstMapItem> child) {
    (void) child;
}

void MutableAstVisitor::visit_param_decl(MutableChild<AstParamDecl> child) {
    (void) child;
}

void MutableAstVisitor::visit_stmt(MutableChild<AstStmt> child) {
    (void) child;
}

void MutableAstVisitor::visit_string_expr(MutableChild<AstStringExpr> child) {
    (void) child;
}

void MutableAstVisitor::visit_var_decl(MutableChild<AstVarDecl> child) {
    (void) child;
}
// [[[end]]]

void visit_children(
    NotNull<const AstNode*> node, FunctionRef<void(AstNode*)> callback) {
    visit(node, FieldVisitor(callback));
}

void mutate_children(NotNull<AstNode*> node, MutableAstVisitor& visitor);

} // namespace tiro
