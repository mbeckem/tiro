// Automatically generated by ./utils/generate-ast at 2020-01-14 19:51:40.984974.
// Do not include this file directly and do not edit by hand!

namespace hammer::compiler {

template<typename T, typename Visitor, typename... Arguments>
decltype(auto) visit(T* node, Visitor&& visitor, Arguments&&... args) {
    HAMMER_ASSERT_NOT_NULL(node);

    const NodeType type = node ? node->type() : NodeType(-1);
    switch (type) {

#define HAMMER_VISIT_CASE(CaseType, case_function)                            \
    case NodeType::CaseType: {                                                \
        if constexpr (std::is_base_of_v<T, CaseType>) {                       \
            return visitor.case_function(                                     \
                must_cast<CaseType>(node), std::forward<Arguments>(args)...); \
        }                                                                     \
        break;                                                                \
    }

        HAMMER_VISIT_CASE(TupleBinding, visit_tuple_binding)
        HAMMER_VISIT_CASE(VarBinding, visit_var_binding)
        HAMMER_VISIT_CASE(BindingList, visit_binding_list)
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
        HAMMER_VISIT_CASE(
            InterpolatedStringExpr, visit_interpolated_string_expr)
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
        HAMMER_VISIT_CASE(TupleMemberExpr, visit_tuple_member_expr)
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
        HAMMER_VISIT_CASE(VarList, visit_var_list)

#undef HAMMER_VISIT_CASE
    }
    HAMMER_UNREACHABLE("Broken node type information.");
}

template<typename T, typename Callback>
decltype(auto) downcast(T* node, Callback&& callback) {
    HAMMER_ASSERT_NOT_NULL(node);

    const NodeType type = node ? node->type() : NodeType(-1);
    switch (type) {

#define HAMMER_VISIT_CASE(CaseType)                     \
    case NodeType::CaseType: {                          \
        if constexpr (std::is_base_of_v<T, CaseType>) { \
            return callback(must_cast<CaseType>(node)); \
        }                                               \
        break;                                          \
    }

        HAMMER_VISIT_CASE(TupleBinding)
        HAMMER_VISIT_CASE(VarBinding)
        HAMMER_VISIT_CASE(BindingList)
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
        HAMMER_VISIT_CASE(InterpolatedStringExpr)
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
        HAMMER_VISIT_CASE(TupleMemberExpr)
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
        HAMMER_VISIT_CASE(VarList)

#undef HAMMER_VISIT_CASE
    }
    HAMMER_UNREACHABLE("Broken node type information.");
}

template<>
struct NodeTraits<Binding> {
    static constexpr bool is_abstract = true;

    static constexpr NodeType first_node_type = NodeType::FirstBinding;
    static constexpr NodeType last_node_type = NodeType::LastBinding;

    template<typename Visitor>
    static void traverse_children(Binding* node, Visitor&& visitor) {
        NodeTraits<Node>::traverse_children(node, visitor);

        visitor(node->init());
    }

    template<typename Transform>
    static void transform_children(Binding* node, Transform&& transform) {
        NodeTraits<Node>::transform_children(node, transform);

        node->init(must_cast_nullable<Expr>(transform(node->init())));
    }
};

template<>
struct NodeTraits<TupleBinding> {
    static constexpr bool is_abstract = false;

    static constexpr NodeType node_type = NodeType::TupleBinding;

    template<typename Visitor>
    static void traverse_children(TupleBinding* node, Visitor&& visitor) {
        NodeTraits<Binding>::traverse_children(node, visitor);

        visitor(node->vars());
    }

    template<typename Transform>
    static void transform_children(TupleBinding* node, Transform&& transform) {
        NodeTraits<Binding>::transform_children(node, transform);

        node->vars(must_cast_nullable<VarList>(transform(node->vars())));
    }
};

template<>
struct NodeTraits<VarBinding> {
    static constexpr bool is_abstract = false;

    static constexpr NodeType node_type = NodeType::VarBinding;

    template<typename Visitor>
    static void traverse_children(VarBinding* node, Visitor&& visitor) {
        NodeTraits<Binding>::traverse_children(node, visitor);

        visitor(node->var());
    }

