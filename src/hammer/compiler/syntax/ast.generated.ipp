// Automatically generated by ./utils/generate-ast at 2020-01-03 21:27:53.655425.
// Do not include this file directly and do not edit by hand!

namespace hammer::compiler {

template<typename T, typename Visitor, typename... Arguments>
decltype(auto)
visit(const NodePtr<T>& node, Visitor&& visitor, Arguments&&... args) {
    HAMMER_ASSERT_NOT_NULL(node);

    const NodeType type = node ? node->type() : NodeType(-1);
    switch (type) {

#define HAMMER_VISIT_CASE(CaseType, case_function)        \
    case NodeType::CaseType: {                            \
        if constexpr (std::is_base_of_v<T, CaseType>) {   \
            return visitor.case_function(                 \
                std::static_pointer_cast<CaseType>(node), \
                std::forward<Arguments>(args)...);        \
        }                                                 \
        break;                                            \
    }

        HAMMER_VISIT_CASE(FuncDecl, visit_func_decl)
        HAMMER_VISIT_CASE(ImportDecl, visit_import_decl)
        HAMMER_VISIT_CASE(ParamDecl, visit_param_decl)
        HAMMER_VISIT_CASE(VarDecl, visit_var_decl)
        HAMMER_VISIT_CASE(BinaryExpr, visit_binary_expr)
        HAMMER_VISIT_CASE(BlockExpr, visit_block_expr)
        HAMMER_VISIT_CASE(BreakExpr, visit_break_expr)
        HAMMER_VISIT_CASE(CallExpr, visit_call_expr)
        HAMMER_VISIT_CASE(ContinueExpr, visit_continue_expr)
        HAMMER_VISIT_CASE(DotExpr, visit_dot_expr)
        HAMMER_VISIT_CASE(IfExpr, visit_if_expr)
        HAMMER_VISIT_CASE(IndexExpr, visit_index_expr)
        HAMMER_VISIT_CASE(ArrayLiteral, visit_array_literal)
        HAMMER_VISIT_CASE(BooleanLiteral, visit_boolean_literal)
        HAMMER_VISIT_CASE(FloatLiteral, visit_float_literal)
        HAMMER_VISIT_CASE(FuncLiteral, visit_func_literal)
        HAMMER_VISIT_CASE(IntegerLiteral, visit_integer_literal)
        HAMMER_VISIT_CASE(MapLiteral, visit_map_literal)
        HAMMER_VISIT_CASE(NullLiteral, visit_null_literal)
        HAMMER_VISIT_CASE(SetLiteral, visit_set_literal)
        HAMMER_VISIT_CASE(StringLiteral, visit_string_literal)
        HAMMER_VISIT_CASE(SymbolLiteral, visit_symbol_literal)
        HAMMER_VISIT_CASE(TupleLiteral, visit_tuple_literal)
        HAMMER_VISIT_CASE(ReturnExpr, visit_return_expr)
        HAMMER_VISIT_CASE(StringSequenceExpr, visit_string_sequence_expr)
        HAMMER_VISIT_CASE(UnaryExpr, visit_unary_expr)
        HAMMER_VISIT_CASE(VarExpr, visit_var_expr)
        HAMMER_VISIT_CASE(ExprList, visit_expr_list)
        HAMMER_VISIT_CASE(File, visit_file)
        HAMMER_VISIT_CASE(MapEntry, visit_map_entry)
        HAMMER_VISIT_CASE(MapEntryList, visit_map_entry_list)
        HAMMER_VISIT_CASE(NodeList, visit_node_list)
        HAMMER_VISIT_CASE(ParamList, visit_param_list)
        HAMMER_VISIT_CASE(Root, visit_root)
        HAMMER_VISIT_CASE(AssertStmt, visit_assert_stmt)
        HAMMER_VISIT_CASE(DeclStmt, visit_decl_stmt)
        HAMMER_VISIT_CASE(EmptyStmt, visit_empty_stmt)
        HAMMER_VISIT_CASE(ExprStmt, visit_expr_stmt)
        HAMMER_VISIT_CASE(ForStmt, visit_for_stmt)
        HAMMER_VISIT_CASE(WhileStmt, visit_while_stmt)
        HAMMER_VISIT_CASE(StmtList, visit_stmt_list)

#undef HAMMER_VISIT_CASE
    }
    HAMMER_UNREACHABLE("Broken node type information.");
}

template<typename T, typename Callback>
decltype(auto) downcast(const NodePtr<T>& node, Callback&& callback) {
    HAMMER_ASSERT_NOT_NULL(node);

    const NodeType type = node ? node->type() : NodeType(-1);
    switch (type) {

#define HAMMER_VISIT_CASE(CaseType)                                    \
    case NodeType::CaseType: {                                         \
        if constexpr (std::is_base_of_v<T, CaseType>) {                \
            return callback(std::static_pointer_cast<CaseType>(node)); \
        }                                                              \
        break;                                                         \
    }

        HAMMER_VISIT_CASE(FuncDecl)
        HAMMER_VISIT_CASE(ImportDecl)
        HAMMER_VISIT_CASE(ParamDecl)
        HAMMER_VISIT_CASE(VarDecl)
        HAMMER_VISIT_CASE(BinaryExpr)
        HAMMER_VISIT_CASE(BlockExpr)
        HAMMER_VISIT_CASE(BreakExpr)
        HAMMER_VISIT_CASE(CallExpr)
        HAMMER_VISIT_CASE(ContinueExpr)
        HAMMER_VISIT_CASE(DotExpr)
        HAMMER_VISIT_CASE(IfExpr)
        HAMMER_VISIT_CASE(IndexExpr)
        HAMMER_VISIT_CASE(ArrayLiteral)
        HAMMER_VISIT_CASE(BooleanLiteral)
        HAMMER_VISIT_CASE(FloatLiteral)
        HAMMER_VISIT_CASE(FuncLiteral)
        HAMMER_VISIT_CASE(IntegerLiteral)
        HAMMER_VISIT_CASE(MapLiteral)
        HAMMER_VISIT_CASE(NullLiteral)
        HAMMER_VISIT_CASE(SetLiteral)
        HAMMER_VISIT_CASE(StringLiteral)
        HAMMER_VISIT_CASE(SymbolLiteral)
        HAMMER_VISIT_CASE(TupleLiteral)
        HAMMER_VISIT_CASE(ReturnExpr)
        HAMMER_VISIT_CASE(StringSequenceExpr)
        HAMMER_VISIT_CASE(UnaryExpr)
        HAMMER_VISIT_CASE(VarExpr)
        HAMMER_VISIT_CASE(ExprList)
        HAMMER_VISIT_CASE(File)
        HAMMER_VISIT_CASE(MapEntry)
        HAMMER_VISIT_CASE(MapEntryList)
        HAMMER_VISIT_CASE(NodeList)
        HAMMER_VISIT_CASE(ParamList)
        HAMMER_VISIT_CASE(Root)
        HAMMER_VISIT_CASE(AssertStmt)
        HAMMER_VISIT_CASE(DeclStmt)
        HAMMER_VISIT_CASE(EmptyStmt)
        HAMMER_VISIT_CASE(ExprStmt)
        HAMMER_VISIT_CASE(ForStmt)
        HAMMER_VISIT_CASE(WhileStmt)
        HAMMER_VISIT_CASE(StmtList)

#undef HAMMER_VISIT_CASE
    }
    HAMMER_UNREACHABLE("Broken node type information.");
}

template<>
struct NodeTraits<Decl> {
    static constexpr bool is_abstract = true;

