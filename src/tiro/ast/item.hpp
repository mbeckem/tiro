#ifndef TIRO_AST_ITEM_HPP
#define TIRO_AST_ITEM_HPP

#include "tiro/ast/node.hpp"

namespace tiro {

/* [[[cog
    from codegen.ast import NODE_TYPES, define, walk_types
    
    node_types = list(walk_types(NODE_TYPES.get("Item")))
    file_type = NODE_TYPES.get("File")
    define(*node_types, file_type)
]]] */
/// Represents the contents of a toplevel item.
class AstItem : public AstNode {
protected:
    AstItem(AstNodeType type);

public:
    ~AstItem();

protected:
    void do_traverse_children(FunctionRef<void(AstNode*)> callback) override;
    void do_mutate_children(MutableAstVisitor& visitor) override;
};

/// Represents an empty item.
class AstEmptyItem final : public AstItem {
public:
    AstEmptyItem();

    ~AstEmptyItem();

protected:
    void do_traverse_children(FunctionRef<void(AstNode*)> callback) override;
    void do_mutate_children(MutableAstVisitor& visitor) override;
};

/// Represents a function item.
class AstFuncItem final : public AstItem {
public:
    AstFuncItem();

    ~AstFuncItem();

    AstFuncDecl* decl() const;
    void decl(AstPtr<AstFuncDecl> new_decl);

protected:
    void do_traverse_children(FunctionRef<void(AstNode*)> callback) override;
    void do_mutate_children(MutableAstVisitor& visitor) override;

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

    std::vector<InternedString>& path();
    const std::vector<InternedString>& path() const;
    void path(std::vector<InternedString> new_path);

protected:
    void do_traverse_children(FunctionRef<void(AstNode*)> callback) override;
    void do_mutate_children(MutableAstVisitor& visitor) override;

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

protected:
    void do_traverse_children(FunctionRef<void(AstNode*)> callback) override;
    void do_mutate_children(MutableAstVisitor& visitor) override;

private:
    AstPtr<AstVarDecl> decl_;
};

/// Represents the contents of a file.
class AstFile final : public AstNode {
public:
    AstFile();

    ~AstFile();

    AstNodeList<AstItem>& items();
    const AstNodeList<AstItem>& items() const;
    void items(AstNodeList<AstItem> new_items);

protected:
    void do_traverse_children(FunctionRef<void(AstNode*)> callback) override;
    void do_mutate_children(MutableAstVisitor& visitor) override;

private:
    AstNodeList<AstItem> items_;
};
// [[[end]]]

} // namespace tiro

#endif // TIRO_AST_ITEM_HPP