    template<typename Transform>
    static void transform_children(VarBinding* node, Transform&& transform) {
        NodeTraits<Binding>::transform_children(node, transform);

        node->var(must_cast_nullable<VarDecl>(transform(node->var())));
    }
};

template<>
struct NodeTraits<BindingList> {
    static constexpr bool is_abstract = false;

    static constexpr NodeType node_type = NodeType::BindingList;

    template<typename Visitor>
    static void traverse_children(BindingList* node, Visitor&& visitor) {
        NodeTraits<Node>::traverse_children(node, visitor);
        NodeListTraits<Binding>::traverse_items(node, visitor);
    }

    template<typename Transform>
    static void transform_children(BindingList* node, Transform&& transform) {
        NodeTraits<Node>::transform_children(node, transform);
        NodeListTraits<Binding>::transform_items(node, transform);
    }
};

template<>
struct NodeTraits<Decl> {
    static constexpr bool is_abstract = true;

    static constexpr NodeType first_node_type = NodeType::FirstDecl;
    static constexpr NodeType last_node_type = NodeType::LastDecl;

    template<typename Visitor>
    static void traverse_children(Decl* node, Visitor&& visitor) {
        NodeTraits<Node>::traverse_children(node, visitor);
    }

    template<typename Transform>
    static void transform_children(Decl* node, Transform&& transform) {
        NodeTraits<Node>::transform_children(node, transform);
    }
};

template<>
struct NodeTraits<FuncDecl> {
    static constexpr bool is_abstract = false;

    static constexpr NodeType node_type = NodeType::FuncDecl;

    template<typename Visitor>
    static void traverse_children(FuncDecl* node, Visitor&& visitor) {
        NodeTraits<Decl>::traverse_children(node, visitor);

        visitor(node->params());
        visitor(node->body());
    }

    template<typename Transform>
    static void transform_children(FuncDecl* node, Transform&& transform) {
        NodeTraits<Decl>::transform_children(node, transform);

        node->params(must_cast_nullable<ParamList>(transform(node->params())));
        node->body(must_cast_nullable<Expr>(transform(node->body())));
    }
};

template<>
struct NodeTraits<ImportDecl> {
    static constexpr bool is_abstract = false;

    static constexpr NodeType node_type = NodeType::ImportDecl;

    template<typename Visitor>
    static void traverse_children(ImportDecl* node, Visitor&& visitor) {
        NodeTraits<Decl>::traverse_children(node, visitor);
    }

    template<typename Transform>
    static void transform_children(ImportDecl* node, Transform&& transform) {
        NodeTraits<Decl>::transform_children(node, transform);
    }
};

template<>
struct NodeTraits<ParamDecl> {
    static constexpr bool is_abstract = false;

    static constexpr NodeType node_type = NodeType::ParamDecl;

    template<typename Visitor>
    static void traverse_children(ParamDecl* node, Visitor&& visitor) {
        NodeTraits<Decl>::traverse_children(node, visitor);
    }

    template<typename Transform>
    static void transform_children(ParamDecl* node, Transform&& transform) {
        NodeTraits<Decl>::transform_children(node, transform);
    }
};

template<>
struct NodeTraits<VarDecl> {
    static constexpr bool is_abstract = false;

    static constexpr NodeType node_type = NodeType::VarDecl;

    template<typename Visitor>
    static void traverse_children(VarDecl* node, Visitor&& visitor) {
        NodeTraits<Decl>::traverse_children(node, visitor);
    }

    template<typename Transform>
    static void transform_children(VarDecl* node, Transform&& transform) {
        NodeTraits<Decl>::transform_children(node, transform);
    }
};

template<>
struct NodeTraits<Expr> {
    static constexpr bool is_abstract = true;

    static constexpr NodeType first_node_type = NodeType::FirstExpr;
    static constexpr NodeType last_node_type = NodeType::LastExpr;

    template<typename Visitor>
    static void traverse_children(Expr* node, Visitor&& visitor) {
        NodeTraits<Node>::traverse_children(node, visitor);
    }

    template<typename Transform>
    static void transform_children(Expr* node, Transform&& transform) {
        NodeTraits<Node>::transform_children(node, transform);
    }
};

template<>
struct NodeTraits<BinaryExpr> {
    static constexpr bool is_abstract = false;

    static constexpr NodeType node_type = NodeType::BinaryExpr;