    static constexpr NodeType first_node_type = NodeType::FirstDecl;
    static constexpr NodeType last_node_type = NodeType::LastDecl;

    template<typename NodePointer, typename Visitor>
    static void traverse_children(const NodePointer& node, Visitor&& visitor) {
        NodeTraits<Node>::traverse_children(node, visitor);
    }

    template<typename NodePointer, typename Transform>
    static void
    transform_children(const NodePointer& node, Transform&& transform) {
        NodeTraits<Node>::transform_children(node, transform);
    }
};

template<>
struct NodeTraits<FuncDecl> {
    static constexpr bool is_abstract = false;

    static constexpr NodeType node_type = NodeType::FuncDecl;

    template<typename NodePointer, typename Visitor>
    static void traverse_children(const NodePointer& node, Visitor&& visitor) {
        NodeTraits<Decl>::traverse_children(node, visitor);

        visitor(node->params());
        visitor(node->body());
    }

    template<typename NodePointer, typename Transform>
    static void
    transform_children(const NodePointer& node, Transform&& transform) {
        NodeTraits<Decl>::transform_children(node, transform);

        node->params(transform(node->params()));
        node->body(transform(node->body()));
    }
};

template<>
struct NodeTraits<ImportDecl> {
    static constexpr bool is_abstract = false;

