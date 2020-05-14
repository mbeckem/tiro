#ifndef TIRO_AST_EXPR_HPP
#define TIRO_AST_EXPR_HPP

#include "tiro/ast/node.hpp"

namespace tiro {

/* [[[cog
    from codegen.ast import NODE_TYPES, define, walk_types
    
    node_types = list(walk_types(NODE_TYPES.get("Expr")))
    define(*node_types)
]]] */
/// Represents a single expression.
class AstExpr : public AstNode {
protected:
    AstExpr(AstNodeType type);

public:
    ~AstExpr();
};

/// Represents a binary expression.
class AstBinaryExpr final : public AstExpr {
public:
    AstBinaryExpr();

    ~AstBinaryExpr();

    const BinaryOperator& operation() const { return operation_; }
    void operation(BinaryOperator new_operation) {
        operation_ = std::move(new_operation);
    }

    const AstPtr<AstExpr>& left() const { return left_; }
    void left(AstPtr<AstExpr> new_left) { left_ = std::move(new_left); }

    const AstPtr<AstExpr>& right() const { return right_; }
    void right(AstPtr<AstExpr> new_right) { right_ = std::move(new_right); }

private:
    BinaryOperator operation_;
    AstPtr<AstExpr> left_;
    AstPtr<AstExpr> right_;
};

/// Represents a block expression containing multiple statements.
class AstBlockExpr final : public AstExpr {
public:
    AstBlockExpr();

    ~AstBlockExpr();

    const AstNodeList<AstStmt>& stmts() const { return stmts_; }
    void stmts(AstNodeList<AstStmt> new_stmts) {
        stmts_ = std::move(new_stmts);
    }

private:
    AstNodeList<AstStmt> stmts_;
};

/// Represents a break expression within a loop.
class AstBreakExpr final : public AstExpr {
public:
    AstBreakExpr();

    ~AstBreakExpr();
};

/// Represents a function call expression.
class AstCallExpr final : public AstExpr {
public:
    AstCallExpr();

    ~AstCallExpr();

    const AccessType& access_type() const { return access_type_; }
    void access_type(AccessType new_access_type) {
        access_type_ = std::move(new_access_type);
    }

    const AstPtr<AstExpr>& func() const { return func_; }
    void func(AstPtr<AstExpr> new_func) { func_ = std::move(new_func); }

    const AstNodeList<AstExpr>& args() const { return args_; }
    void args(AstNodeList<AstExpr> new_args) { args_ = std::move(new_args); }

private:
    AccessType access_type_;
    AstPtr<AstExpr> func_;
    AstNodeList<AstExpr> args_;
};

/// Represents a continue expression within a loop.
class AstContinueExpr final : public AstExpr {
public:
    AstContinueExpr();

    ~AstContinueExpr();
};

/// Represents an access to a container element.
class AstElementExpr final : public AstExpr {
public:
    AstElementExpr();

    ~AstElementExpr();

    const AccessType& access_type() const { return access_type_; }
    void access_type(AccessType new_access_type) {
        access_type_ = std::move(new_access_type);
    }

    const AstPtr<AstExpr>& instance() const { return instance_; }
    void instance(AstPtr<AstExpr> new_instance) {
        instance_ = std::move(new_instance);
    }

    const AstPtr<AstExpr>& element() const { return element_; }
    void element(AstPtr<AstExpr> new_element) {
        element_ = std::move(new_element);
    }

private:
    AccessType access_type_;
    AstPtr<AstExpr> instance_;
    AstPtr<AstExpr> element_;
};

/// Represents a function expression.
class AstFuncExpr final : public AstExpr {
public:
    AstFuncExpr();

    ~AstFuncExpr();

    const AstPtr<AstFuncDecl>& decl() const { return decl_; }
    void decl(AstPtr<AstFuncDecl> new_decl) { decl_ = std::move(new_decl); }

private:
    AstPtr<AstFuncDecl> decl_;
};

/// Represents an if expression.
class AstIfExpr final : public AstExpr {
public:
    AstIfExpr();

    ~AstIfExpr();

    const AstPtr<AstExpr>& cond() const { return cond_; }
    void cond(AstPtr<AstExpr> new_cond) { cond_ = std::move(new_cond); }

    const AstPtr<AstExpr>& then_branch() const { return then_branch_; }
    void then_branch(AstPtr<AstExpr> new_then_branch) {
        then_branch_ = std::move(new_then_branch);
    }

    const AstPtr<AstExpr>& else_branch() const { return else_branch_; }
    void else_branch(AstPtr<AstExpr> new_else_branch) {
        else_branch_ = std::move(new_else_branch);
    }

private:
    AstPtr<AstExpr> cond_;
    AstPtr<AstExpr> then_branch_;
    AstPtr<AstExpr> else_branch_;
};

/// Represents a literal value.
class AstLiteral : public AstExpr {
protected:
    AstLiteral(AstNodeType type);

public:
    ~AstLiteral();
};

/// Represents an array expression.
class AstArrayLiteral final : public AstLiteral {
public:
    AstArrayLiteral();

    ~AstArrayLiteral();