    template<typename Visitor>
    static void traverse_children(BinaryExpr* node, Visitor&& visitor) {
        NodeTraits<Expr>::traverse_children(node, visitor);

        visitor(node->left());
        visitor(node->right());
    }

    template<typename Transform>
    static void transform_children(BinaryExpr* node, Transform&& transform) {
        NodeTraits<Expr>::transform_children(node, transform);

        node->left(must_cast_nullable<Expr>(transform(node->left())));
        node->right(must_cast_nullable<Expr>(transform(node->right())));
    }
};

template<>
struct NodeTraits<BlockExpr> {
    static constexpr bool is_abstract = false;

    static constexpr NodeType node_type = NodeType::BlockExpr;

    template<typename Visitor>
    static void traverse_children(BlockExpr* node, Visitor&& visitor) {
        NodeTraits<Expr>::traverse_children(node, visitor);

        visitor(node->stmts());
    }

    template<typename Transform>
    static void transform_children(BlockExpr* node, Transform&& transform) {
        NodeTraits<Expr>::transform_children(node, transform);

        node->stmts(must_cast_nullable<StmtList>(transform(node->stmts())));
    }
};

template<>
struct NodeTraits<BreakExpr> {
    static constexpr bool is_abstract = false;

    static constexpr NodeType node_type = NodeType::BreakExpr;

    template<typename Visitor>
    static void traverse_children(BreakExpr* node, Visitor&& visitor) {
        NodeTraits<Expr>::traverse_children(node, visitor);
    }

    template<typename Transform>
    static void transform_children(BreakExpr* node, Transform&& transform) {
        NodeTraits<Expr>::transform_children(node, transform);
    }
};

template<>
struct NodeTraits<CallExpr> {
    static constexpr bool is_abstract = false;

    static constexpr NodeType node_type = NodeType::CallExpr;

    template<typename Visitor>
    static void traverse_children(CallExpr* node, Visitor&& visitor) {
        NodeTraits<Expr>::traverse_children(node, visitor);

        visitor(node->func());
        visitor(node->args());
    }

    template<typename Transform>
    static void transform_children(CallExpr* node, Transform&& transform) {
        NodeTraits<Expr>::transform_children(node, transform);

        node->func(must_cast_nullable<Expr>(transform(node->func())));
        node->args(must_cast_nullable<ExprList>(transform(node->args())));
    }
};

template<>
struct NodeTraits<ContinueExpr> {
    static constexpr bool is_abstract = false;

    static constexpr NodeType node_type = NodeType::ContinueExpr;

    template<typename Visitor>
    static void traverse_children(ContinueExpr* node, Visitor&& visitor) {
        NodeTraits<Expr>::traverse_children(node, visitor);
    }

    template<typename Transform>
    static void transform_children(ContinueExpr* node, Transform&& transform) {
        NodeTraits<Expr>::transform_children(node, transform);
    }
};

template<>
struct NodeTraits<DotExpr> {
    static constexpr bool is_abstract = false;

    static constexpr NodeType node_type = NodeType::DotExpr;

    template<typename Visitor>
    static void traverse_children(DotExpr* node, Visitor&& visitor) {
        NodeTraits<Expr>::traverse_children(node, visitor);

        visitor(node->inner());
    }

    template<typename Transform>
    static void transform_children(DotExpr* node, Transform&& transform) {
        NodeTraits<Expr>::transform_children(node, transform);

        node->inner(must_cast_nullable<Expr>(transform(node->inner())));
    }
};

template<>
struct NodeTraits<IfExpr> {
    static constexpr bool is_abstract = false;

    static constexpr NodeType node_type = NodeType::IfExpr;

    template<typename Visitor>
    static void traverse_children(IfExpr* node, Visitor&& visitor) {
        NodeTraits<Expr>::traverse_children(node, visitor);

        visitor(node->condition());
        visitor(node->then_branch());
        visitor(node->else_branch());
    }

    template<typename Transform>
    static void transform_children(IfExpr* node, Transform&& transform) {
        NodeTraits<Expr>::transform_children(node, transform);

        node->condition(must_cast_nullable<Expr>(transform(node->condition())));
        node->then_branch(
            must_cast_nullable<Expr>(transform(node->then_branch())));
        node->else_branch(
            must_cast_nullable<Expr>(transform(node->else_branch())));
    }
};

template<>
struct NodeTraits<IndexExpr> {
    static constexpr bool is_abstract = false;