    static constexpr NodeType node_type = NodeType::ImportDecl;

    template<typename NodePointer, typename Visitor>
    static void traverse_children(const NodePointer& node, Visitor&& visitor) {
        NodeTraits<Decl>::traverse_children(node, visitor);
    }

    template<typename NodePointer, typename Transform>
    static void
    transform_children(const NodePointer& node, Transform&& transform) {
        NodeTraits<Decl>::transform_children(node, transform);
    }
};

template<>
struct NodeTraits<ParamDecl> {
    static constexpr bool is_abstract = false;

    static constexpr NodeType node_type = NodeType::ParamDecl;

    template<typename NodePointer, typename Visitor>
    static void traverse_children(const NodePointer& node, Visitor&& visitor) {
        NodeTraits<Decl>::traverse_children(node, visitor);
    }

    template<typename NodePointer, typename Transform>
    static void
    transform_children(const NodePointer& node, Transform&& transform) {
        NodeTraits<Decl>::transform_children(node, transform);
    }
};

template<>
struct NodeTraits<VarDecl> {
    static constexpr bool is_abstract = false;

    static constexpr NodeType node_type = NodeType::VarDecl;

    template<typename NodePointer, typename Visitor>
    static void traverse_children(const NodePointer& node, Visitor&& visitor) {
        NodeTraits<Decl>::traverse_children(node, visitor);

        visitor(node->initializer());
    }

    template<typename NodePointer, typename Transform>
    static void
    transform_children(const NodePointer& node, Transform&& transform) {
        NodeTraits<Decl>::transform_children(node, transform);

        node->initializer(transform(node->initializer()));
    }
};

template<>
struct NodeTraits<Expr> {
    static constexpr bool is_abstract = true;

    static constexpr NodeType first_node_type = NodeType::FirstExpr;
    static constexpr NodeType last_node_type = NodeType::LastExpr;

    template<typename NodePointer, typename Visitor>
    static void traverse_children(const NodePointer& node, Visitor&& visitor) {
        NodeTraits<Node>::traverse_children(node, visitor);
    }

    template<typename NodePointer, typename Transform>
    static void
    transform_children(const NodePointer& node, Transform&& transform) {
        NodeTraits<Node>::transform_children(node, transform);
    }
};

template<>
struct NodeTraits<BinaryExpr> {
    static constexpr bool is_abstract = false;

    static constexpr NodeType node_type = NodeType::BinaryExpr;

    template<typename NodePointer, typename Visitor>
    static void traverse_children(const NodePointer& node, Visitor&& visitor) {
        NodeTraits<Expr>::traverse_children(node, visitor);

        visitor(node->left());
        visitor(node->right());
    }

    template<typename NodePointer, typename Transform>
    static void
    transform_children(const NodePointer& node, Transform&& transform) {
        NodeTraits<Expr>::transform_children(node, transform);

        node->left(transform(node->left()));
        node->right(transform(node->right()));
    }
};

template<>
struct NodeTraits<BlockExpr> {
    static constexpr bool is_abstract = false;

    static constexpr NodeType node_type = NodeType::BlockExpr;

    template<typename NodePointer, typename Visitor>
    static void traverse_children(const NodePointer& node, Visitor&& visitor) {
        NodeTraits<Expr>::traverse_children(node, visitor);

        visitor(node->stmts());
    }

    template<typename NodePointer, typename Transform>
    static void
    transform_children(const NodePointer& node, Transform&& transform) {
        NodeTraits<Expr>::transform_children(node, transform);

        node->stmts(transform(node->stmts()));
    }
};

template<>
struct NodeTraits<BreakExpr> {
    static constexpr bool is_abstract = false;

    static constexpr NodeType node_type = NodeType::BreakExpr;

    template<typename NodePointer, typename Visitor>
    static void traverse_children(const NodePointer& node, Visitor&& visitor) {
        NodeTraits<Expr>::traverse_children(node, visitor);
    }

    template<typename NodePointer, typename Transform>
    static void
    transform_children(const NodePointer& node, Transform&& transform) {
        NodeTraits<Expr>::transform_children(node, transform);
    }
};

template<>
struct NodeTraits<CallExpr> {
    static constexpr bool is_abstract = false;

    static constexpr NodeType node_type = NodeType::CallExpr;

    template<typename NodePointer, typename Visitor>
    static void traverse_children(const NodePointer& node, Visitor&& visitor) {
        NodeTraits<Expr>::traverse_children(node, visitor);

        visitor(node->func());
        visitor(node->args());
    }

