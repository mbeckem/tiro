#include "tiro/ast/ast.hpp"

namespace tiro {

/* [[[cog
    from codegen.ast import NODE_TYPES, implement, walk_types
    
    node_types = list(walk_types(NODE_TYPES.get("Item")))
    file_type = NODE_TYPES.get("File")
    implement(*node_types, file_type)
]]] */
AstItem::AstItem(AstNodeType type)
    : AstNode(type) {
    TIRO_DEBUG_ASSERT(
        type >= AstNodeType::FirstItem && type <= AstNodeType::LastItem,
        "Derived type is invalid for this base class.");
}

AstItem::~AstItem() = default;

AstEmptyItem::AstEmptyItem()
    : AstItem(AstNodeType::EmptyItem) {}

AstEmptyItem::~AstEmptyItem() = default;

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

std::vector<InternedString>& AstImportItem::path() {
    return path_;
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

AstFile::AstFile()
    : AstNode(AstNodeType::File)
    , items_() {}

AstFile::~AstFile() = default;

AstNodeList<AstItem>& AstFile::items() {
    return items_;
}

const AstNodeList<AstItem>& AstFile::items() const {
    return items_;
}

void AstFile::items(AstNodeList<AstItem> new_items) {
    items_ = std::move(new_items);
}

// [[[end]]]

} // namespace tiro