    static constexpr NodeType node_type = NodeType::IndexExpr;

    template<typename Visitor>
    static void traverse_children(IndexExpr* node, Visitor&& visitor) {
        NodeTraits<Expr>::traverse_children(node, visitor);

        visitor(node->inner());
        visitor(node->index());
    }

    template<typename Transform>
    static void transform_children(IndexExpr* node, Transform&& transform) {
        NodeTraits<Expr>::transform_children(node, transform);

        node->inner(must_cast_nullable<Expr>(transform(node->inner())));
        node->index(must_cast_nullable<Expr>(transform(node->index())));
    }
};

template<>
struct NodeTraits<InterpolatedStringExpr> {
    static constexpr bool is_abstract = false;

    static constexpr NodeType node_type = NodeType::InterpolatedStringExpr;

    template<typename Visitor>
    static void
    traverse_children(InterpolatedStringExpr* node, Visitor&& visitor) {
        NodeTraits<Expr>::traverse_children(node, visitor);

        visitor(node->items());
    }

    template<typename Transform>
    static void
    transform_children(InterpolatedStringExpr* node, Transform&& transform) {
        NodeTraits<Expr>::transform_children(node, transform);

        node->items(must_cast_nullable<ExprList>(transform(node->items())));
    }
};

template<>
struct NodeTraits<Literal> {
    static constexpr bool is_abstract = true;

    static constexpr NodeType first_node_type = NodeType::FirstLiteral;
    static constexpr NodeType last_node_type = NodeType::LastLiteral;

    template<typename Visitor>
    static void traverse_children(Literal* node, Visitor&& visitor) {
        NodeTraits<Expr>::traverse_children(node, visitor);
    }

    template<typename Transform>
    static void transform_children(Literal* node, Transform&& transform) {
        NodeTraits<Expr>::transform_children(node, transform);
    }
};

template<>
struct NodeTraits<ArrayLiteral> {
    static constexpr bool is_abstract = false;

    static constexpr NodeType node_type = NodeType::ArrayLiteral;

    template<typename Visitor>
    static void traverse_children(ArrayLiteral* node, Visitor&& visitor) {
        NodeTraits<Literal>::traverse_children(node, visitor);

        visitor(node->entries());
    }

    template<typename Transform>
    static void transform_children(ArrayLiteral* node, Transform&& transform) {
        NodeTraits<Literal>::transform_children(node, transform);

        node->entries(must_cast_nullable<ExprList>(transform(node->entries())));
    }
};

template<>
struct NodeTraits<BooleanLiteral> {
    static constexpr bool is_abstract = false;

    static constexpr NodeType node_type = NodeType::BooleanLiteral;

    template<typename Visitor>
    static void traverse_children(BooleanLiteral* node, Visitor&& visitor) {
        NodeTraits<Literal>::traverse_children(node, visitor);
    }

    template<typename Transform>
    static void
    transform_children(BooleanLiteral* node, Transform&& transform) {
        NodeTraits<Literal>::transform_children(node, transform);
    }
};

template<>
struct NodeTraits<FloatLiteral> {
    static constexpr bool is_abstract = false;

    static constexpr NodeType node_type = NodeType::FloatLiteral;

    template<typename Visitor>
    static void traverse_children(FloatLiteral* node, Visitor&& visitor) {
        NodeTraits<Literal>::traverse_children(node, visitor);
    }

    template<typename Transform>
    static void transform_children(FloatLiteral* node, Transform&& transform) {
        NodeTraits<Literal>::transform_children(node, transform);
    }
};

template<>
struct NodeTraits<FuncLiteral> {
    static constexpr bool is_abstract = false;

    static constexpr NodeType node_type = NodeType::FuncLiteral;

    template<typename Visitor>
    static void traverse_children(FuncLiteral* node, Visitor&& visitor) {
        NodeTraits<Literal>::traverse_children(node, visitor);

        visitor(node->func());
    }

    template<typename Transform>
    static void transform_children(FuncLiteral* node, Transform&& transform) {
        NodeTraits<Literal>::transform_children(node, transform);

        node->func(must_cast_nullable<FuncDecl>(transform(node->func())));
    }
};

template<>
struct NodeTraits<IntegerLiteral> {
    static constexpr bool is_abstract = false;