    template<typename NodePointer, typename Transform>
    static void
    transform_children(const NodePointer& node, Transform&& transform) {
        NodeTraits<Expr>::transform_children(node, transform);

        node->func(transform(node->func()));
        node->args(transform(node->args()));
    }
};

template<>
struct NodeTraits<ContinueExpr> {
    static constexpr bool is_abstract = false;

    static constexpr NodeType node_type = NodeType::ContinueExpr;

    template<typename NodePointer, typename Visitor>
    static void traverse_children(const NodePointer& node, Visitor&& visitor) {
        NodeTraits<Expr>::traverse_children(node, visitor);
    }

    template<typename NodePointer, typename Transform>
    static void
    transform_children(const NodePointer& node, Transform&& transform) {
        NodeTraits<Expr>::transform_children(node, transform);
    }
};

template<>
struct NodeTraits<DotExpr> {
    static constexpr bool is_abstract = false;

    static constexpr NodeType node_type = NodeType::DotExpr;

    template<typename NodePointer, typename Visitor>
    static void traverse_children(const NodePointer& node, Visitor&& visitor) {
        NodeTraits<Expr>::traverse_children(node, visitor);

        visitor(node->inner());
    }

    template<typename NodePointer, typename Transform>
    static void
    transform_children(const NodePointer& node, Transform&& transform) {
        NodeTraits<Expr>::transform_children(node, transform);

        node->inner(transform(node->inner()));
    }
};

template<>
struct NodeTraits<IfExpr> {
    static constexpr bool is_abstract = false;

    static constexpr NodeType node_type = NodeType::IfExpr;

    template<typename NodePointer, typename Visitor>
    static void traverse_children(const NodePointer& node, Visitor&& visitor) {
        NodeTraits<Expr>::traverse_children(node, visitor);

        visitor(node->condition());
        visitor(node->then_branch());
        visitor(node->else_branch());
    }

    template<typename NodePointer, typename Transform>
    static void
    transform_children(const NodePointer& node, Transform&& transform) {
        NodeTraits<Expr>::transform_children(node, transform);

        node->condition(transform(node->condition()));
        node->then_branch(transform(node->then_branch()));
        node->else_branch(transform(node->else_branch()));
    }
};

template<>
struct NodeTraits<IndexExpr> {
    static constexpr bool is_abstract = false;

    static constexpr NodeType node_type = NodeType::IndexExpr;

    template<typename NodePointer, typename Visitor>
    static void traverse_children(const NodePointer& node, Visitor&& visitor) {
        NodeTraits<Expr>::traverse_children(node, visitor);

        visitor(node->inner());
        visitor(node->index());
    }

    template<typename NodePointer, typename Transform>
    static void
    transform_children(const NodePointer& node, Transform&& transform) {
        NodeTraits<Expr>::transform_children(node, transform);

        node->inner(transform(node->inner()));
        node->index(transform(node->index()));
    }
};

template<>
struct NodeTraits<Literal> {
    static constexpr bool is_abstract = true;

    static constexpr NodeType first_node_type = NodeType::FirstLiteral;
    static constexpr NodeType last_node_type = NodeType::LastLiteral;

    template<typename NodePointer, typename Visitor>
    static void traverse_children(const NodePointer& node, Visitor&& visitor) {
        NodeTraits<Expr>::traverse_children(node, visitor);
    }

    template<typename NodePointer, typename Transform>
    static void
    transform_children(const NodePointer& node, Transform&& transform) {
        NodeTraits<Expr>::transform_children(node, transform);
    }
};

template<>
struct NodeTraits<ArrayLiteral> {
    static constexpr bool is_abstract = false;

    static constexpr NodeType node_type = NodeType::ArrayLiteral;

    template<typename NodePointer, typename Visitor>
    static void traverse_children(const NodePointer& node, Visitor&& visitor) {
        NodeTraits<Literal>::traverse_children(node, visitor);

        visitor(node->entries());
    }

    template<typename NodePointer, typename Transform>
    static void
    transform_children(const NodePointer& node, Transform&& transform) {
        NodeTraits<Literal>::transform_children(node, transform);

        node->entries(transform(node->entries()));
    }
};

template<>
struct NodeTraits<BooleanLiteral> {
    static constexpr bool is_abstract = false;

    static constexpr NodeType node_type = NodeType::BooleanLiteral;

    template<typename NodePointer, typename Visitor>
    static void traverse_children(const NodePointer& node, Visitor&& visitor) {
        NodeTraits<Literal>::traverse_children(node, visitor);
    }

