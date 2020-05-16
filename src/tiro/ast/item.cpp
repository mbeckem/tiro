#include "tiro/ast/item.hpp"

namespace tiro {

/* [[[cog
    from codegen.ast import NODE_TYPES, implement, walk_types
    
    node_types = list(walk_types(NODE_TYPES.get("Item")))
    implement(*node_types)
]]] */
AstItem::AstItem(AstNodeType type)
    : AstNode(type) {
    TIRO_DEBUG_ASSERT(
        type >= AstNodeType::FirstItem && type <= AstNodeType::LastItem,
        "Derived type is invalid for this base class.");
}

AstItem::~AstItem() = default;

AstFuncItem::AstFuncItem()
    : AstItem(AstNodeType::FuncItem)
    , decl_() {}

AstFuncItem::~AstFuncItem() = default;

AstFuncDecl* AstFuncItem::decl() const {
    return decl_.get();
}

void AstFuncItem::decl(AstPtr<AstFuncDecl> new_decl) {
    decl_ = std::move(new_decl);
}

AstImportItem::AstImportItem()
    : AstItem(AstNodeType::ImportItem)
    , name_()
    , path_() {}

AstImportItem::~AstImportItem() = default;

InternedString AstImportItem::name() const {
    return name_;
}

void AstImportItem::name(InternedString new_name) {
    name_ = std::move(new_name);
}

const std::vector<InternedString>& AstImportItem::path() const {
    return path_;
}

void AstImportItem::path(std::vector<InternedString> new_path) {
    path_ = std::move(new_path);
}

AstVarItem::AstVarItem()
    : AstItem(AstNodeType::VarItem)
    , decl_() {}

AstVarItem::~AstVarItem() = default;

AstVarDecl* AstVarItem::decl() const {
    return decl_.get();
}

void AstVarItem::decl(AstPtr<AstVarDecl> new_decl) {
    decl_ = std::move(new_decl);
}

// [[[end]]]

} // namespace tiro