    static constexpr NodeType node_type = NodeType::IntegerLiteral;

    template<typename Visitor>
    static void traverse_children(IntegerLiteral* node, Visitor&& visitor) {
        NodeTraits<Literal>::traverse_children(node, visitor);
    }

    template<typename Transform>
    static void
    transform_children(IntegerLiteral* node, Transform&& transform) {
        NodeTraits<Literal>::transform_children(node, transform);
    }
};

template<>
struct NodeTraits<MapLiteral> {
    static constexpr bool is_abstract = false;

    static constexpr NodeType node_type = NodeType::MapLiteral;

    template<typename Visitor>
    static void traverse_children(MapLiteral* node, Visitor&& visitor) {
        NodeTraits<Literal>::traverse_children(node, visitor);

        visitor(node->entries());
    }

    template<typename Transform>
    static void transform_children(MapLiteral* node, Transform&& transform) {
        NodeTraits<Literal>::transform_children(node, transform);

        node->entries(
            must_cast_nullable<MapEntryList>(transform(node->entries())));
    }
};

template<>
struct NodeTraits<NullLiteral> {
    static constexpr bool is_abstract = false;

    static constexpr NodeType node_type = NodeType::NullLiteral;

    template<typename Visitor>
    static void traverse_children(NullLiteral* node, Visitor&& visitor) {
        NodeTraits<Literal>::traverse_children(node, visitor);
    }

    template<typename Transform>
    static void transform_children(NullLiteral* node, Transform&& transform) {
        NodeTraits<Literal>::transform_children(node, transform);
    }
};

template<>
struct NodeTraits<SetLiteral> {
    static constexpr bool is_abstract = false;

    static constexpr NodeType node_type = NodeType::SetLiteral;

    template<typename Visitor>
    static void traverse_children(SetLiteral* node, Visitor&& visitor) {
        NodeTraits<Literal>::traverse_children(node, visitor);

        visitor(node->entries());
    }

    template<typename Transform>
    static void transform_children(SetLiteral* node, Transform&& transform) {
        NodeTraits<Literal>::transform_children(node, transform);

        node->entries(must_cast_nullable<ExprList>(transform(node->entries())));
    }
};

template<>
struct NodeTraits<StringLiteral> {
    static constexpr bool is_abstract = false;

    static constexpr NodeType node_type = NodeType::StringLiteral;

    template<typename Visitor>
    static void traverse_children(StringLiteral* node, Visitor&& visitor) {
        NodeTraits<Literal>::traverse_children(node, visitor);
    }

    template<typename Transform>
    static void transform_children(StringLiteral* node, Transform&& transform) {
        NodeTraits<Literal>::transform_children(node, transform);
    }
};

template<>
struct NodeTraits<SymbolLiteral> {
    static constexpr bool is_abstract = false;

    static constexpr NodeType node_type = NodeType::SymbolLiteral;

    template<typename Visitor>
    static void traverse_children(SymbolLiteral* node, Visitor&& visitor) {
        NodeTraits<Literal>::traverse_children(node, visitor);
    }

    template<typename Transform>
    static void transform_children(SymbolLiteral* node, Transform&& transform) {
        NodeTraits<Literal>::transform_children(node, transform);
    }
};

template<>
struct NodeTraits<TupleLiteral> {
    static constexpr bool is_abstract = false;

    static constexpr NodeType node_type = NodeType::TupleLiteral;

    template<typename Visitor>
    static void traverse_children(TupleLiteral* node, Visitor&& visitor) {
        NodeTraits<Literal>::traverse_children(node, visitor);

        visitor(node->entries());
    }

    template<typename Transform>
    static void transform_children(TupleLiteral* node, Transform&& transform) {
        NodeTraits<Literal>::transform_children(node, transform);

        node->entries(must_cast_nullable<ExprList>(transform(node->entries())));
    }
};

template<>
struct NodeTraits<ReturnExpr> {
    static constexpr bool is_abstract = false;

    static constexpr NodeType node_type = NodeType::ReturnExpr;

    template<typename Visitor>
    static void traverse_children(ReturnExpr* node, Visitor&& visitor) {
        NodeTraits<Expr>::traverse_children(node, visitor);

        visitor(node->inner());
    }

