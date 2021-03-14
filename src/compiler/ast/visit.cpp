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

void MutableAstVisitor::visit_map_item_list(AstNodeList<AstMapItem>& items) {
    (void) items;
}

void MutableAstVisitor::visit_modifier_list(AstNodeList<AstModifier>& modifiers) {
    (void) modifiers;
}

void MutableAstVisitor::visit_param_decl_list(AstNodeList<AstParamDecl>& params) {
    (void) params;
}

void MutableAstVisitor::visit_record_item_list(AstNodeList<AstRecordItem>& items) {
    (void) items;
}

void MutableAstVisitor::visit_stmt_list(AstNodeList<AstStmt>& stmts) {
    (void) stmts;
}

void MutableAstVisitor::visit_string_identifier_list(AstNodeList<AstStringIdentifier>& names) {
    (void) names;
}

void MutableAstVisitor::visit_binding_spec(AstPtr<AstBindingSpec>& spec) {
    (void) spec;
}

void MutableAstVisitor::visit_decl(AstPtr<AstDecl>& decl) {
    (void) decl;
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

void MutableAstVisitor::visit_string_identifier(AstPtr<AstStringIdentifier>& name) {
    (void) name;
}

void MutableAstVisitor::visit_var_decl(AstPtr<AstVarDecl>& decl) {
    (void) decl;
}
// [[[end]]]

} // namespace tiro
