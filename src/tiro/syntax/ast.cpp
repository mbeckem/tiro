#include "tiro/syntax/ast.hpp"

#include "tiro/compiler/utils.hpp"

namespace tiro {

Node::Node(NodeType type)
    : type_(type) {
    TIRO_ASSERT(type >= NodeType::FirstNode && type <= NodeType::LastNode,
        "Invalid node type.");
}

Node::~Node() {}

class NodePrinter : public DefaultNodeVisitor<NodePrinter> {
public:
    explicit NodePrinter(const StringTable& strings)
        : strings_(strings) {}

    const std::string& dispatch(Node* node) {
        props_.clear();
        result_.clear();

        if (!node) {
            result_ = "null";
        } else {
            visit(TIRO_NN(node), *this);
        }
        return result_;
    }

    void visit_import_decl(ImportDecl* d) TIRO_VISITOR_OVERRIDE {
        const size_t path_element_count = d->path_elements().size();

        std::string path;
        for (size_t i = 0; i < path_element_count; ++i) {
            if (i > 0) {
                path += ".";
            }
            path += str(d->path_elements()[i]);
        }
        props_.emplace_back("path", std::move(path));

        visit_decl(d);
    }

    void visit_var_decl(VarDecl* d) TIRO_VISITOR_OVERRIDE {
        props_.emplace_back(d->is_const() ? "is_const" : "", "");
        visit_decl(d);
    }

    void visit_binary_expr(BinaryExpr* e) TIRO_VISITOR_OVERRIDE {
        props_.emplace_back("operation", to_string(e->operation()));
        visit_expr(e);
    }

    void visit_unary_expr(UnaryExpr* e) TIRO_VISITOR_OVERRIDE {
        props_.emplace_back("operation", to_string(e->operation()));
        visit_expr(e);
    }

    void visit_dot_expr(DotExpr* e) TIRO_VISITOR_OVERRIDE {
        props_.emplace_back("name", str(e->name()));
        visit_expr(e);
    }

    void visit_boolean_literal(BooleanLiteral* e) TIRO_VISITOR_OVERRIDE {
        props_.emplace_back("value", str(e->value()));
        visit_literal(e);
    }

    void visit_float_literal(FloatLiteral* e) TIRO_VISITOR_OVERRIDE {
        props_.emplace_back("value", std::to_string(e->value()));
        visit_literal(e);
    }

    void visit_integer_literal(IntegerLiteral* e) TIRO_VISITOR_OVERRIDE {
        props_.emplace_back("value", std::to_string(e->value()));
        visit_literal(e);
    }

    void visit_string_literal(StringLiteral* e) TIRO_VISITOR_OVERRIDE {
        std::string value;
        value += '"';
        value += escape_string(str(e->value()));
        value += '"';

        props_.emplace_back("value", std::move(value));
        visit_literal(e);
    }

    void visit_var_expr(VarExpr* e) TIRO_VISITOR_OVERRIDE {
        props_.emplace_back("name", str(e->name()));
        visit_expr(e);
    }

    void visit_file(File* f) TIRO_VISITOR_OVERRIDE {
        props_.emplace_back("file_name", str(f->file_name()));
        visit_node(f);
    }

    void visit_decl(Decl* d) TIRO_VISITOR_OVERRIDE {
        props_.emplace_back("name", str(d->name()));
        visit_node(d);
    }

    void visit_expr(Expr* e) TIRO_VISITOR_OVERRIDE {
        props_.emplace_back("expr_type", to_string(e->expr_type()));
        visit_node(e);
    }

    void visit_node(Node* n) TIRO_VISITOR_OVERRIDE {
        props_.emplace_back(n->has_error() ? "error" : "", "");

        fmt::memory_buffer buf;
        fmt::format_to(buf, "{}(", to_string(n->type()));

        bool first = true;
        for (const auto& pair : props_) {
            if (pair.first.empty())
                continue;

            if (!first)
                fmt::format_to(buf, ", ");
            first = false;

            if (!pair.second.empty()) {
                fmt::format_to(buf, "{}={}", pair.first, pair.second);
            } else {
                fmt::format_to(buf, "{}", pair.first);
            }
        }
        fmt::format_to(buf, ") @{}", (void*) n);
        result_ = to_string(buf);
    }

private:
    std::string_view str(InternedString s) {
        return s ? strings_.value(s) : "<Invalid String>";
    }

