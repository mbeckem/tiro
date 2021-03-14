#ifndef TIRO_COMPILER_AST_EXPR_HPP
#define TIRO_COMPILER_AST_EXPR_HPP

#include "common/format.hpp"
#include "compiler/ast/node.hpp"

namespace tiro {

/// The operator used in a unary operation.
enum class UnaryOperator : u8 {
    // Arithmetic
    Plus,
    Minus,

    // Binary
    BitwiseNot,

    // Boolean
    LogicalNot
};

std::string_view to_string(UnaryOperator op);

/// The operator used in a binary operation.
///
/// NOTE: currently all binary operations (including assignment)
/// share the same class. It could be useful in future to separate
/// the classes into more subclasses (e.g. AssignmentExpr).
enum class BinaryOperator : u8 {
    // Arithmetic
    Plus,
    Minus,
    Multiply,
    Divide,
    Modulus,
    Power,

    // Binary
    LeftShift,
    RightShift,
    BitwiseAnd,
    BitwiseOr,
    BitwiseXor,

    // Boolean
    Less,
    LessEquals,
    Greater,
    GreaterEquals,
    Equals,
    NotEquals,
    LogicalAnd,
    LogicalOr,

    NullCoalesce,

    // Assigments
    Assign,
    AssignPlus,
    AssignMinus,
    AssignMultiply,
    AssignDivide,
    AssignModulus,
    AssignPower
};

std::string_view to_string(BinaryOperator op);

/* [[[cog
    from codegen.ast import NODE_TYPES, define, walk_types

    roots = [NODE_TYPES.get(root) for root in ["Expr", "Identifier", "MapItem", "RecordItem"]]
    node_types = list(walk_types(*roots))
    define(*node_types)
]]] */
/// Represents a single expression.
class AstExpr : public AstNode {
protected:
    AstExpr(AstNodeType type);

public:
    ~AstExpr();

protected:
    void do_traverse_children(FunctionRef<void(AstNode*)> callback) const override;
    void do_mutate_children(MutableAstVisitor& visitor) override;
};

/// Represents a binary expression.
class AstBinaryExpr final : public AstExpr {
public:
    AstBinaryExpr(BinaryOperator operation);

    ~AstBinaryExpr();

    BinaryOperator operation() const;
    void operation(BinaryOperator new_operation);

    AstExpr* left() const;
    void left(AstPtr<AstExpr> new_left);

    AstExpr* right() const;
    void right(AstPtr<AstExpr> new_right);

protected:
    void do_traverse_children(FunctionRef<void(AstNode*)> callback) const override;
    void do_mutate_children(MutableAstVisitor& visitor) override;

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

    AstNodeList<AstStmt>& stmts();
    const AstNodeList<AstStmt>& stmts() const;
    void stmts(AstNodeList<AstStmt> new_stmts);

protected:
    void do_traverse_children(FunctionRef<void(AstNode*)> callback) const override;
    void do_mutate_children(MutableAstVisitor& visitor) override;

private:
    AstNodeList<AstStmt> stmts_;
};

/// Represents a break expression within a loop.
class AstBreakExpr final : public AstExpr {
public:
    AstBreakExpr();

    ~AstBreakExpr();

protected:
    void do_traverse_children(FunctionRef<void(AstNode*)> callback) const override;
    void do_mutate_children(MutableAstVisitor& visitor) override;
};

/// Represents a function call expression.
class AstCallExpr final : public AstExpr {
public:
    AstCallExpr(AccessType access_type);

    ~AstCallExpr();

    AccessType access_type() const;
    void access_type(AccessType new_access_type);

    AstExpr* func() const;
    void func(AstPtr<AstExpr> new_func);

    AstNodeList<AstExpr>& args();
    const AstNodeList<AstExpr>& args() const;
    void args(AstNodeList<AstExpr> new_args);

protected:
    void do_traverse_children(FunctionRef<void(AstNode*)> callback) const override;
    void do_mutate_children(MutableAstVisitor& visitor) override;

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

protected:
    void do_traverse_children(FunctionRef<void(AstNode*)> callback) const override;
    void do_mutate_children(MutableAstVisitor& visitor) override;
};

/// Represents an access to a container element.
class AstElementExpr final : public AstExpr {
public:
    AstElementExpr(AccessType access_type);