    template<typename NodePointer, typename Transform>
    static void
    transform_children(const NodePointer& node, Transform&& transform) {
        NodeTraits<Literal>::transform_children(node, transform);
    }
};

template<>
struct NodeTraits<FloatLiteral> {
    static constexpr bool is_abstract = false;

    static constexpr NodeType node_type = NodeType::FloatLiteral;

    template<typename NodePointer, typename Visitor>
    static void traverse_children(const NodePointer& node, Visitor&& visitor) {
        NodeTraits<Literal>::traverse_children(node, visitor);
    }

    template<typename NodePointer, typename Transform>
    static void
    transform_children(const NodePointer& node, Transform&& transform) {
        NodeTraits<Literal>::transform_children(node, transform);
    }
};

template<>
struct NodeTraits<FuncLiteral> {
    static constexpr bool is_abstract = false;

    static constexpr NodeType node_type = NodeType::FuncLiteral;

    template<typename NodePointer, typename Visitor>
    static void traverse_children(const NodePointer& node, Visitor&& visitor) {
        NodeTraits<Literal>::traverse_children(node, visitor);

        visitor(node->func());
    }

    template<typename NodePointer, typename Transform>
    static void
    transform_children(const NodePointer& node, Transform&& transform) {
        NodeTraits<Literal>::transform_children(node, transform);

        node->func(transform(node->func()));
    }
};

template<>
struct NodeTraits<IntegerLiteral> {
    static constexpr bool is_abstract = false;

    static constexpr NodeType node_type = NodeType::IntegerLiteral;

    template<typename NodePointer, typename Visitor>
    static void traverse_children(const NodePointer& node, Visitor&& visitor) {
        NodeTraits<Literal>::traverse_children(node, visitor);
    }

    template<typename NodePointer, typename Transform>
    static void
    transform_children(const NodePointer& node, Transform&& transform) {
        NodeTraits<Literal>::transform_children(node, transform);
    }
};

template<>
struct NodeTraits<MapLiteral> {
    static constexpr bool is_abstract = false;

    static constexpr NodeType node_type = NodeType::MapLiteral;

    template<typename NodePointer, typename Visitor>
    static void traverse_children(const NodePointer& node, Visitor&& visitor) {
        NodeTraits<Literal>::traverse_children(node, visitor);

        visitor(node->entries());
    }

    template<typename NodePointer, typename Transform>
    static void
    transform_children(const NodePointer& node, Transform&& transform) {
        NodeTraits<Literal>::transform_children(node, transform);

        node->entries(transform(node->entries()));
    }
};

template<>
struct NodeTraits<NullLiteral> {
    static constexpr bool is_abstract = false;

    static constexpr NodeType node_type = NodeType::NullLiteral;

    template<typename NodePointer, typename Visitor>
    static void traverse_children(const NodePointer& node, Visitor&& visitor) {
        NodeTraits<Literal>::traverse_children(node, visitor);
    }

    template<typename NodePointer, typename Transform>
    static void
    transform_children(const NodePointer& node, Transform&& transform) {
        NodeTraits<Literal>::transform_children(node, transform);
    }
};

template<>
struct NodeTraits<SetLiteral> {
    static constexpr bool is_abstract = false;

    static constexpr NodeType node_type = NodeType::SetLiteral;

    template<typename NodePointer, typename Visitor>
    static void traverse_children(const NodePointer& node, Visitor&& visitor) {
        NodeTraits<Literal>::traverse_children(node, visitor);

        visitor(node->entries());
    }

    template<typename NodePointer, typename Transform>
    static void
    transform_children(const NodePointer& node, Transform&& transform) {
        NodeTraits<Literal>::transform_children(node, transform);

        node->entries(transform(node->entries()));
    }
};

template<>
struct NodeTraits<StringLiteral> {
    static constexpr bool is_abstract = false;

    static constexpr NodeType node_type = NodeType::StringLiteral;

    template<typename NodePointer, typename Visitor>
    static void traverse_children(const NodePointer& node, Visitor&& visitor) {
        NodeTraits<Literal>::traverse_children(node, visitor);
    }

    template<typename NodePointer, typename Transform>
    static void
    transform_children(const NodePointer& node, Transform&& transform) {
        NodeTraits<Literal>::transform_children(node, transform);
    }
};

template<>
struct NodeTraits<SymbolLiteral> {
    static constexpr bool is_abstract = false;

