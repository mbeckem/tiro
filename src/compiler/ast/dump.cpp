#include "compiler/ast/dump.hpp"

#include "compiler/ast/ast.hpp"
#include "compiler/source_db.hpp"
#include "compiler/utils.hpp"

#include <nlohmann/json.hpp>

namespace tiro {

using nlohmann::ordered_json;

static ordered_json
map_node(const AstNode* raw_node, const StringTable& strings, const SourceDb& sources);

namespace {

class NodeMapper final {
public:
    explicit NodeMapper(const StringTable& strings, const SourceDb& sources)
        : strings_(strings)
        , sources_(sources) {}

    ordered_json map(const AstNode* node);

private:
    void visit_fields(NotNull<const AstNode*> node);

    template<typename T>
    void visit_field(std::string_view name, T&& data);

    ordered_json format_value(const InternedString& str) {
        if (!str.valid())
            return ordered_json();
        return ordered_json(strings_.value(str));
    }

    ordered_json format_value(const AstNode* node) { return map_node(node, strings_, sources_); }

    template<typename T>
    ordered_json format_value(const AstNodeList<T>& list) {
        ordered_json result = ordered_json::array();
        for (const auto& child : list) {
            result.push_back(format_value(child));
        }
        return result;
    }

    template<typename T>
    ordered_json format_value(const std::vector<T>& value) {
        auto jv = ordered_json::array();
        for (const auto& v : value) {
            jv.push_back(format_value(v));
        }
        return jv;
    }

    ordered_json format_value(const CursorPosition& pos) {
        auto jv = ordered_json::object();
        jv.emplace("line", pos.line());
        jv.emplace("column", pos.column());
        return jv;
    }

    ordered_json format_value(ordered_json jv) { return jv; }

    ordered_json format_value(AstId id) { return id ? ordered_json(id.value()) : ordered_json(); }

    ordered_json format_value(AstNodeType type) { return ordered_json(to_string(type)); }

    template<typename T, std::enable_if_t<supports_formatting<T>()>* = nullptr>
    ordered_json format_value(const T& v) {
        return ordered_json(fmt::format("{}", v));
    }