    template<typename Transform>
    static void transform_children(ReturnExpr* node, Transform&& transform) {
        NodeTraits<Expr>::transform_children(node, transform);

        node->inner(must_cast_nullable<Expr>(transform(node->inner())));
    }
};

template<>
struct NodeTraits<StringSequenceExpr> {
    static constexpr bool is_abstract = false;

    static constexpr NodeType node_type = NodeType::StringSequenceExpr;

    template<typename Visitor>
    static void traverse_children(StringSequenceExpr* node, Visitor&& visitor) {
        NodeTraits<Expr>::traverse_children(node, visitor);

        visitor(node->strings());
    }

    template<typename Transform>
    static void
    transform_children(StringSequenceExpr* node, Transform&& transform) {
        NodeTraits<Expr>::transform_children(node, transform);

        node->strings(must_cast_nullable<ExprList>(transform(node->strings())));
    }
};

template<>
struct NodeTraits<TupleMemberExpr> {
    static constexpr bool is_abstract = false;

    static constexpr NodeType node_type = NodeType::TupleMemberExpr;

    template<typename Visitor>
    static void traverse_children(TupleMemberExpr* node, Visitor&& visitor) {
        NodeTraits<Expr>::traverse_children(node, visitor);

        visitor(node->inner());
    }

    template<typename Transform>
    static void
    transform_children(TupleMemberExpr* node, Transform&& transform) {
        NodeTraits<Expr>::transform_children(node, transform);

        node->inner(must_cast_nullable<Expr>(transform(node->inner())));
    }
};

template<>
struct NodeTraits<UnaryExpr> {
    static constexpr bool is_abstract = false;

    static constexpr NodeType node_type = NodeType::UnaryExpr;

    template<typename Visitor>
    static void traverse_children(UnaryExpr* node, Visitor&& visitor) {
        NodeTraits<Expr>::traverse_children(node, visitor);

        visitor(node->inner());
    }

    template<typename Transform>
    static void transform_children(UnaryExpr* node, Transform&& transform) {
        NodeTraits<Expr>::transform_children(node, transform);

        node->inner(must_cast_nullable<Expr>(transform(node->inner())));
    }
};

template<>
struct NodeTraits<VarExpr> {
    static constexpr bool is_abstract = false;

    static constexpr NodeType node_type = NodeType::VarExpr;

    template<typename Visitor>
    static void traverse_children(VarExpr* node, Visitor&& visitor) {
        NodeTraits<Expr>::traverse_children(node, visitor);
    }

    template<typename Transform>
    static void transform_children(VarExpr* node, Transform&& transform) {
        NodeTraits<Expr>::transform_children(node, transform);
    }
};

template<>
struct NodeTraits<ExprList> {
    static constexpr bool is_abstract = false;

    static constexpr NodeType node_type = NodeType::ExprList;

    template<typename Visitor>
    static void traverse_children(ExprList* node, Visitor&& visitor) {
        NodeTraits<Node>::traverse_children(node, visitor);
        NodeListTraits<Expr>::traverse_items(node, visitor);
    }

    template<typename Transform>
    static void transform_children(ExprList* node, Transform&& transform) {
        NodeTraits<Node>::transform_children(node, transform);
        NodeListTraits<Expr>::transform_items(node, transform);
    }
};

template<>
struct NodeTraits<File> {
    static constexpr bool is_abstract = false;

    static constexpr NodeType node_type = NodeType::File;

    template<typename Visitor>
    static void traverse_children(File* node, Visitor&& visitor) {
        NodeTraits<Node>::traverse_children(node, visitor);

        visitor(node->items());
    }

    template<typename Transform>
    static void transform_children(File* node, Transform&& transform) {
        NodeTraits<Node>::transform_children(node, transform);

        node->items(must_cast_nullable<NodeList>(transform(node->items())));
    }
};

template<>
struct NodeTraits<MapEntry> {
    static constexpr bool is_abstract = false;

    static constexpr NodeType node_type = NodeType::MapEntry;

    template<typename Visitor>
    static void traverse_children(MapEntry* node, Visitor&& visitor) {
        NodeTraits<Node>::traverse_children(node, visitor);

        visitor(node->key());
        visitor(node->value());
    }