    const AstNodeList<AstExpr>& items() const { return items_; }
    void items(AstNodeList<AstExpr> new_items) {
        items_ = std::move(new_items);
    }

private:
    AstNodeList<AstExpr> items_;
};

/// Represents a boolean literal.
class AstBooleanLiteral final : public AstLiteral {
public:
    AstBooleanLiteral();

    ~AstBooleanLiteral();

    const bool& value() const { return value_; }
    void value(bool new_value) { value_ = std::move(new_value); }

private:
    bool value_;
};

/// Represents a floating point literal.
class AstFloatLiteral final : public AstLiteral {
public:
    AstFloatLiteral();

    ~AstFloatLiteral();

    const f64& value() const { return value_; }
    void value(f64 new_value) { value_ = std::move(new_value); }

private:
    f64 value_;
};

/// Represents an integer literal.
class AstIntegerLiteral final : public AstLiteral {
public:
    AstIntegerLiteral();

    ~AstIntegerLiteral();

    const i64& value() const { return value_; }
    void value(i64 new_value) { value_ = std::move(new_value); }

private:
    i64 value_;
};

/// Represents a map expression.
class AstMapLiteral final : public AstLiteral {
public:
    AstMapLiteral();

    ~AstMapLiteral();

    const AstNodeList<AstMapItem>& items() const { return items_; }
    void items(AstNodeList<AstMapItem> new_items) {
        items_ = std::move(new_items);
    }

private:
    AstNodeList<AstMapItem> items_;
};

/// Represents a null literal.
class AstNullLiteral final : public AstLiteral {
public:
    AstNullLiteral();

    ~AstNullLiteral();
};

/// Represents a set expression.
class AstSetLiteral final : public AstLiteral {
public:
    AstSetLiteral();

    ~AstSetLiteral();

    const AstNodeList<AstExpr>& items() const { return items_; }
    void items(AstNodeList<AstExpr> new_items) {
        items_ = std::move(new_items);
    }

private:
    AstNodeList<AstExpr> items_;
};

/// Represents a string literal.
class AstStringLiteral final : public AstLiteral {
public:
    AstStringLiteral();

    ~AstStringLiteral();

    const InternedString& value() const { return value_; }
    void value(InternedString new_value) { value_ = std::move(new_value); }

private:
    InternedString value_;
};

/// Represents a symbol.
class AstSymbolLiteral final : public AstLiteral {
public:
    AstSymbolLiteral();

    ~AstSymbolLiteral();

    const InternedString& value() const { return value_; }
    void value(InternedString new_value) { value_ = std::move(new_value); }

private:
    InternedString value_;
};

/// Represents a tuple expression.
class AstTupleLiteral final : public AstLiteral {
public:
    AstTupleLiteral();

    ~AstTupleLiteral();

    const AstNodeList<AstExpr>& items() const { return items_; }
    void items(AstNodeList<AstExpr> new_items) {
        items_ = std::move(new_items);
    }

private:
    AstNodeList<AstExpr> items_;
};

/// Represents an access to an object property.
class AstPropertyExpr final : public AstExpr {
public:
    AstPropertyExpr();

    ~AstPropertyExpr();

    const AccessType& access_type() const { return access_type_; }
    void access_type(AccessType new_access_type) {
        access_type_ = std::move(new_access_type);
    }

    const AstPtr<AstExpr>& instance() const { return instance_; }
    void instance(AstPtr<AstExpr> new_instance) {
        instance_ = std::move(new_instance);
    }

    const AstProperty& property() const { return property_; }
    void property(AstProperty new_property) {
        property_ = std::move(new_property);
    }

private:
    AccessType access_type_;
    AstPtr<AstExpr> instance_;
    AstProperty property_;
};

/// Represents a return expression with an optional return value.
class AstReturnExpr final : public AstExpr {
public:
    AstReturnExpr();

    ~AstReturnExpr();

    const AstPtr<AstExpr>& value() const { return value_; }
    void value(AstPtr<AstExpr> new_value) { value_ = std::move(new_value); }

private:
    AstPtr<AstExpr> value_;
};

/// Represents a string expression consisting of literal strings and formatted sub expressions.
class AstStringExpr final : public AstExpr {
public:
    AstStringExpr();

    ~AstStringExpr();

    const AstNodeList<AstExpr>& items() const { return items_; }
    void items(AstNodeList<AstExpr> new_items) {
        items_ = std::move(new_items);
    }

private:
    AstNodeList<AstExpr> items_;
};

/// Represents a unary expression.
class AstUnaryExpr final : public AstExpr {
public:
    AstUnaryExpr();

    ~AstUnaryExpr();

    const UnaryOperator& operation() const { return operation_; }
    void operation(UnaryOperator new_operation) {
        operation_ = std::move(new_operation);
    }

    const AstPtr<AstExpr>& inner() const { return inner_; }
    void inner(AstPtr<AstExpr> new_inner) { inner_ = std::move(new_inner); }

private:
    UnaryOperator operation_;
    AstPtr<AstExpr> inner_;
};

/// Represents a reference to a variable.
class AstVarExpr final : public AstExpr {
public:
    AstVarExpr();

    ~AstVarExpr();

    const InternedString& name() const { return name_; }
    void name(InternedString new_name) { name_ = std::move(new_name); }

private:
    InternedString name_;
};
// [[[end]]]

} // namespace tiro

#endif // TIRO_AST_EXPR_HPP
