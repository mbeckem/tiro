#ifndef TIRO_AST_TRAVERSAL_HPP
#define TIRO_AST_TRAVERSAL_HPP

#include "tiro/ast/fwd.hpp"
#include "tiro/core/defs.hpp"
#include "tiro/core/function_ref.hpp"
#include "tiro/core/not_null.hpp"

namespace tiro {

/// Represents a child of an AST node. Mutable children can be replaced.
template<typename Node>
class MutableChild final {
public:
    explicit MutableChild(
        Node* current, FunctionRef<void(AstPtr<Node>)> replace)
        : current_(current)
        , replace_(replace) {}

    MutableChild(MutableChild&&) noexcept = default;

    MutableChild(const MutableChild&) = delete;
    MutableChild& operator=(const MutableChild&) = delete;

    /// Returns the current child value.
    Node* get() const { return current_; }

    /// Returns the current child value.
    Node* operator->() const {
        TIRO_DEBUG_ASSERT(current_ != nullptr, "Invalid child pointer.");
        return current_;
    }

    /// Replaces the current child with the new value. Note that the original child will be deleted.
    void replace(AstPtr<Node> new_value) {
        Node* addr = new_value.get();
        replace_(std::move(new_value));
        current_ = addr;
    }

    explicit operator bool() const { return static_cast<bool>(current_); }

private:
    Node* current_;                           // current child pointer
    FunctionRef<void(AstPtr<Node>)> replace_; // updates child in node
};

/// This interface must be implemented by callers that wish to modify the AST.
/// The visitor will be invoked for every child slot within a parent node.
/// Note that the default implementations of the visit_* functions do nothing.
class MutableAstVisitor {
public:
    MutableAstVisitor();
    virtual ~MutableAstVisitor();

    /* [[[cog
        from cog import outl
        from codegen.ast import slot_types

        for node_type in slot_types():
            outl(f"virtual void {node_type.visitor_name}(MutableChild<{node_type.cpp_name}> child);")
    ]]] */
    virtual void visit_binding(MutableChild<AstBinding> child);
    virtual void visit_expr(MutableChild<AstExpr> child);
    virtual void visit_func_decl(MutableChild<AstFuncDecl> child);
    virtual void visit_identifier(MutableChild<AstIdentifier> child);
    virtual void visit_item(MutableChild<AstItem> child);
    virtual void visit_map_item(MutableChild<AstMapItem> child);
    virtual void visit_param_decl(MutableChild<AstParamDecl> child);
    virtual void visit_stmt(MutableChild<AstStmt> child);
    virtual void visit_string_expr(MutableChild<AstStringExpr> child);
    virtual void visit_var_decl(MutableChild<AstVarDecl> child);
    // [[[end]]]
};

/// Invokes the callback function for every child of the given node.
void visit_children(
    NotNull<const AstNode*> node, FunctionRef<void(AstNode*)> callback);

/// Invokes the visitor for every child of the given node.
void mutate_children(NotNull<AstNode*> node, MutableAstVisitor& visitor);

} // namespace tiro

#endif // TIRO_AST_TRAVERSAL_HPP