    template<typename T,
        std::enable_if_t<
            std::is_same_v<T, bool> || std::is_integral_v<T> || std::is_floating_point_v<T>>* =
            nullptr>
    ordered_json format_value(const T& v) {
        return ordered_json(v);
    }

private:
    const StringTable& strings_;
    const SourceDb& sources_;
    ordered_json result_;
};

}; // namespace

static ordered_json
map_node(const AstNode* raw_node, const StringTable& strings, const SourceDb& sources) {
    NodeMapper mapper(strings, sources);
    return mapper.map(raw_node);
}

ordered_json NodeMapper::map(const AstNode* raw_node) {
    if (!raw_node)
        return ordered_json();

    result_ = ordered_json::object();

    auto node = TIRO_NN(raw_node);
    visit_field("type", node->type());
    visit_field("id", node->id());
    if (auto range = node->range()) {
        auto [start, end] = sources_.cursor_pos(node->range());
        visit_field("start", start);
        visit_field("end", end);
    }
    visit_field("has_error", node->has_error());
    visit_fields(node);
    return std::move(result_);
}

void NodeMapper::visit_fields(NotNull<const AstNode*> node) {
    struct FieldVisitor {
        NodeMapper& self;

        /* [[[cog
            from cog import outl
            from codegen.ast import NODE_TYPES, walk_types

            root = NODE_TYPES.get("Node")
            types = list(node for node in walk_types(root) if node is not root)

            for (index, type) in enumerate(types):
                if index > 0:
                    outl()

                base = type.base if type.base is not root else None

                outl(f"void {type.visitor_name}(NotNull<const {type.cpp_name}*> n) {{")
                if base and type.walk_order == "base_first":
                    outl(f"    {type.base.visitor_name}(n);")

                for member in type.members:
                    name = repr(member.name).replace("'", '"')
                    outl(f"    self.visit_field({name}, n->{member.name}());")

                if base and type.walk_order == "derived_first":
                    outl(f"    {type.base.visitor_name}(n);")

                if base is None and len(type.members) == 0:
                    outl(f"(void) n;")

                outl(f"}}")
        ]]] */
        void visit_binding(NotNull<const AstBinding*> n) {
            self.visit_field("is_const", n->is_const());
            self.visit_field("spec", n->spec());
            self.visit_field("init", n->init());
        }

        void visit_binding_spec(NotNull<const AstBindingSpec*> n) { (void) n; }

        void visit_tuple_binding_spec(NotNull<const AstTupleBindingSpec*> n) {
            visit_binding_spec(n);
            self.visit_field("names", n->names());
        }

        void visit_var_binding_spec(NotNull<const AstVarBindingSpec*> n) {
            visit_binding_spec(n);
            self.visit_field("name", n->name());
        }

        void visit_decl(NotNull<const AstDecl*> n) {
            self.visit_field("modifiers", n->modifiers());
        }

        void visit_func_decl(NotNull<const AstFuncDecl*> n) {
            visit_decl(n);
            self.visit_field("name", n->name());
            self.visit_field("body_is_value", n->body_is_value());
            self.visit_field("params", n->params());
            self.visit_field("body", n->body());
        }

        void visit_import_decl(NotNull<const AstImportDecl*> n) {
            visit_decl(n);
            self.visit_field("name", n->name());
            self.visit_field("path", n->path());
        }

        void visit_param_decl(NotNull<const AstParamDecl*> n) {
            visit_decl(n);
            self.visit_field("name", n->name());
        }

        void visit_var_decl(NotNull<const AstVarDecl*> n) {
            visit_decl(n);
            self.visit_field("bindings", n->bindings());
        }

        void visit_expr(NotNull<const AstExpr*> n) { (void) n; }

        void visit_binary_expr(NotNull<const AstBinaryExpr*> n) {
            visit_expr(n);
            self.visit_field("operation", n->operation());
            self.visit_field("left", n->left());
            self.visit_field("right", n->right());
        }

        void visit_block_expr(NotNull<const AstBlockExpr*> n) {
            visit_expr(n);
            self.visit_field("stmts", n->stmts());
        }

        void visit_break_expr(NotNull<const AstBreakExpr*> n) { visit_expr(n); }

        void visit_call_expr(NotNull<const AstCallExpr*> n) {
            visit_expr(n);
            self.visit_field("access_type", n->access_type());
            self.visit_field("func", n->func());
            self.visit_field("args", n->args());
        }

        void visit_continue_expr(NotNull<const AstContinueExpr*> n) { visit_expr(n); }

        void visit_element_expr(NotNull<const AstElementExpr*> n) {
            visit_expr(n);
            self.visit_field("access_type", n->access_type());
            self.visit_field("instance", n->instance());
            self.visit_field("element", n->element());
        }

        void visit_error_expr(NotNull<const AstErrorExpr*> n) { visit_expr(n); }

        void visit_field_expr(NotNull<const AstFieldExpr*> n) {
            visit_expr(n);
            self.visit_field("access_type", n->access_type());
            self.visit_field("instance", n->instance());
            self.visit_field("name", n->name());
        }

        void visit_func_expr(NotNull<const AstFuncExpr*> n) {
            visit_expr(n);
            self.visit_field("decl", n->decl());
        }

        void visit_if_expr(NotNull<const AstIfExpr*> n) {
            visit_expr(n);
            self.visit_field("cond", n->cond());
            self.visit_field("then_branch", n->then_branch());
            self.visit_field("else_branch", n->else_branch());
        }

        void visit_literal(NotNull<const AstLiteral*> n) { visit_expr(n); }

        void visit_array_literal(NotNull<const AstArrayLiteral*> n) {
            visit_literal(n);
            self.visit_field("items", n->items());
        }

        void visit_boolean_literal(NotNull<const AstBooleanLiteral*> n) {
            visit_literal(n);
            self.visit_field("value", n->value());
        }

        void visit_float_literal(NotNull<const AstFloatLiteral*> n) {
            visit_literal(n);
            self.visit_field("value", n->value());
        }

        void visit_integer_literal(NotNull<const AstIntegerLiteral*> n) {
            visit_literal(n);
            self.visit_field("value", n->value());
        }

        void visit_map_literal(NotNull<const AstMapLiteral*> n) {
            visit_literal(n);
            self.visit_field("items", n->items());
        }

        void visit_null_literal(NotNull<const AstNullLiteral*> n) { visit_literal(n); }

        void visit_record_literal(NotNull<const AstRecordLiteral*> n) {
            visit_literal(n);
            self.visit_field("items", n->items());
        }

        void visit_set_literal(NotNull<const AstSetLiteral*> n) {
            visit_literal(n);
            self.visit_field("items", n->items());
        }

        void visit_string_literal(NotNull<const AstStringLiteral*> n) {
            visit_literal(n);
            self.visit_field("value", n->value());
        }

        void visit_symbol_literal(NotNull<const AstSymbolLiteral*> n) {
            visit_literal(n);
            self.visit_field("value", n->value());
        }

        void visit_tuple_literal(NotNull<const AstTupleLiteral*> n) {
            visit_literal(n);
            self.visit_field("items", n->items());
        }

        void visit_return_expr(NotNull<const AstReturnExpr*> n) {
            visit_expr(n);
            self.visit_field("value", n->value());
        }

        void visit_string_expr(NotNull<const AstStringExpr*> n) {
            visit_expr(n);
            self.visit_field("items", n->items());
        }

        void visit_tuple_field_expr(NotNull<const AstTupleFieldExpr*> n) {
            visit_expr(n);
            self.visit_field("access_type", n->access_type());
            self.visit_field("instance", n->instance());
            self.visit_field("index", n->index());
        }

        void visit_unary_expr(NotNull<const AstUnaryExpr*> n) {
            visit_expr(n);
            self.visit_field("operation", n->operation());
            self.visit_field("inner", n->inner());
        }

        void visit_var_expr(NotNull<const AstVarExpr*> n) {
            visit_expr(n);
            self.visit_field("name", n->name());
        }

        void visit_file(NotNull<const AstFile*> n) { self.visit_field("items", n->items()); }

        void visit_identifier(NotNull<const AstIdentifier*> n) {
            self.visit_field("value", n->value());
        }

        void visit_map_item(NotNull<const AstMapItem*> n) {
            self.visit_field("key", n->key());
            self.visit_field("value", n->value());
        }

        void visit_modifier(NotNull<const AstModifier*> n) { (void) n; }

        void visit_export_modifier(NotNull<const AstExportModifier*> n) { visit_modifier(n); }

        void visit_module(NotNull<const AstModule*> n) { self.visit_field("files", n->files()); }

        void visit_record_item(NotNull<const AstRecordItem*> n) {
            self.visit_field("key", n->key());
            self.visit_field("value", n->value());
        }

        void visit_stmt(NotNull<const AstStmt*> n) { (void) n; }

        void visit_assert_stmt(NotNull<const AstAssertStmt*> n) {
            visit_stmt(n);
            self.visit_field("cond", n->cond());
            self.visit_field("message", n->message());
        }

        void visit_decl_stmt(NotNull<const AstDeclStmt*> n) {
            visit_stmt(n);
            self.visit_field("decl", n->decl());
        }

        void visit_defer_stmt(NotNull<const AstDeferStmt*> n) {
            visit_stmt(n);
            self.visit_field("expr", n->expr());
        }

        void visit_empty_stmt(NotNull<const AstEmptyStmt*> n) { visit_stmt(n); }

        void visit_error_stmt(NotNull<const AstErrorStmt*> n) { visit_stmt(n); }

        void visit_expr_stmt(NotNull<const AstExprStmt*> n) {
            visit_stmt(n);
            self.visit_field("expr", n->expr());
        }

        void visit_for_each_stmt(NotNull<const AstForEachStmt*> n) {
            visit_stmt(n);
            self.visit_field("spec", n->spec());
            self.visit_field("expr", n->expr());
            self.visit_field("body", n->body());
        }

        void visit_for_stmt(NotNull<const AstForStmt*> n) {
            visit_stmt(n);
            self.visit_field("decl", n->decl());
            self.visit_field("cond", n->cond());
            self.visit_field("step", n->step());
            self.visit_field("body", n->body());
        }

        void visit_while_stmt(NotNull<const AstWhileStmt*> n) {
            visit_stmt(n);
            self.visit_field("cond", n->cond());
            self.visit_field("body", n->body());
        }
        // [[[end]]]
    };
    visit(node, FieldVisitor{*this});
}

template<typename T>
void NodeMapper::visit_field(std::string_view name, T&& data) {
    result_.emplace(std::string(name), format_value(std::forward<T>(data)));
}

std::string dump(const AstNode* node, const StringTable& strings, const SourceDb& sources) {
    ordered_json simplified = map_node(node, strings, sources);
    return simplified.dump(4);
}

} // namespace tiro