    template<typename Transform>
    static void transform_children(MapEntry* node, Transform&& transform) {
        NodeTraits<Node>::transform_children(node, transform);

        node->key(must_cast_nullable<Expr>(transform(node->key())));
        node->value(must_cast_nullable<Expr>(transform(node->value())));
    }
};

template<>
struct NodeTraits<MapEntryList> {
    static constexpr bool is_abstract = false;

    static constexpr NodeType node_type = NodeType::MapEntryList;

    template<typename Visitor>
    static void traverse_children(MapEntryList* node, Visitor&& visitor) {
        NodeTraits<Node>::traverse_children(node, visitor);
        NodeListTraits<MapEntry>::traverse_items(node, visitor);
    }

    template<typename Transform>
    static void transform_children(MapEntryList* node, Transform&& transform) {
        NodeTraits<Node>::transform_children(node, transform);
        NodeListTraits<MapEntry>::transform_items(node, transform);
    }
};

template<>
struct NodeTraits<NodeList> {
    static constexpr bool is_abstract = false;

    static constexpr NodeType node_type = NodeType::NodeList;

    template<typename Visitor>
    static void traverse_children(NodeList* node, Visitor&& visitor) {
        NodeTraits<Node>::traverse_children(node, visitor);
        NodeListTraits<Node>::traverse_items(node, visitor);
    }

    template<typename Transform>
    static void transform_children(NodeList* node, Transform&& transform) {
        NodeTraits<Node>::transform_children(node, transform);
        NodeListTraits<Node>::transform_items(node, transform);
    }
};

template<>
struct NodeTraits<ParamList> {
    static constexpr bool is_abstract = false;

    static constexpr NodeType node_type = NodeType::ParamList;

    template<typename Visitor>
    static void traverse_children(ParamList* node, Visitor&& visitor) {
        NodeTraits<Node>::traverse_children(node, visitor);
        NodeListTraits<ParamDecl>::traverse_items(node, visitor);
    }

    template<typename Transform>
    static void transform_children(ParamList* node, Transform&& transform) {
        NodeTraits<Node>::transform_children(node, transform);
        NodeListTraits<ParamDecl>::transform_items(node, transform);
    }
};

template<>
struct NodeTraits<Root> {
    static constexpr bool is_abstract = false;

    static constexpr NodeType node_type = NodeType::Root;

    template<typename Visitor>
    static void traverse_children(Root* node, Visitor&& visitor) {
        NodeTraits<Node>::traverse_children(node, visitor);

        visitor(node->file());
    }

    template<typename Transform>
    static void transform_children(Root* node, Transform&& transform) {
        NodeTraits<Node>::transform_children(node, transform);

        node->file(must_cast_nullable<File>(transform(node->file())));
    }
};

template<>
struct NodeTraits<Stmt> {
    static constexpr bool is_abstract = true;

    static constexpr NodeType first_node_type = NodeType::FirstStmt;
    static constexpr NodeType last_node_type = NodeType::LastStmt;

    template<typename Visitor>
    static void traverse_children(Stmt* node, Visitor&& visitor) {
        NodeTraits<Node>::traverse_children(node, visitor);
    }

    template<typename Transform>
    static void transform_children(Stmt* node, Transform&& transform) {
        NodeTraits<Node>::transform_children(node, transform);
    }
};

template<>
struct NodeTraits<AssertStmt> {
    static constexpr bool is_abstract = false;

    static constexpr NodeType node_type = NodeType::AssertStmt;

    template<typename Visitor>
    static void traverse_children(AssertStmt* node, Visitor&& visitor) {
        NodeTraits<Stmt>::traverse_children(node, visitor);

        visitor(node->condition());
        visitor(node->message());
    }

    template<typename Transform>
    static void transform_children(AssertStmt* node, Transform&& transform) {
        NodeTraits<Stmt>::transform_children(node, transform);

        node->condition(must_cast_nullable<Expr>(transform(node->condition())));
        node->message(
            must_cast_nullable<StringLiteral>(transform(node->message())));
    }
};

template<>
struct NodeTraits<DeclStmt> {
    static constexpr bool is_abstract = false;

    static constexpr NodeType node_type = NodeType::DeclStmt;

    template<typename Visitor>
    static void traverse_children(DeclStmt* node, Visitor&& visitor) {
        NodeTraits<Stmt>::traverse_children(node, visitor);

        visitor(node->bindings());
    }