    ~AstElementExpr();

    AccessType access_type() const;
    void access_type(AccessType new_access_type);

    AstExpr* instance() const;
    void instance(AstPtr<AstExpr> new_instance);

    AstExpr* element() const;
    void element(AstPtr<AstExpr> new_element);

protected:
    void do_traverse_children(FunctionRef<void(AstNode*)> callback) const override;
    void do_mutate_children(MutableAstVisitor& visitor) override;

private:
    AccessType access_type_;
    AstPtr<AstExpr> instance_;
    AstPtr<AstExpr> element_;
};

/// Represents an error at expression level.
class AstErrorExpr final : public AstExpr {
public:
    AstErrorExpr();

    ~AstErrorExpr();

protected:
    void do_traverse_children(FunctionRef<void(AstNode*)> callback) const override;
    void do_mutate_children(MutableAstVisitor& visitor) override;
};

/// Represents a function expression.
class AstFuncExpr final : public AstExpr {
public:
    AstFuncExpr();

    ~AstFuncExpr();

    AstFuncDecl* decl() const;
    void decl(AstPtr<AstFuncDecl> new_decl);

protected:
    void do_traverse_children(FunctionRef<void(AstNode*)> callback) const override;
    void do_mutate_children(MutableAstVisitor& visitor) override;

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

protected:
    void do_traverse_children(FunctionRef<void(AstNode*)> callback) const override;
    void do_mutate_children(MutableAstVisitor& visitor) override;

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

protected:
    void do_traverse_children(FunctionRef<void(AstNode*)> callback) const override;
    void do_mutate_children(MutableAstVisitor& visitor) override;
};

/// Represents an array expression.
class AstArrayLiteral final : public AstLiteral {
public:
    AstArrayLiteral();

    ~AstArrayLiteral();

    AstNodeList<AstExpr>& items();
    const AstNodeList<AstExpr>& items() const;
    void items(AstNodeList<AstExpr> new_items);

protected:
    void do_traverse_children(FunctionRef<void(AstNode*)> callback) const override;
    void do_mutate_children(MutableAstVisitor& visitor) override;

private:
    AstNodeList<AstExpr> items_;
};

/// Represents a boolean literal.
class AstBooleanLiteral final : public AstLiteral {
public:
    AstBooleanLiteral(bool value);

    ~AstBooleanLiteral();

    bool value() const;
    void value(bool new_value);

protected:
    void do_traverse_children(FunctionRef<void(AstNode*)> callback) const override;
    void do_mutate_children(MutableAstVisitor& visitor) override;

private:
    bool value_;
};

/// Represents a floating point literal.
class AstFloatLiteral final : public AstLiteral {
public:
    AstFloatLiteral(f64 value);

    ~AstFloatLiteral();

    f64 value() const;
    void value(f64 new_value);

protected:
    void do_traverse_children(FunctionRef<void(AstNode*)> callback) const override;
    void do_mutate_children(MutableAstVisitor& visitor) override;

private:
    f64 value_;
};

/// Represents an integer literal.
class AstIntegerLiteral final : public AstLiteral {
public:
    AstIntegerLiteral(i64 value);

    ~AstIntegerLiteral();

    i64 value() const;
    void value(i64 new_value);

protected:
    void do_traverse_children(FunctionRef<void(AstNode*)> callback) const override;
    void do_mutate_children(MutableAstVisitor& visitor) override;

private:
    i64 value_;
};

/// Represents a map expression.
class AstMapLiteral final : public AstLiteral {
public:
    AstMapLiteral();

    ~AstMapLiteral();