    static constexpr NodeType node_type = NodeType::SymbolLiteral;

    template<typename NodePointer, typename Visitor>
    static void traverse_children(const NodePointer& node, Visitor&& visitor) {
        NodeTraits<Literal>::traverse_children(node, visitor);
    }

    template<typename NodePointer, typename Transform>
    static void
    transform_children(const NodePointer& node, Transform&& transform) {
        NodeTraits<Literal>::transform_children(node, transform);
    }
};

template<>
struct NodeTraits<TupleLiteral> {
    static constexpr bool is_abstract = false;

    static constexpr NodeType node_type = NodeType::TupleLiteral;

    template<typename NodePointer, typename Visitor>
    static void traverse_children(const NodePointer& node, Visitor&& visitor) {
        NodeTraits<Literal>::traverse_children(node, visitor);

        visitor(node->entries());
    }

    template<typename NodePointer, typename Transform>
    static void
    transform_children(const NodePointer& node, Transform&& transform) {
        NodeTraits<Literal>::transform_children(node, transform);

        node->entries(transform(node->entries()));
    }
};

template<>
struct NodeTraits<ReturnExpr> {
    static constexpr bool is_abstract = false;

    static constexpr NodeType node_type = NodeType::ReturnExpr;

    template<typename NodePointer, typename Visitor>
    static void traverse_children(const NodePointer& node, Visitor&& visitor) {
        NodeTraits<Expr>::traverse_children(node, visitor);

        visitor(node->inner());
    }

    template<typename NodePointer, typename Transform>
    static void
    transform_children(const NodePointer& node, Transform&& transform) {
        NodeTraits<Expr>::transform_children(node, transform);

        node->inner(transform(node->inner()));
    }
};

template<>
struct NodeTraits<StringSequenceExpr> {
    static constexpr bool is_abstract = false;

    static constexpr NodeType node_type = NodeType::StringSequenceExpr;

    template<typename NodePointer, typename Visitor>
    static void traverse_children(const NodePointer& node, Visitor&& visitor) {
        NodeTraits<Expr>::traverse_children(node, visitor);

        visitor(node->strings());
    }

    template<typename NodePointer, typename Transform>
    static void
    transform_children(const NodePointer& node, Transform&& transform) {
        NodeTraits<Expr>::transform_children(node, transform);

        node->strings(transform(node->strings()));
    }
};

template<>
struct NodeTraits<UnaryExpr> {
    static constexpr bool is_abstract = false;

    static constexpr NodeType node_type = NodeType::UnaryExpr;

    template<typename NodePointer, typename Visitor>
    static void traverse_children(const NodePointer& node, Visitor&& visitor) {
        NodeTraits<Expr>::traverse_children(node, visitor);

        visitor(node->inner());
    }

    template<typename NodePointer, typename Transform>
    static void
    transform_children(const NodePointer& node, Transform&& transform) {
        NodeTraits<Expr>::transform_children(node, transform);

        node->inner(transform(node->inner()));
    }
};

template<>
struct NodeTraits<VarExpr> {
    static constexpr bool is_abstract = false;

    static constexpr NodeType node_type = NodeType::VarExpr;

    template<typename NodePointer, typename Visitor>
    static void traverse_children(const NodePointer& node, Visitor&& visitor) {
        NodeTraits<Expr>::traverse_children(node, visitor);
    }

    template<typename NodePointer, typename Transform>
    static void
    transform_children(const NodePointer& node, Transform&& transform) {
        NodeTraits<Expr>::transform_children(node, transform);
    }
};

template<>
struct NodeTraits<ExprList> {
    static constexpr bool is_abstract = false;

    static constexpr NodeType node_type = NodeType::ExprList;

    template<typename NodePointer, typename Visitor>
    static void traverse_children(const NodePointer& node, Visitor&& visitor) {
        NodeTraits<Node>::traverse_children(node, visitor);
        NodeListTraits<Expr>::traverse_items(node, visitor);
    }

    template<typename NodePointer, typename Transform>
    static void
    transform_children(const NodePointer& node, Transform&& transform) {
        NodeTraits<Node>::transform_children(node, transform);
        NodeListTraits<Expr>::transform_items(node, transform);
    }
};

template<>
struct NodeTraits<File> {
    static constexpr bool is_abstract = false;

    static constexpr NodeType node_type = NodeType::File;

    template<typename NodePointer, typename Visitor>
    static void traverse_children(const NodePointer& node, Visitor&& visitor) {
        NodeTraits<Node>::traverse_children(node, visitor);

        visitor(node->items());
    }

