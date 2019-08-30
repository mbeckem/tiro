#ifndef HAMMER_AST_DECL_HPP
#define HAMMER_AST_DECL_HPP

#include "hammer/ast/node.hpp"
#include "hammer/ast/scope.hpp"

namespace hammer::ast {

/**
 * Base class for constructs that introduce a symbol into their surrounding scope,
 * such as variable or function declarations.
 */
class Decl : public Node {
public:
    ~Decl();

    /// The name of the symbol. Can be unnamed.
    InternedString name() const { return name_; }
    void name(InternedString n) { name_ = n; }
    bool anonymous() const { return !name_.valid(); }

    /// True if the symbol is being referenced by a nested function.
    bool captured() const { return captured_; }
    void captured(bool captured) { captured_ = captured; }

    /// True if the symbol's declaration has been seen already. It is an error
    /// to use a symbol before it's declaration (in the same scope).
    bool active() const { return active_; }
    void active(bool active) { active_ = active; }

    /// The scope in which the symbol has been defined.
    Scope* parent_scope() const { return parent_scope_; }
    void parent_scope(Scope* sc) { parent_scope_ = sc; }

    /// The node that owns the parent scope.
    ast::Node* scope_owner() const;

    // TODO: Position and stuff

protected:
    void dump_impl(NodeFormatter& fmt) const;

protected:
    explicit Decl(NodeKind kind);

private:
    InternedString name_;
    Scope* parent_scope_ = nullptr;
    bool captured_ = false;
    bool active_ = false;
};

/**
 * Represents a variable with an optional initializer.
 */
class VarDecl : public Decl {
public:
    VarDecl();
    ~VarDecl();

    bool is_const() const { return is_const_; }
    void is_const(bool const_decl) { is_const_ = const_decl; }

    // The initializer may be null.
    Expr* initializer() const;
    void initializer(std::unique_ptr<Expr> value);

    void dump_impl(NodeFormatter& fmt) const;

private:
    bool is_const_ = false;
    Expr* initializer_ = nullptr;
};

/**
 * Represents a function.
 */
class FuncDecl : public Decl, public Scope {
public:
    FuncDecl();
    ~FuncDecl();

    ParamDecl* get_param(size_t index) const {
        HAMMER_ASSERT(index < params_.size(), "Index out of bounds.");
        return params_[index];
    }

    size_t param_count() const { return params_.size(); }

    void add_param(std::unique_ptr<ParamDecl> param);

    BlockExpr* body() const;
    void body(std::unique_ptr<BlockExpr> block);

    void dump_impl(NodeFormatter& fmt) const;

private:
    InternedString name_;
    std::vector<ParamDecl*> params_;
    BlockExpr* body_ = nullptr;
};

/**
 * Represents a formal parameter to a function.
 */
class ParamDecl : public Decl {
public:
    ParamDecl();
    ~ParamDecl();

    void dump_impl(NodeFormatter& fmt) const;

private:
};

/**
 * Represents an imported symbol.
 */
class ImportDecl : public Decl {
public:
    ImportDecl();
    ~ImportDecl();

    void dump_impl(NodeFormatter& fmt) const;
};

} // namespace hammer::ast

#endif // HAMMER_AST_DECL_HPP
