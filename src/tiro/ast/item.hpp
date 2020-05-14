#ifndef TIRO_AST_ITEM_HPP
#define TIRO_AST_ITEM_HPP

#include "tiro/ast/node.hpp"

namespace tiro {

/* [[[cog
    from codegen.ast import NODE_TYPES, define, walk_types
    
    node_types = list(walk_types(NODE_TYPES.get("Item")))
    define(*node_types)
]]] */
/// Represents the contents of a toplevel item.
class AstItem : public AstNode {
protected:
    AstItem(AstNodeType type);

public:
    ~AstItem();
};

/// Represents a function item.
class AstFuncItem final : public AstItem {
public:
    AstFuncItem();

    ~AstFuncItem();

    AstFuncDecl* decl() const;
    void decl(AstPtr<AstFuncDecl> new_decl);

private:
    AstPtr<AstFuncDecl> decl_;
};

/// Represents a module import.
class AstImportItem final : public AstItem {
public:
    AstImportItem();

    ~AstImportItem();

    InternedString name() const;
    void name(InternedString new_name);

    const std::vector<InternedString>& path() const;
    void path(std::vector<InternedString> new_path);

private:
    InternedString name_;
    std::vector<InternedString> path_;
};

/// Represents a variable item.
class AstVarItem final : public AstItem {
public:
    AstVarItem();

    ~AstVarItem();

    AstVarDecl* decl() const;
    void decl(AstPtr<AstVarDecl> new_decl);

private:
    AstPtr<AstVarDecl> decl_;
};
// [[[end]]]

} // namespace tiro

#endif // TIRO_AST_ITEM_HPP
