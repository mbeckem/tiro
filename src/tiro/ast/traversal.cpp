#include "tiro/ast/traversal.hpp"

namespace tiro {

template<typename ExprRange>
static void walk_exprs(ExprRange&& exprs, AstVisitor& v) {
    for (auto&& expr : exprs) {
        v.visit_expr(expr);
    }
}

template<typename StmtRange>
static void walk_stmts(StmtRange&& stmts, AstVisitor& v) {
    for (auto&& stmt : stmts) {
        v.visit_stmt(stmt);
    }
}

template<typename DeclRange>
static void walk_decls(DeclRange&& decls, AstVisitor& v) {
    for (auto&& decl : decls) {
        v.visit_decl(decl);
    }
}

void walk_expr(AstExpr& expr, AstVisitor& v) {
    struct Walker {
        AstVisitor& v;

        void visit_block(AstExprData::Block& block) {
            walk_stmts(block.stmts, v);
        }

        void visit_unary(AstExprData::Unary& unary) {
            v.visit_expr(unary.inner);
        }

        void visit_binary(AstExprData::Binary& binary) {
            v.visit_expr(binary.left);
            v.visit_expr(binary.right);
        }

        void visit_var(AstExprData::Var&) {}

        void visit_property_access(AstExprData::PropertyAccess& prop) {
            v.visit_expr(prop.instance);
        }

        void visit_element_access(AstExprData::ElementAccess& elem) {
            v.visit_expr(elem.instance);
            v.visit_expr(elem.element);
        }

        void visit_call(AstExprData::Call& call) {
            v.visit_expr(call.func);
            walk_exprs(call.args, v);
        }

        void visit_if(AstExprData::If& i) {
            v.visit_expr(i.cond);
            v.visit_expr(i.then_branch);
            v.visit_expr(i.else_branch);
        }

        void visit_return(AstExprData::Return& ret) { v.visit_expr(ret.value); }

        void visit_break(AstExprData::Break&) {}

        void visit_continue(AstExprData::Continue&) {}

        void visit_string_sequence(AstExprData::StringSequence& seq) {
            walk_exprs(seq.strings, v);
        }

        void visit_interpolated_string(AstExprData::InterpolatedString& ips) {
            walk_exprs(ips.strings, v);
        }

        void visit_null(AstExprData::Null&) {}

        void visit_boolean(AstExprData::Boolean&) {}

        void visit_integer(AstExprData::Integer&) {}

        void visit_float(AstExprData::Float&) {}

        void visit_string(AstExprData::String&) {}

        void visit_symbol(AstExprData::Symbol&) {}

        void visit_array(AstExprData::Array& array) {
            walk_exprs(array.items, v);
        }

        void visit_tuple(AstExprData::Tuple& tuple) {
            walk_exprs(tuple.items, v);
        }

        void visit_set(AstExprData::Set& set) { walk_exprs(set.items, v); }

        void visit_map(AstExprData::Map& map) {
            walk_exprs(map.keys, v);
            walk_exprs(map.values, v);
        }

        void visit_func(AstExprData::Func& func) { v.visit_decl(func.decl); }
    };
    expr.data.visit(Walker{v});
}

void walk_stmt(AstStmt& stmt, AstVisitor& v) {
    (void) stmt;
    (void) v;
    // TODO
}

void walk_decl(AstDecl& decl, AstVisitor& v) {
    struct Walker {
        AstVisitor& v;

        void visit_func(AstDeclData::Func& func) {
            walk_decls(func.params, v);
            v.visit_expr(func.body);
        }

        void visit_var(AstDeclData::Var& var) { v.visit_expr(var.init); }

        void visit_tuple(AstDeclData::Tuple& tuple) {
            v.visit_expr(tuple.init);
        }

        void visit_import(AstDeclData::Import&) {}
    };
    decl.data.visit(Walker{v});
}

} // namespace tiro