    std::string_view str(bool b) { return b ? "true" : "false"; }

private:
    const StringTable& strings_;
    std::vector<std::pair<std::string, std::string>> props_;
    std::string result_;
};

std::string format_node(Node* node, const StringTable& strings) {
    NodePrinter printer(strings);
    return printer.dispatch(node);
}

static StringTree tree_to_string(Node* root, const StringTable& strings) {
    struct Generator {
        NodePrinter printer;

        Generator(const StringTable& strings)
            : printer(strings) {}

        StringTree operator()(Node* node) {
            StringTree result;
            result.line = printer.dispatch(node);

            if (node) {
                traverse_children(TIRO_NN(node), [&](Node* child) {
                    result.children.push_back(this->operator()(child));
                });
            }
            return result;
        }
    };

    Generator gen(strings);
    return gen(root);
}

std::string format_tree(Node* node, const StringTable& strings) {
    const auto tree = tree_to_string(node, strings);
    return format_tree(tree);
}

std::string_view to_string(ExprType type) {
    switch (type) {
    case ExprType::None:
        return "None";
    case ExprType::Never:
        return "Never";
    case ExprType::Value:
        return "Value";
    }

    TIRO_UNREACHABLE("Invalid expression type.");
}

std::string_view to_string(UnaryOperator op) {
#define TIRO_CASE(op)       \
    case UnaryOperator::op: \
        return #op;

    switch (op) {
        TIRO_CASE(Plus)
        TIRO_CASE(Minus)
        TIRO_CASE(BitwiseNot)
        TIRO_CASE(LogicalNot)
    }

#undef TIRO_CASE

    TIRO_UNREACHABLE("Invalid unary operation.");
}

std::string_view to_string(BinaryOperator op) {
#define TIRO_CASE(op)        \
    case BinaryOperator::op: \
        return #op;

    switch (op) {
        TIRO_CASE(Plus)
        TIRO_CASE(Minus)
        TIRO_CASE(Multiply)
        TIRO_CASE(Divide)
        TIRO_CASE(Modulus)
        TIRO_CASE(Power)
        TIRO_CASE(LeftShift)
        TIRO_CASE(RightShift)
        TIRO_CASE(BitwiseOr)
        TIRO_CASE(BitwiseXor)
        TIRO_CASE(BitwiseAnd)

        TIRO_CASE(Less)
        TIRO_CASE(LessEquals)
        TIRO_CASE(Greater)
        TIRO_CASE(GreaterEquals)
        TIRO_CASE(Equals)
        TIRO_CASE(NotEquals)
        TIRO_CASE(LogicalAnd)
        TIRO_CASE(LogicalOr)

        TIRO_CASE(Assign)
        TIRO_CASE(AssignPlus)
        TIRO_CASE(AssignMinus)
        TIRO_CASE(AssignMultiply)
        TIRO_CASE(AssignDivide)
        TIRO_CASE(AssignModulus)
        TIRO_CASE(AssignPower)
    }

#undef TIRO_CASE

    TIRO_UNREACHABLE("Invalid binary operation.");
}

void traverse_children(NotNull<Node*> node, FunctionRef<void(Node*)> visitor) {
    downcast(node, [&](auto* downcasted) {
        using node_type = std::remove_pointer_t<decltype(downcasted)>;
        NodeTraits<node_type>::traverse_children(downcasted, visitor);
    });
}

void transform_children(
    NotNull<Node*> node, FunctionRef<NodePtr<>(Node*)> transformer) {
    downcast(node, [&](auto* downcasted) {
        using node_type = std::remove_pointer_t<decltype(downcasted)>;
        NodeTraits<node_type>::transform_children(downcasted, transformer);
    });
}

} // namespace tiro