    template<typename NodePointer, typename Transform>
    static void
    transform_children(const NodePointer& node, Transform&& transform) {
        NodeTraits<Node>::transform_children(node, transform);

        node->items(transform(node->items()));
    }
};

template<>
struct NodeTraits<MapEntry> {
    static constexpr bool is_abstract = false;

    static constexpr NodeType node_type = NodeType::MapEntry;

    template<typename NodePointer, typename Visitor>
    static void traverse_children(const NodePointer& node, Visitor&& visitor) {
        NodeTraits<Node>::traverse_children(node, visitor);

        visitor(node->key());
        visitor(node->value());
    }

    template<typename NodePointer, typename Transform>
    static void
    transform_children(const NodePointer& node, Transform&& transform) {
        NodeTraits<Node>::transform_children(node, transform);

        node->key(transform(node->key()));
        node->value(transform(node->value()));
    }
};

template<>
struct NodeTraits<MapEntryList> {
    static constexpr bool is_abstract = false;

    static constexpr NodeType node_type = NodeType::MapEntryList;

    template<typename NodePointer, typename Visitor>
    static void traverse_children(const NodePointer& node, Visitor&& visitor) {
        NodeTraits<Node>::traverse_children(node, visitor);
        NodeListTraits<MapEntry>::traverse_items(node, visitor);
    }

    template<typename NodePointer, typename Transform>
    static void
    transform_children(const NodePointer& node, Transform&& transform) {
        NodeTraits<Node>::transform_children(node, transform);
        NodeListTraits<MapEntry>::transform_items(node, transform);
    }
};

template<>
struct NodeTraits<NodeList> {
    static constexpr bool is_abstract = false;

    static constexpr NodeType node_type = NodeType::NodeList;

    template<typename NodePointer, typename Visitor>
    static void traverse_children(const NodePointer& node, Visitor&& visitor) {
        NodeTraits<Node>::traverse_children(node, visitor);
        NodeListTraits<Node>::traverse_items(node, visitor);
    }

    template<typename NodePointer, typename Transform>
    static void
    transform_children(const NodePointer& node, Transform&& transform) {
        NodeTraits<Node>::transform_children(node, transform);
        NodeListTraits<Node>::transform_items(node, transform);
    }
};

template<>
struct NodeTraits<ParamList> {
    static constexpr bool is_abstract = false;

    static constexpr NodeType node_type = NodeType::ParamList;

    template<typename NodePointer, typename Visitor>
    static void traverse_children(const NodePointer& node, Visitor&& visitor) {
        NodeTraits<Node>::traverse_children(node, visitor);
        NodeListTraits<ParamDecl>::traverse_items(node, visitor);
    }

    template<typename NodePointer, typename Transform>
    static void
    transform_children(const NodePointer& node, Transform&& transform) {
        NodeTraits<Node>::transform_children(node, transform);
        NodeListTraits<ParamDecl>::transform_items(node, transform);
    }
};

template<>
struct NodeTraits<Root> {
    static constexpr bool is_abstract = false;

    static constexpr NodeType node_type = NodeType::Root;

    template<typename NodePointer, typename Visitor>
    static void traverse_children(const NodePointer& node, Visitor&& visitor) {
        NodeTraits<Node>::traverse_children(node, visitor);

        visitor(node->file());
    }

    template<typename NodePointer, typename Transform>
    static void
    transform_children(const NodePointer& node, Transform&& transform) {
        NodeTraits<Node>::transform_children(node, transform);

        node->file(transform(node->file()));
    }
};

template<>
struct NodeTraits<Stmt> {
    static constexpr bool is_abstract = true;

    static constexpr NodeType first_node_type = NodeType::FirstStmt;
    static constexpr NodeType last_node_type = NodeType::LastStmt;

    template<typename NodePointer, typename Visitor>
    static void traverse_children(const NodePointer& node, Visitor&& visitor) {
        NodeTraits<Node>::traverse_children(node, visitor);
    }

    template<typename NodePointer, typename Transform>
    static void
    transform_children(const NodePointer& node, Transform&& transform) {
        NodeTraits<Node>::transform_children(node, transform);
    }
};

template<>
struct NodeTraits<AssertStmt> {
    static constexpr bool is_abstract = false;

    static constexpr NodeType node_type = NodeType::AssertStmt;

