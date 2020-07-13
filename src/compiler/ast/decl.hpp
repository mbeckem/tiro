#ifndef TIRO_COMPILER_AST_DECL_HPP
#define TIRO_COMPILER_AST_DECL_HPP

#include "compiler/ast/node.hpp"

namespace tiro {

/* [[[cog
    from codegen.ast import NODE_TYPES, define, walk_types

    roots = map(lambda name: NODE_TYPES.get(name), ["Decl", "Binding", "BindingSpec", "Modifier"])
    types = walk_types(*roots)
    define(*types)
]]] */
/// Represents a declaration.
class AstDecl : public AstNode {
protected:
    AstDecl(AstNodeType type);

public:
    ~AstDecl();

    AstNodeList<AstModifier>& modifiers();
    const AstNodeList<AstModifier>& modifiers() const;
    void modifiers(AstNodeList<AstModifier> new_modifiers);

protected:
    void do_traverse_children(FunctionRef<void(AstNode*)> callback) const override;
    void do_mutate_children(MutableAstVisitor& visitor) override;

private:
    AstNodeList<AstModifier> modifiers_;
};

/// Represents a function declaration.
class AstFuncDecl final : public AstDecl {
public:
    AstFuncDecl();

    ~AstFuncDecl();

    InternedString name() const;
    void name(InternedString new_name);

    bool body_is_value() const;
    void body_is_value(bool new_body_is_value);

    AstNodeList<AstParamDecl>& params();
    const AstNodeList<AstParamDecl>& params() const;
    void params(AstNodeList<AstParamDecl> new_params);

    AstExpr* body() const;
    void body(AstPtr<AstExpr> new_body);

protected:
    void do_traverse_children(FunctionRef<void(AstNode*)> callback) const override;
    void do_mutate_children(MutableAstVisitor& visitor) override;

private:
    InternedString name_;
    bool body_is_value_;
    AstNodeList<AstParamDecl> params_;
    AstPtr<AstExpr> body_;
};

/// Represents a module import.
class AstImportDecl final : public AstDecl {
public:
    AstImportDecl();

    ~AstImportDecl();

    InternedString name() const;
    void name(InternedString new_name);

    std::vector<InternedString>& path();
    const std::vector<InternedString>& path() const;
    void path(std::vector<InternedString> new_path);

protected:
    void do_traverse_children(FunctionRef<void(AstNode*)> callback) const override;
    void do_mutate_children(MutableAstVisitor& visitor) override;

private:
    InternedString name_;
    std::vector<InternedString> path_;
};

/// Represents a function parameter declaration.
class AstParamDecl final : public AstDecl {
public:
    AstParamDecl();

    ~AstParamDecl();

    InternedString name() const;
    void name(InternedString new_name);

protected:
    void do_traverse_children(FunctionRef<void(AstNode*)> callback) const override;
    void do_mutate_children(MutableAstVisitor& visitor) override;

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

protected:
    void do_traverse_children(FunctionRef<void(AstNode*)> callback) const override;
    void do_mutate_children(MutableAstVisitor& visitor) override;

private:
    AstNodeList<AstBinding> bindings_;
};

/// Represents a binding of one or more variables to a value
class AstBinding final : public AstNode {
public:
    AstBinding(bool is_const);

    ~AstBinding();

    bool is_const() const;
    void is_const(bool new_is_const);

    AstBindingSpec* spec() const;
    void spec(AstPtr<AstBindingSpec> new_spec);

    AstExpr* init() const;
    void init(AstPtr<AstExpr> new_init);

protected:
    void do_traverse_children(FunctionRef<void(AstNode*)> callback) const override;
    void do_mutate_children(MutableAstVisitor& visitor) override;

private:
    bool is_const_;
    AstPtr<AstBindingSpec> spec_;
    AstPtr<AstExpr> init_;
};

/// Represents the variable specifiers in the left hand side of a binding.
class AstBindingSpec : public AstNode {
protected:
    AstBindingSpec(AstNodeType type);

public:
    ~AstBindingSpec();

protected:
    void do_traverse_children(FunctionRef<void(AstNode*)> callback) const override;
    void do_mutate_children(MutableAstVisitor& visitor) override;
};

/// Represents a tuple that is being unpacked into a number of variables.
class AstTupleBindingSpec final : public AstBindingSpec {
public:
    AstTupleBindingSpec();

    ~AstTupleBindingSpec();

    AstNodeList<AstStringIdentifier>& names();
    const AstNodeList<AstStringIdentifier>& names() const;
    void names(AstNodeList<AstStringIdentifier> new_names);

protected:
    void do_traverse_children(FunctionRef<void(AstNode*)> callback) const override;
    void do_mutate_children(MutableAstVisitor& visitor) override;

private:
    AstNodeList<AstStringIdentifier> names_;
};

/// Represents a variable name bound to an (optional) value.
class AstVarBindingSpec final : public AstBindingSpec {
public:
    AstVarBindingSpec();

    ~AstVarBindingSpec();

    AstStringIdentifier* name() const;
    void name(AstPtr<AstStringIdentifier> new_name);

protected:
    void do_traverse_children(FunctionRef<void(AstNode*)> callback) const override;
    void do_mutate_children(MutableAstVisitor& visitor) override;

private:
    AstPtr<AstStringIdentifier> name_;
};

/// Represents a item modifier.
class AstModifier : public AstNode {
protected:
    AstModifier(AstNodeType type);

public:
    ~AstModifier();

protected:
    void do_traverse_children(FunctionRef<void(AstNode*)> callback) const override;
    void do_mutate_children(MutableAstVisitor& visitor) override;
};

/// Represents an export modifier.
class AstExportModifier final : public AstModifier {
public:
    AstExportModifier();

    ~AstExportModifier();

protected:
    void do_traverse_children(FunctionRef<void(AstNode*)> callback) const override;
    void do_mutate_children(MutableAstVisitor& visitor) override;
};
// [[[end]]]

} // namespace tiro

#endif // TIRO_COMPILER_AST_DECL_HPP
