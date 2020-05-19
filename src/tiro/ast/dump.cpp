#include "tiro/ast/dump.hpp"

#include "tiro/ast/ast.hpp"
#include "tiro/compiler/utils.hpp"

#include <nlohmann/json.hpp>

namespace tiro {

namespace {

using nlohmann::json;

template<typename T>
struct IsNodeList : std::false_type {};

template<typename T>
struct IsNodeList<AstNodeList<T>> : std::true_type {};

template<typename T>
inline constexpr bool is_node_ptr =
    std::is_pointer_v<T>&& std::is_base_of_v<AstNode, std::remove_pointer_t<T>>;

template<typename T>
inline constexpr bool is_node_list = IsNodeList<remove_cvref_t<T>>::value;

class NodeMapper final {
public:
    explicit NodeMapper(const StringTable& strings)
        : strings_(strings) {}

    json map(const AstNode* node);

private:
    void visit_fields(NotNull<const AstNode*> node);

    void visit_field(std::string_view name, AstNode* node) {
        return visit_field(name, const_cast<const AstNode*>(node));
    }

    void visit_field(std::string_view name, const AstNode* node);

    template<typename T>
    void visit_field(std::string_view name, const AstNodeList<T>& list);

    template<typename T,
        std::enable_if_t<!is_node_ptr<T> && !is_node_list<T>>* = nullptr>
    void visit_field(std::string_view name, const T& data);

    template<typename T>
    json format_field(const std::vector<T>& value);

    template<typename T>
    json format_field(const T& value);

private:
    const StringTable& strings_;
    json result_;
};

}; // namespace

static json map_node(const AstNode* raw_node, const StringTable& strings) {
    NodeMapper mapper(strings);
    return mapper.map(raw_node);
}

template<typename Node>
static json
map_list(const AstNodeList<Node>& list, const StringTable& strings) {
    json result = json::array();
    for (const Node* child : list) {
        result.push_back(map_node(child, strings));
    }
    return result;
}

json NodeMapper::map(const AstNode* raw_node) {
    if (!raw_node)
        return json();

    result_ = json::object();

    auto node = TIRO_NN(raw_node);
    visit_field("type", node->type());
    visit_field("id", node->id());
    visit_field("source", node->source());
    visit_field("flags", node->flags());
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
            self.visit_field("init", n->init());
        }

        void visit_tuple_binding(NotNull<const AstTupleBinding*> n) {
            self.visit_field("names", n->names());
            visit_binding(n);
        }

        void visit_var_binding(NotNull<const AstVarBinding*> n) {
            self.visit_field("name", n->name());
            visit_binding(n);
        }

        void visit_decl(NotNull<const AstDecl*> n) { (void) n; }

        void visit_func_decl(NotNull<const AstFuncDecl*> n) {
            visit_decl(n);
            self.visit_field("name", n->name());
            self.visit_field("body_is_value", n->body_is_value());
            self.visit_field("params", n->params());
            self.visit_field("body", n->body());
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

        void visit_continue_expr(NotNull<const AstContinueExpr*> n) {
            visit_expr(n);
        }

        void visit_element_expr(NotNull<const AstElementExpr*> n) {
            visit_expr(n);
            self.visit_field("access_type", n->access_type());
            self.visit_field("instance", n->instance());
            self.visit_field("element", n->element());
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

        void visit_null_literal(NotNull<const AstNullLiteral*> n) {
            visit_literal(n);
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

        void visit_property_expr(NotNull<const AstPropertyExpr*> n) {
            visit_expr(n);
            self.visit_field("access_type", n->access_type());
            self.visit_field("instance", n->instance());
            self.visit_field("property", n->property());
        }

        void visit_return_expr(NotNull<const AstReturnExpr*> n) {
            visit_expr(n);
            self.visit_field("value", n->value());
        }

        void visit_string_expr(NotNull<const AstStringExpr*> n) {
            visit_expr(n);
            self.visit_field("items", n->items());
        }

        void visit_string_group_expr(NotNull<const AstStringGroupExpr*> n) {
            visit_expr(n);
            self.visit_field("strings", n->strings());
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

        void visit_file(NotNull<const AstFile*> n) {
            self.visit_field("items", n->items());
        }

        void visit_identifier(NotNull<const AstIdentifier*> n) { (void) n; }

        void visit_numeric_identifier(NotNull<const AstNumericIdentifier*> n) {
            visit_identifier(n);
            self.visit_field("value", n->value());
        }

        void visit_string_identifier(NotNull<const AstStringIdentifier*> n) {
            visit_identifier(n);
            self.visit_field("value", n->value());
        }

        void visit_item(NotNull<const AstItem*> n) { (void) n; }

        void visit_empty_item(NotNull<const AstEmptyItem*> n) { visit_item(n); }

        void visit_func_item(NotNull<const AstFuncItem*> n) {
            visit_item(n);
            self.visit_field("decl", n->decl());
        }

        void visit_import_item(NotNull<const AstImportItem*> n) {
            visit_item(n);
            self.visit_field("name", n->name());
            self.visit_field("path", n->path());
        }

        void visit_var_item(NotNull<const AstVarItem*> n) {
            visit_item(n);
            self.visit_field("decl", n->decl());
        }

        void visit_map_item(NotNull<const AstMapItem*> n) {
            self.visit_field("key", n->key());
            self.visit_field("value", n->value());
        }

        void visit_stmt(NotNull<const AstStmt*> n) { (void) n; }

        void visit_assert_stmt(NotNull<const AstAssertStmt*> n) {
            visit_stmt(n);
            self.visit_field("cond", n->cond());
            self.visit_field("message", n->message());
        }

        void visit_empty_stmt(NotNull<const AstEmptyStmt*> n) { visit_stmt(n); }

        void visit_expr_stmt(NotNull<const AstExprStmt*> n) {
            visit_stmt(n);
            self.visit_field("expr", n->expr());
        }

        void visit_for_stmt(NotNull<const AstForStmt*> n) {
            visit_stmt(n);
            self.visit_field("decl", n->decl());
            self.visit_field("cond", n->cond());
            self.visit_field("step", n->step());
            self.visit_field("body", n->body());
        }

        void visit_var_stmt(NotNull<const AstVarStmt*> n) {
            visit_stmt(n);
            self.visit_field("decl", n->decl());
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

void NodeMapper::visit_field(std::string_view name, const AstNode* child) {
    result_[std::string(name)] = map_node(child, strings_);
}

template<typename T>
void NodeMapper::visit_field(
    std::string_view name, const AstNodeList<T>& children) {
    result_[std::string(name)] = map_list(children, strings_);
}

template<typename T, std::enable_if_t<!is_node_ptr<T> && !is_node_list<T>>*>
void NodeMapper::visit_field(std::string_view name, const T& data) {
    result_[std::string(name)] = format_field(data);
}

template<typename T>
json NodeMapper::format_field(const std::vector<T>& value) {
    json result = json::array();
    for (const auto& item : value) {
        result.push_back(format_field(item));
    }
    return result;
}

template<typename T>
json NodeMapper::format_field(const T& value) {
    if constexpr (std::is_same_v<T, InternedString>) {
        return strings_.dump(value);
    } else if constexpr (std::is_same_v<T, bool>) {
        return value ? "true" : "false";
    } else if constexpr (std::is_integral_v<T> || std::is_floating_point_v<T>) {
        return value;
    } else {
        return fmt::format("{}", value);
    }
}

std::string dump(const AstNode* node, const StringTable& strings) {
    json simplified = map_node(node, strings);
    return simplified.dump(4);
}

} // namespace tiro