    template<typename NodePointer, typename Visitor>
    static void traverse_children(const NodePointer& node, Visitor&& visitor) {
        NodeTraits<Stmt>::traverse_children(node, visitor);

        visitor(node->condition());
        visitor(node->message());
    }

    template<typename NodePointer, typename Transform>
    static void
    transform_children(const NodePointer& node, Transform&& transform) {
        NodeTraits<Stmt>::transform_children(node, transform);

        node->condition(transform(node->condition()));
        node->message(transform(node->message()));
    }
};

template<>
struct NodeTraits<DeclStmt> {
    static constexpr bool is_abstract = false;

    static constexpr NodeType node_type = NodeType::DeclStmt;

    template<typename NodePointer, typename Visitor>
    static void traverse_children(const NodePointer& node, Visitor&& visitor) {
        NodeTraits<Stmt>::traverse_children(node, visitor);

        visitor(node->decl());
    }

    template<typename NodePointer, typename Transform>
    static void
    transform_children(const NodePointer& node, Transform&& transform) {
        NodeTraits<Stmt>::transform_children(node, transform);

        node->decl(transform(node->decl()));
    }
};

template<>
struct NodeTraits<EmptyStmt> {
    static constexpr bool is_abstract = false;

    static constexpr NodeType node_type = NodeType::EmptyStmt;

    template<typename NodePointer, typename Visitor>
    static void traverse_children(const NodePointer& node, Visitor&& visitor) {
        NodeTraits<Stmt>::traverse_children(node, visitor);
    }

    template<typename NodePointer, typename Transform>
    static void
    transform_children(const NodePointer& node, Transform&& transform) {
        NodeTraits<Stmt>::transform_children(node, transform);
    }
};

template<>
struct NodeTraits<ExprStmt> {
    static constexpr bool is_abstract = false;

    static constexpr NodeType node_type = NodeType::ExprStmt;

    template<typename NodePointer, typename Visitor>
    static void traverse_children(const NodePointer& node, Visitor&& visitor) {
        NodeTraits<Stmt>::traverse_children(node, visitor);

        visitor(node->expr());
    }

    template<typename NodePointer, typename Transform>
    static void
    transform_children(const NodePointer& node, Transform&& transform) {
        NodeTraits<Stmt>::transform_children(node, transform);

        node->expr(transform(node->expr()));
    }
};

template<>
struct NodeTraits<ForStmt> {
    static constexpr bool is_abstract = false;

    static constexpr NodeType node_type = NodeType::ForStmt;

    template<typename NodePointer, typename Visitor>
    static void traverse_children(const NodePointer& node, Visitor&& visitor) {
        NodeTraits<Stmt>::traverse_children(node, visitor);

        visitor(node->decl());
        visitor(node->condition());
        visitor(node->step());
        visitor(node->body());
    }

    template<typename NodePointer, typename Transform>
    static void
    transform_children(const NodePointer& node, Transform&& transform) {
        NodeTraits<Stmt>::transform_children(node, transform);

        node->decl(transform(node->decl()));
        node->condition(transform(node->condition()));
        node->step(transform(node->step()));
        node->body(transform(node->body()));
    }
};

template<>
struct NodeTraits<WhileStmt> {
    static constexpr bool is_abstract = false;

    static constexpr NodeType node_type = NodeType::WhileStmt;

    template<typename NodePointer, typename Visitor>
    static void traverse_children(const NodePointer& node, Visitor&& visitor) {
        NodeTraits<Stmt>::traverse_children(node, visitor);

        visitor(node->condition());
        visitor(node->body());
    }

    template<typename NodePointer, typename Transform>
    static void
    transform_children(const NodePointer& node, Transform&& transform) {
        NodeTraits<Stmt>::transform_children(node, transform);

        node->condition(transform(node->condition()));
        node->body(transform(node->body()));
    }
};

template<>
struct NodeTraits<StmtList> {
    static constexpr bool is_abstract = false;

    static constexpr NodeType node_type = NodeType::StmtList;

    template<typename NodePointer, typename Visitor>
    static void traverse_children(const NodePointer& node, Visitor&& visitor) {
        NodeTraits<Node>::traverse_children(node, visitor);
        NodeListTraits<Stmt>::traverse_items(node, visitor);
    }

    template<typename NodePointer, typename Transform>
    static void
    transform_children(const NodePointer& node, Transform&& transform) {
        NodeTraits<Node>::transform_children(node, transform);
        NodeListTraits<Stmt>::transform_items(node, transform);
    }
};

} // namespace hammer::compiler