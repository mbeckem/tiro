#ifndef TIRO_AST_STMT_HPP
#define TIRO_AST_STMT_HPP

#include "tiro/ast/node.hpp"

namespace tiro {

/* [[[cog
    from codegen.ast import NODE_TYPES, define, walk_types
    
    node_types = list(walk_types(NODE_TYPES.get("Stmt")))
    define(*node_types)
]]] */
/// Represents a statement.
class AstStmt : public AstNode {
protected:
    AstStmt(AstNodeType type);

public:
    ~AstStmt();
};

/// Represents an assert statement with an optional message.
class AstAssertStmt final : public AstStmt {
public:
    AstAssertStmt();

    ~AstAssertStmt();

    const AstPtr<AstExpr>& cond() const { return cond_; }
    void cond(AstPtr<AstExpr> new_cond) { cond_ = std::move(new_cond); }

    const AstPtr<AstExpr>& message() const { return message_; }
    void message(AstPtr<AstExpr> new_message) {
        message_ = std::move(new_message);
    }

private:
    AstPtr<AstExpr> cond_;
    AstPtr<AstExpr> message_;
};

/// Represents an empty statement.
class AstEmptyStmt final : public AstStmt {
public:
    AstEmptyStmt();

    ~AstEmptyStmt();
};

/// Represents a for loop.
class AstForStmt final : public AstStmt {
public:
    AstForStmt();

    ~AstForStmt();

    const AstPtr<AstVarItem>& decl() const { return decl_; }
    void decl(AstPtr<AstVarItem> new_decl) { decl_ = std::move(new_decl); }

    const AstPtr<AstExpr>& cond() const { return cond_; }
    void cond(AstPtr<AstExpr> new_cond) { cond_ = std::move(new_cond); }

    const AstPtr<AstExpr>& step() const { return step_; }
    void step(AstPtr<AstExpr> new_step) { step_ = std::move(new_step); }

    const AstPtr<AstExpr>& body() const { return body_; }
    void body(AstPtr<AstExpr> new_body) { body_ = std::move(new_body); }

private:
    AstPtr<AstVarItem> decl_;
    AstPtr<AstExpr> cond_;
    AstPtr<AstExpr> step_;
    AstPtr<AstExpr> body_;
};

/// Represents an item in a statement context.
class AstItemStmt final : public AstStmt {
public:
    AstItemStmt();

    ~AstItemStmt();

    const AstPtr<AstItem>& item() const { return item_; }
    void item(AstPtr<AstItem> new_item) { item_ = std::move(new_item); }

private:
    AstPtr<AstItem> item_;
};

/// Represents a while loop.
class AstWhileStmt final : public AstStmt {
public:
    AstWhileStmt();

    ~AstWhileStmt();

    const AstPtr<AstExpr>& cond() const { return cond_; }
    void cond(AstPtr<AstExpr> new_cond) { cond_ = std::move(new_cond); }

    const AstPtr<AstExpr>& body() const { return body_; }
    void body(AstPtr<AstExpr> new_body) { body_ = std::move(new_body); }

private:
    AstPtr<AstExpr> cond_;
    AstPtr<AstExpr> body_;
};
// [[[end]]]

} // namespace tiro

#endif // TIRO_AST_STMT_HPP
