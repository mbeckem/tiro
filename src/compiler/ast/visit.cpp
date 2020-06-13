#include "compiler/ast/visit.hpp"

namespace tiro {

MutableAstVisitor::MutableAstVisitor() = default;

MutableAstVisitor::~MutableAstVisitor() = default;

/* [[[cog
    from cog import outl
    from codegen.ast import slot_types

    for index, member in enumerate(slot_types()):
        if index > 0:
            outl()
        
        outl(f"void MutableAstVisitor::{member.visitor_name}({member.cpp_type}& {member.name}) {{ (void) {member.name}; }}")
]]] */
void MutableAstVisitor::visit_binding_list(AstNodeList<AstBinding>& bindings) {
    (void) bindings;
}

void MutableAstVisitor::visit_expr_list(AstNodeList<AstExpr>& args) {
    (void) args;
}

void MutableAstVisitor::visit_item_list(AstNodeList<AstItem>& items) {
    (void) items;
}

void MutableAstVisitor::visit_map_item_list(AstNodeList<AstMapItem>& items) {
    (void) items;
}

void MutableAstVisitor::visit_param_decl_list(AstNodeList<AstParamDecl>& params) {
    (void) params;
}

void MutableAstVisitor::visit_stmt_list(AstNodeList<AstStmt>& stmts) {
    (void) stmts;
}

void MutableAstVisitor::visit_string_expr_list(AstNodeList<AstStringExpr>& strings) {
    (void) strings;
}

void MutableAstVisitor::visit_expr(AstPtr<AstExpr>& init) {
    (void) init;
}

void MutableAstVisitor::visit_func_decl(AstPtr<AstFuncDecl>& decl) {
    (void) decl;
}

void MutableAstVisitor::visit_identifier(AstPtr<AstIdentifier>& property) {
    (void) property;
}

void MutableAstVisitor::visit_item(AstPtr<AstItem>& inner) {
    (void) inner;
}

void MutableAstVisitor::visit_var_decl(AstPtr<AstVarDecl>& decl) {
    (void) decl;
}
// [[[end]]]

} // namespace tiro