    AstNodeList<AstMapItem>& items();
    const AstNodeList<AstMapItem>& items() const;
    void items(AstNodeList<AstMapItem> new_items);

protected:
    void do_traverse_children(FunctionRef<void(AstNode*)> callback) const override;
    void do_mutate_children(MutableAstVisitor& visitor) override;

private:
    AstNodeList<AstMapItem> items_;
};

/// Represents a null literal.
class AstNullLiteral final : public AstLiteral {
public:
    AstNullLiteral();

    ~AstNullLiteral();

protected:
    void do_traverse_children(FunctionRef<void(AstNode*)> callback) const override;
    void do_mutate_children(MutableAstVisitor& visitor) override;
};

/// Represents a record expression.
class AstRecordLiteral final : public AstLiteral {
public:
    AstRecordLiteral();

    ~AstRecordLiteral();

    AstNodeList<AstRecordItem>& items();
    const AstNodeList<AstRecordItem>& items() const;
    void items(AstNodeList<AstRecordItem> new_items);

protected:
    void do_traverse_children(FunctionRef<void(AstNode*)> callback) const override;
    void do_mutate_children(MutableAstVisitor& visitor) override;

private:
    AstNodeList<AstRecordItem> items_;
};

/// Represents a set expression.
class AstSetLiteral final : public AstLiteral {
public:
    AstSetLiteral();

    ~AstSetLiteral();

    AstNodeList<AstExpr>& items();
    const AstNodeList<AstExpr>& items() const;
    void items(AstNodeList<AstExpr> new_items);

protected:
    void do_traverse_children(FunctionRef<void(AstNode*)> callback) const override;
    void do_mutate_children(MutableAstVisitor& visitor) override;

private:
    AstNodeList<AstExpr> items_;
};

/// Represents a string literal.
class AstStringLiteral final : public AstLiteral {
public:
    AstStringLiteral(InternedString value);

    ~AstStringLiteral();

    InternedString value() const;
    void value(InternedString new_value);

protected:
    void do_traverse_children(FunctionRef<void(AstNode*)> callback) const override;
    void do_mutate_children(MutableAstVisitor& visitor) override;

private:
    InternedString value_;
};

/// Represents a symbol.
class AstSymbolLiteral final : public AstLiteral {
public:
    AstSymbolLiteral(InternedString value);

    ~AstSymbolLiteral();

    InternedString value() const;
    void value(InternedString new_value);

protected:
    void do_traverse_children(FunctionRef<void(AstNode*)> callback) const override;
    void do_mutate_children(MutableAstVisitor& visitor) override;

private:
    InternedString value_;
};

/// Represents a tuple expression.
class AstTupleLiteral final : public AstLiteral {
public:
    AstTupleLiteral();

    ~AstTupleLiteral();

    AstNodeList<AstExpr>& items();
    const AstNodeList<AstExpr>& items() const;
    void items(AstNodeList<AstExpr> new_items);

protected:
    void do_traverse_children(FunctionRef<void(AstNode*)> callback) const override;
    void do_mutate_children(MutableAstVisitor& visitor) override;

private:
    AstNodeList<AstExpr> items_;
};

/// Represents an access to an object property.
class AstPropertyExpr final : public AstExpr {
public:
    AstPropertyExpr(AccessType access_type);

    ~AstPropertyExpr();

    AccessType access_type() const;
    void access_type(AccessType new_access_type);

    AstExpr* instance() const;
    void instance(AstPtr<AstExpr> new_instance);

    AstIdentifier* property() const;
    void property(AstPtr<AstIdentifier> new_property);

protected:
    void do_traverse_children(FunctionRef<void(AstNode*)> callback) const override;
    void do_mutate_children(MutableAstVisitor& visitor) override;

private:
    AccessType access_type_;
    AstPtr<AstExpr> instance_;
    AstPtr<AstIdentifier> property_;
};

/// Represents a return expression with an optional return value.
class AstReturnExpr final : public AstExpr {
public:
    AstReturnExpr();

    ~AstReturnExpr();

    AstExpr* value() const;
    void value(AstPtr<AstExpr> new_value);

protected:
    void do_traverse_children(FunctionRef<void(AstNode*)> callback) const override;
    void do_mutate_children(MutableAstVisitor& visitor) override;

private:
    AstPtr<AstExpr> value_;
};

/// Represents a string expression consisting of literal strings and formatted sub expressions.
class AstStringExpr final : public AstExpr {
public:
    AstStringExpr();