    template<typename Transform>
    static void transform_children(DeclStmt* node, Transform&& transform) {
        NodeTraits<Stmt>::transform_children(node, transform);

        node->bindings(
            must_cast_nullable<BindingList>(transform(node->bindings())));
    }
};

template<>
struct NodeTraits<EmptyStmt> {
    static constexpr bool is_abstract = false;

    static constexpr NodeType node_type = NodeType::EmptyStmt;

    template<typename Visitor>
    static void traverse_children(EmptyStmt* node, Visitor&& visitor) {
        NodeTraits<Stmt>::traverse_children(node, visitor);
    }

    template<typename Transform>
    static void transform_children(EmptyStmt* node, Transform&& transform) {
        NodeTraits<Stmt>::transform_children(node, transform);
    }
};

template<>
struct NodeTraits<ExprStmt> {
    static constexpr bool is_abstract = false;

    static constexpr NodeType node_type = NodeType::ExprStmt;

    template<typename Visitor>
    static void traverse_children(ExprStmt* node, Visitor&& visitor) {
        NodeTraits<Stmt>::traverse_children(node, visitor);

        visitor(node->expr());
    }

    template<typename Transform>
    static void transform_children(ExprStmt* node, Transform&& transform) {
        NodeTraits<Stmt>::transform_children(node, transform);

        node->expr(must_cast_nullable<Expr>(transform(node->expr())));
    }
};

template<>
struct NodeTraits<ForStmt> {
    static constexpr bool is_abstract = false;

    static constexpr NodeType node_type = NodeType::ForStmt;

    template<typename Visitor>
    static void traverse_children(ForStmt* node, Visitor&& visitor) {
        NodeTraits<Stmt>::traverse_children(node, visitor);

        visitor(node->decl());
        visitor(node->condition());
        visitor(node->step());
        visitor(node->body());
    }

    template<typename Transform>
    static void transform_children(ForStmt* node, Transform&& transform) {
        NodeTraits<Stmt>::transform_children(node, transform);

        node->decl(must_cast_nullable<DeclStmt>(transform(node->decl())));
        node->condition(must_cast_nullable<Expr>(transform(node->condition())));
        node->step(must_cast_nullable<Expr>(transform(node->step())));
        node->body(must_cast_nullable<Expr>(transform(node->body())));
    }
};

template<>
struct NodeTraits<WhileStmt> {
    static constexpr bool is_abstract = false;

    static constexpr NodeType node_type = NodeType::WhileStmt;

    template<typename Visitor>
    static void traverse_children(WhileStmt* node, Visitor&& visitor) {
        NodeTraits<Stmt>::traverse_children(node, visitor);

        visitor(node->condition());
        visitor(node->body());
    }

    template<typename Transform>
    static void transform_children(WhileStmt* node, Transform&& transform) {
        NodeTraits<Stmt>::transform_children(node, transform);

        node->condition(must_cast_nullable<Expr>(transform(node->condition())));
        node->body(must_cast_nullable<BlockExpr>(transform(node->body())));
    }
};

template<>
struct NodeTraits<StmtList> {
    static constexpr bool is_abstract = false;

    static constexpr NodeType node_type = NodeType::StmtList;

    template<typename Visitor>
    static void traverse_children(StmtList* node, Visitor&& visitor) {
        NodeTraits<Node>::traverse_children(node, visitor);
        NodeListTraits<Stmt>::traverse_items(node, visitor);
    }

    template<typename Transform>
    static void transform_children(StmtList* node, Transform&& transform) {
        NodeTraits<Node>::transform_children(node, transform);
        NodeListTraits<Stmt>::transform_items(node, transform);
    }
};

template<>
struct NodeTraits<VarList> {
    static constexpr bool is_abstract = false;

    static constexpr NodeType node_type = NodeType::VarList;

    template<typename Visitor>
    static void traverse_children(VarList* node, Visitor&& visitor) {
        NodeTraits<Node>::traverse_children(node, visitor);
        NodeListTraits<VarDecl>::traverse_items(node, visitor);
    }

    template<typename Transform>
    static void transform_children(VarList* node, Transform&& transform) {
        NodeTraits<Node>::transform_children(node, transform);
        NodeListTraits<VarDecl>::transform_items(node, transform);
    }
};

} // namespace hammer::compiler
