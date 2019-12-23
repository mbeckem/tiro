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

protected:
    void dump_impl(NodeFormatter& fmt) const;

    template<typename Visitor>
    void visit_children(Visitor&& v) {
        Node::visit_children(v);
    };

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
class VarDecl final : public Decl {
public:
    VarDecl();
    ~VarDecl();

    bool is_const() const { return is_const_; }
    void is_const(bool const_decl) { is_const_ = const_decl; }

    // The initializer may be null.
    Expr* initializer() const;
    void initializer(std::unique_ptr<Expr> value);

    void dump_impl(NodeFormatter& fmt) const;

    template<typename Visitor>
    void visit_children(Visitor&& v) {
        Decl::visit_children(v);
        v(initializer());
    };

private:
    bool is_const_ = false;
    std::unique_ptr<Expr> initializer_;
};

/**
 * Represents a function.
 */
class FuncDecl final : public Decl, public Scope {
public:
    FuncDecl();
    ~FuncDecl();

    void add_param(std::unique_ptr<ParamDecl> param);
    ParamDecl* get_param(size_t index) const;
    size_t param_count() const { return params_.size(); }

    BlockExpr* body() const;
    void body(std::unique_ptr<BlockExpr> block);

    void dump_impl(NodeFormatter& fmt) const;

    template<typename Visitor>
    void visit_children(Visitor&& v) {
        Decl::visit_children(v);
        params_.for_each(v);
        v(body());
    };

private:
    InternedString name_;
    NodeVector<ParamDecl> params_;
    std::unique_ptr<BlockExpr> body_;
};

/**
 * Represents a formal parameter to a function.
 */
class ParamDecl final : public Decl {
public:
    ParamDecl();
    ~ParamDecl();

    template<typename Visitor>
    void visit_children(Visitor&& v) {
        Decl::visit_children(v);
    }

    void dump_impl(NodeFormatter& fmt) const;

private:
};

/**
 * Represents an imported symbol.
 */
class ImportDecl final : public Decl {
public:
    ImportDecl();
    ~ImportDecl();

    template<typename Visitor>
    void visit_children(Visitor&& v) {
        Decl::visit_children(v);
    }

    void dump_impl(NodeFormatter& fmt) const;
};

} // namespace hammer::ast

#endif // HAMMER_AST_DECL_HPP