    ~AstStringExpr();

    AstNodeList<AstExpr>& items();
    const AstNodeList<AstExpr>& items() const;
    void items(AstNodeList<AstExpr> new_items);

protected:
    void do_traverse_children(FunctionRef<void(AstNode*)> callback) const override;
    void do_mutate_children(MutableAstVisitor& visitor) override;

private:
    AstNodeList<AstExpr> items_;
};

/// Represents a unary expression.
class AstUnaryExpr final : public AstExpr {
public:
    AstUnaryExpr(UnaryOperator operation);

    ~AstUnaryExpr();

    UnaryOperator operation() const;
    void operation(UnaryOperator new_operation);

    AstExpr* inner() const;
    void inner(AstPtr<AstExpr> new_inner);

protected:
    void do_traverse_children(FunctionRef<void(AstNode*)> callback) const override;
    void do_mutate_children(MutableAstVisitor& visitor) override;

private:
    UnaryOperator operation_;
    AstPtr<AstExpr> inner_;
};

/// Represents a reference to a variable.
class AstVarExpr final : public AstExpr {
public:
    AstVarExpr(InternedString name);

    ~AstVarExpr();

    InternedString name() const;
    void name(InternedString new_name);

protected:
    void do_traverse_children(FunctionRef<void(AstNode*)> callback) const override;
    void do_mutate_children(MutableAstVisitor& visitor) override;

private:
    InternedString name_;
};

/// Represents an identifier in a property access expression.
class AstIdentifier : public AstNode {
protected:
    AstIdentifier(AstNodeType type);

public:
    ~AstIdentifier();

protected:
    void do_traverse_children(FunctionRef<void(AstNode*)> callback) const override;
    void do_mutate_children(MutableAstVisitor& visitor) override;
};

/// Represents an integer literal in an identifier context (such as a tuple member expression).
class AstNumericIdentifier final : public AstIdentifier {
public:
    AstNumericIdentifier(u32 value);

    ~AstNumericIdentifier();

    u32 value() const;
    void value(u32 new_value);

protected:
    void do_traverse_children(FunctionRef<void(AstNode*)> callback) const override;
    void do_mutate_children(MutableAstVisitor& visitor) override;

private:
    u32 value_;
};

/// Represents the name of a variable or a field.
class AstStringIdentifier final : public AstIdentifier {
public:
    AstStringIdentifier(InternedString value);

    ~AstStringIdentifier();

    InternedString value() const;
    void value(InternedString new_value);

protected:
    void do_traverse_children(FunctionRef<void(AstNode*)> callback) const override;
    void do_mutate_children(MutableAstVisitor& visitor) override;

private:
    InternedString value_;
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

protected:
    void do_traverse_children(FunctionRef<void(AstNode*)> callback) const override;
    void do_mutate_children(MutableAstVisitor& visitor) override;

private:
    AstPtr<AstExpr> key_;
    AstPtr<AstExpr> value_;
};

/// Represents a key-value pair in a record expression. All keys are StringIdentifiers.
class AstRecordItem final : public AstNode {
public:
    AstRecordItem();

    ~AstRecordItem();

    AstStringIdentifier* key() const;
    void key(AstPtr<AstStringIdentifier> new_key);

    AstExpr* value() const;
    void value(AstPtr<AstExpr> new_value);

protected:
    void do_traverse_children(FunctionRef<void(AstNode*)> callback) const override;
    void do_mutate_children(MutableAstVisitor& visitor) override;

private:
    AstPtr<AstStringIdentifier> key_;
    AstPtr<AstExpr> value_;
};
// [[[end]]]

} // namespace tiro

TIRO_ENABLE_FREE_TO_STRING(tiro::UnaryOperator)
TIRO_ENABLE_FREE_TO_STRING(tiro::BinaryOperator)

#endif // TIRO_COMPILER_AST_EXPR_HPP
