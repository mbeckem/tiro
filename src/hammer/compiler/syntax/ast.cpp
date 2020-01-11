#include "hammer/compiler/syntax/ast.hpp"

namespace hammer::compiler {

Node::Node(NodeType type)
    : type_(type) {
    HAMMER_ASSERT(type >= NodeType::FirstNode && type <= NodeType::LastNode,
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
            visit(node, *this);
        }
        return result_;
    }

    void visit_import_decl(ImportDecl* d) HAMMER_VISITOR_OVERRIDE {
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

    void visit_var_decl(VarDecl* d) HAMMER_VISITOR_OVERRIDE {
        props_.emplace_back("is_const", str(d->is_const()));
        visit_decl(d);
    }

    void visit_binary_expr(BinaryExpr* e) HAMMER_VISITOR_OVERRIDE {
        props_.emplace_back("operation", to_string(e->operation()));
        visit_expr(e);
    }

    void visit_unary_expr(UnaryExpr* e) HAMMER_VISITOR_OVERRIDE {
        props_.emplace_back("operation", to_string(e->operation()));
        visit_expr(e);
    }

    void visit_dot_expr(DotExpr* e) HAMMER_VISITOR_OVERRIDE {
        props_.emplace_back("name", str(e->name()));
        visit_expr(e);
    }

    void visit_boolean_literal(BooleanLiteral* e) HAMMER_VISITOR_OVERRIDE {
        props_.emplace_back("value", str(e->value()));
        visit_literal(e);
    }

    void visit_float_literal(FloatLiteral* e) HAMMER_VISITOR_OVERRIDE {
        props_.emplace_back("value", std::to_string(e->value()));
        visit_literal(e);
    }

    void visit_integer_literal(IntegerLiteral* e) HAMMER_VISITOR_OVERRIDE {
        props_.emplace_back("value", std::to_string(e->value()));
        visit_literal(e);
    }

    void visit_string_literal(StringLiteral* e) HAMMER_VISITOR_OVERRIDE {
        props_.emplace_back("value", str(e->value()));
        visit_literal(e);
    }

    void visit_var_expr(VarExpr* e) HAMMER_VISITOR_OVERRIDE {
        props_.emplace_back("name", str(e->name()));
        visit_expr(e);
    }

    void visit_file(File* f) HAMMER_VISITOR_OVERRIDE {
        props_.emplace_back("file_name", str(f->file_name()));
        visit_node(f);
    }

    void visit_decl(Decl* d) HAMMER_VISITOR_OVERRIDE {
        props_.emplace_back("name", str(d->name()));
        visit_node(d);
    }

    void visit_expr(Expr* e) HAMMER_VISITOR_OVERRIDE {
        props_.emplace_back("expr_type", to_string(e->expr_type()));
        props_.emplace_back("observed", str(e->observed()));
        visit_node(e);
    }

    void visit_node(Node* n) HAMMER_VISITOR_OVERRIDE {
        props_.emplace_back("has_error", str(n->has_error()));

        fmt::memory_buffer buf;
        fmt::format_to(buf, "{}(", to_string(n->type()));

        bool first = true;
        for (const auto& pair : props_) {
            if (!first) {
                fmt::format_to(buf, ", ");
            }
            first = false;
            fmt::format_to(buf, "{}={}", pair.first, pair.second);
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

class RecursiveNodePrinter {
public:
    explicit RecursiveNodePrinter(const StringTable& strings)
        : printer_(strings) {}

    void start(Node* node) {
        print_node(node, 0, false);
        dispatch_children(node, 1);
    }

    void dispatch_children(Node* node, int depth) {
        HAMMER_ASSERT(depth > 0, "Invalid depth for child nodes.");

        if (!node)
            return;

        std::vector<Node*> children;
        traverse_children(
            node, [&](auto&& child) { children.push_back(child); });

        const size_t count = children.size();
        if (count == 0)
            return;

        lines_.push_back(depth - 1);
        for (size_t i = 0; i < count - 1; ++i) {
            print_node(children[i], depth, false);
            dispatch_children(children[i], depth + 1);
        }

        print_node(children[count - 1], depth, true);
        lines_.pop_back();
        dispatch_children(children[count - 1], depth + 1);
    }

    void print_node(Node* node, int depth, bool last_child) {
        std::string prefix;
        {
            auto next_line = lines_.begin();
            const auto last_line = lines_.end();
            for (int i = 0; i < depth; ++i) {
                const bool print_line = next_line != last_line
                                        && *next_line == i;
                if (print_line)
                    ++next_line;

                char branch = ' ';
                char space = ' ';
                if (print_line) {
                    branch = i == depth - 1 && last_child ? '`' : '|';
                    space = i == depth - 1 ? '-' : ' ';
                }

                prefix += branch;
                prefix += space;
            }
            HAMMER_ASSERT(
                next_line == last_line, "Did not reach the last line.");
        }

        fmt::format_to(buf_, "{}{}\n", prefix, printer_.dispatch(node));
    }

    std::string result() const { return to_string(buf_); }

private:
    NodePrinter printer_;
    fmt::memory_buffer buf_;
    std::vector<int> lines_;
};

std::string format_node(Node* node, const StringTable& strings) {
    NodePrinter printer(strings);
    return printer.dispatch(node);
}

std::string format_tree(Node* node, const StringTable& strings) {
    RecursiveNodePrinter rec_printer(strings);
    rec_printer.start(node);
    return rec_printer.result();
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

    HAMMER_UNREACHABLE("Invalid expression type.");
}

std::string_view to_string(UnaryOperator op) {
#define HAMMER_CASE(op)     \
    case UnaryOperator::op: \
        return #op;

    switch (op) {
        HAMMER_CASE(Plus)
        HAMMER_CASE(Minus)
        HAMMER_CASE(BitwiseNot)
        HAMMER_CASE(LogicalNot)
    }

#undef HAMMER_CASE

    HAMMER_UNREACHABLE("Invalid unary operation.");
}

std::string_view to_string(BinaryOperator op) {
#define HAMMER_CASE(op)      \
    case BinaryOperator::op: \
        return #op;

    switch (op) {
        HAMMER_CASE(Plus)
        HAMMER_CASE(Minus)
        HAMMER_CASE(Multiply)
        HAMMER_CASE(Divide)
        HAMMER_CASE(Modulus)
        HAMMER_CASE(Power)
        HAMMER_CASE(LeftShift)
        HAMMER_CASE(RightShift)
        HAMMER_CASE(BitwiseOr)
        HAMMER_CASE(BitwiseXor)
        HAMMER_CASE(BitwiseAnd)

        HAMMER_CASE(Less)
        HAMMER_CASE(LessEquals)
        HAMMER_CASE(Greater)
        HAMMER_CASE(GreaterEquals)
        HAMMER_CASE(Equals)
        HAMMER_CASE(NotEquals)
        HAMMER_CASE(LogicalAnd)
        HAMMER_CASE(LogicalOr)

        HAMMER_CASE(Assign)
    }

#undef HAMMER_CASE

    HAMMER_UNREACHABLE("Invalid binary operation.");
}

void traverse_children(Node* node, FunctionRef<void(Node*)> visitor) {
    HAMMER_ASSERT_NOT_NULL(node);
    downcast(node, [&](auto* downcasted) {
        using node_type = std::remove_pointer_t<decltype(downcasted)>;
        NodeTraits<node_type>::traverse_children(downcasted, visitor);
    });
}

void transform_children(Node* node, FunctionRef<NodePtr<>(Node*)> transformer) {
    HAMMER_ASSERT_NOT_NULL(node);
    downcast(node, [&](auto* downcasted) {
        using node_type = std::remove_pointer_t<decltype(downcasted)>;
        NodeTraits<node_type>::transform_children(downcasted, transformer);
    });
}

} // namespace hammer::compiler
