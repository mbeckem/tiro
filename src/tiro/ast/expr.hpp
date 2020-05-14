#ifndef TIRO_AST_EXPR_HPP
#define TIRO_AST_EXPR_HPP

#include "tiro/ast/node.hpp"

namespace tiro {

/* [[[cog
    from codegen.ast import NODE_TYPES, define, walk_types
    
    node_types = list(walk_types(NODE_TYPES.get("Expr")))
    define(*node_types, NODE_TYPES.get("MapItem"))
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

    BinaryOperator operation() const;
    void operation(BinaryOperator new_operation);

    AstExpr* left() const;
    void left(AstPtr<AstExpr> new_left);

    AstExpr* right() const;
    void right(AstPtr<AstExpr> new_right);

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

    const AstNodeList<AstStmt>& stmts() const;
    void stmts(AstNodeList<AstStmt> new_stmts);

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

    AccessType access_type() const;
    void access_type(AccessType new_access_type);

    AstExpr* func() const;
    void func(AstPtr<AstExpr> new_func);

    const AstNodeList<AstExpr>& args() const;
    void args(AstNodeList<AstExpr> new_args);

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

    AccessType access_type() const;
    void access_type(AccessType new_access_type);

    AstExpr* instance() const;
    void instance(AstPtr<AstExpr> new_instance);

    AstExpr* element() const;
    void element(AstPtr<AstExpr> new_element);

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

    AstFuncDecl* decl() const;
    void decl(AstPtr<AstFuncDecl> new_decl);

private:
    AstPtr<AstFuncDecl> decl_;
};

/// Represents an if expression.
class AstIfExpr final : public AstExpr {
public:
    AstIfExpr();

    ~AstIfExpr();

    AstExpr* cond() const;
    void cond(AstPtr<AstExpr> new_cond);

    AstExpr* then_branch() const;
    void then_branch(AstPtr<AstExpr> new_then_branch);

    AstExpr* else_branch() const;
    void else_branch(AstPtr<AstExpr> new_else_branch);

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

    const AstNodeList<AstExpr>& items() const;
    void items(AstNodeList<AstExpr> new_items);

private:
    AstNodeList<AstExpr> items_;
};

/// Represents a boolean literal.
class AstBooleanLiteral final : public AstLiteral {
public:
    AstBooleanLiteral();

    ~AstBooleanLiteral();

    bool value() const;
    void value(bool new_value);

private:
    bool value_;
};

/// Represents a floating point literal.
class AstFloatLiteral final : public AstLiteral {
public:
    AstFloatLiteral();

    ~AstFloatLiteral();

    f64 value() const;
    void value(f64 new_value);

private:
    f64 value_;
};

/// Represents an integer literal.
class AstIntegerLiteral final : public AstLiteral {
public:
    AstIntegerLiteral();

    ~AstIntegerLiteral();

    i64 value() const;
    void value(i64 new_value);

private:
    i64 value_;
};

/// Represents a map expression.
class AstMapLiteral final : public AstLiteral {
public:
    AstMapLiteral();

    ~AstMapLiteral();

    const AstNodeList<AstMapItem>& items() const;
    void items(AstNodeList<AstMapItem> new_items);

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

    const AstNodeList<AstExpr>& items() const;
    void items(AstNodeList<AstExpr> new_items);

private:
    AstNodeList<AstExpr> items_;
};

/// Represents a string literal.
class AstStringLiteral final : public AstLiteral {
public:
    AstStringLiteral();

    ~AstStringLiteral();

    InternedString value() const;
    void value(InternedString new_value);

private:
    InternedString value_;
};

/// Represents a symbol.
class AstSymbolLiteral final : public AstLiteral {
public:
    AstSymbolLiteral();

    ~AstSymbolLiteral();

    InternedString value() const;
    void value(InternedString new_value);

private:
    InternedString value_;
};

/// Represents a tuple expression.
class AstTupleLiteral final : public AstLiteral {
public:
    AstTupleLiteral();

    ~AstTupleLiteral();

    const AstNodeList<AstExpr>& items() const;
    void items(AstNodeList<AstExpr> new_items);

private:
    AstNodeList<AstExpr> items_;
};

/// Represents an access to an object property.
class AstPropertyExpr final : public AstExpr {
public:
    AstPropertyExpr();

    ~AstPropertyExpr();

    AccessType access_type() const;
    void access_type(AccessType new_access_type);

    AstExpr* instance() const;
    void instance(AstPtr<AstExpr> new_instance);

    const AstProperty& property() const;
    void property(AstProperty new_property);

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

    AstExpr* value() const;
    void value(AstPtr<AstExpr> new_value);

private:
    AstPtr<AstExpr> value_;
};

/// Represents a string expression consisting of literal strings and formatted sub expressions.
class AstStringExpr final : public AstExpr {
public:
    AstStringExpr();

    ~AstStringExpr();

    const AstNodeList<AstExpr>& items() const;
    void items(AstNodeList<AstExpr> new_items);

private:
    AstNodeList<AstExpr> items_;
};

/// Represents a unary expression.
class AstUnaryExpr final : public AstExpr {
public:
    AstUnaryExpr();

    ~AstUnaryExpr();

    UnaryOperator operation() const;
    void operation(UnaryOperator new_operation);

    AstExpr* inner() const;
    void inner(AstPtr<AstExpr> new_inner);

private:
    UnaryOperator operation_;
    AstPtr<AstExpr> inner_;
};

/// Represents a reference to a variable.
class AstVarExpr final : public AstExpr {
public:
    AstVarExpr();

    ~AstVarExpr();

    InternedString name() const;
    void name(InternedString new_name);

private:
    InternedString name_;
};

/// Represents a key-value pair in a map expression.
class AstMapItem final : public AstNode {
public:
    AstMapItem();

    ~AstMapItem();

    AstExpr* key() const;
    void key(AstPtr<AstExpr> new_key);

    AstExpr* value() const;
    void value(AstPtr<AstExpr> new_value);

private:
    AstPtr<AstExpr> key_;
    AstPtr<AstExpr> value_;
};
// [[[end]]]

} // namespace tiro

#endif // TIRO_AST_EXPR_HPP
