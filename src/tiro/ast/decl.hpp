#ifndef TIRO_AST_DECL_HPP
#define TIRO_AST_DECL_HPP

#include "tiro/ast/node.hpp"

namespace tiro {

/* [[[cog
    from codegen.ast import NODE_TYPES, define, walk_types
    
    decl_types = list(walk_types(NODE_TYPES.get("Decl")))
    binding_types = list(walk_types(NODE_TYPES.get("Binding")))
    define(*decl_types, *binding_types)
]]] */
/// Represents a declaration.
class AstDecl : public AstNode {
protected:
    AstDecl(AstNodeType type);

public:
    ~AstDecl();
};

/// Represents a function declaration.
class AstFuncDecl final : public AstDecl {
public:
    AstFuncDecl();

    ~AstFuncDecl();

    InternedString name() const;
    void name(InternedString new_name);

    AstNodeList<AstParamDecl>& params();
    const AstNodeList<AstParamDecl>& params() const;
    void params(AstNodeList<AstParamDecl> new_params);

    AstExpr* body() const;
    void body(AstPtr<AstExpr> new_body);

private:
    InternedString name_;
    AstNodeList<AstParamDecl> params_;
    AstPtr<AstExpr> body_;
};

/// Represents a function parameter declaration.
class AstParamDecl final : public AstDecl {
public:
    AstParamDecl();

    ~AstParamDecl();

    InternedString name() const;
    void name(InternedString new_name);

private:
    InternedString name_;
};

/// Represents the declaration of a number of variables.
class AstVarDecl final : public AstDecl {
public:
    AstVarDecl();

    ~AstVarDecl();

    AstNodeList<AstBinding>& bindings();
    const AstNodeList<AstBinding>& bindings() const;
    void bindings(AstNodeList<AstBinding> new_bindings);

private:
    AstNodeList<AstBinding> bindings_;
};

/// Represents a binding of one or more variables to a value
class AstBinding : public AstNode {
protected:
    AstBinding(AstNodeType type);

public:
    ~AstBinding();

    bool is_const() const;
    void is_const(bool new_is_const);

private:
    bool is_const_;
};

/// Represents a tuple that is being unpacked into a number of variables.
class AstTupleBinding final : public AstBinding {
public:
    AstTupleBinding();

    ~AstTupleBinding();

    const std::vector<InternedString>& names() const;
    void names(std::vector<InternedString> new_names);

private:
    std::vector<InternedString> names_;
};

/// Represents a variable name bound to an (optional) value.
class AstVarBinding final : public AstBinding {
public:
    AstVarBinding();

    ~AstVarBinding();

    InternedString name() const;
    void name(InternedString new_name);

private:
    InternedString name_;
};
// [[[end]]]

} // namespace tiro

#endif // TIRO_AST_DECL_HPP
