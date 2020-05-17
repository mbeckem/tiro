#ifndef TIRO_AST_NODE_HPP
#define TIRO_AST_NODE_HPP

#include "tiro/ast/fwd.hpp"
#include "tiro/ast/operators.hpp"
#include "tiro/ast/ptr.hpp"
#include "tiro/compiler/source_reference.hpp"
#include "tiro/core/format.hpp"
#include "tiro/core/id_type.hpp"
#include "tiro/core/string_table.hpp"

#include <string_view>
#include <vector>

namespace tiro {

TIRO_DEFINE_ID(AstId, u32);

enum class AstNodeType : u8 {
    /* [[[cog
        import cog
        from codegen.ast import type_tags
        for (name, value) in type_tags():
            cog.outl(f"{name} = {value},");
    ]]] */
    TupleBinding = 1,
    VarBinding = 2,
    FirstBinding = 1,
    LastBinding = 2,
    FuncDecl = 3,
    ParamDecl = 4,
    VarDecl = 5,
    FirstDecl = 3,
    LastDecl = 5,
    BinaryExpr = 6,
    BlockExpr = 7,
    BreakExpr = 8,
    CallExpr = 9,
    ContinueExpr = 10,
    ElementExpr = 11,
    FuncExpr = 12,
    IfExpr = 13,
    ArrayLiteral = 14,
    BooleanLiteral = 15,
    FloatLiteral = 16,
    IntegerLiteral = 17,
    MapLiteral = 18,
    NullLiteral = 19,
    SetLiteral = 20,
    StringLiteral = 21,
    SymbolLiteral = 22,
    TupleLiteral = 23,
    FirstLiteral = 14,
    LastLiteral = 23,
    PropertyExpr = 24,
    ReturnExpr = 25,
    StringExpr = 26,
    UnaryExpr = 27,
    VarExpr = 28,
    FirstExpr = 6,
    LastExpr = 28,
    NumericIdentifier = 29,
    StringIdentifier = 30,
    FirstIdentifier = 29,
    LastIdentifier = 30,
    FuncItem = 31,
    ImportItem = 32,
    VarItem = 33,
    FirstItem = 31,
    LastItem = 33,
    MapItem = 34,
    AssertStmt = 35,
    EmptyStmt = 36,
    ExprStmt = 37,
    ForStmt = 38,
    ItemStmt = 39,
    WhileStmt = 40,
    FirstStmt = 35,
    LastStmt = 40,
    FirstNode = 1,
    LastNode = 40,
    // [[[end]]]
};

std::string_view to_string(AstNodeType type);

enum class AstNodeFlags : u32 {
    None = 0,
    HasError = 1 << 0,
};

AstNodeFlags operator|(AstNodeFlags lhs, AstNodeFlags rhs);
AstNodeFlags operator&(AstNodeFlags lhs, AstNodeFlags rhs);
bool test(AstNodeFlags flags, AstNodeFlags test);

void format(FormatStream& stream, AstNodeFlags flags);

/// Base class of all AST nodes.
class AstNode {
public:
    virtual ~AstNode();

    AstNode(const AstNode&) = delete;
    AstNode& operator=(const AstNode&) = delete;

    AstNodeType type() const { return type_; }

    AstId id() const { return id_; }
    void id(AstId new_id) { id_ = new_id; }

    SourceReference source() const { return source_; }
    void source(const SourceReference& new_source) { source_ = new_source; }

    AstNodeFlags flags() const { return flags_; }
    void flags(AstNodeFlags new_flags) { flags_ = new_flags; }

    bool has_error() const { return test(flags_, AstNodeFlags::HasError); }

protected:
    explicit AstNode(AstNodeType type);

private:
    const AstNodeType type_;
    AstId id_;
    SourceReference source_;
    AstNodeFlags flags_ = AstNodeFlags::None;
};

/// A list of AST nodes, backed by a `std::vector`.
template<typename NodeType>
class AstNodeList final {
public:
    AstNodeList();
    ~AstNodeList();

    AstNodeList(AstNodeList&& other) noexcept = default;
    AstNodeList& operator=(AstNodeList&& other) noexcept = default;

    bool empty() const { return items_.empty(); }

    size_t size() const { return items_.size(); }

    NodeType* get(size_t index) const {
        TIRO_DEBUG_ASSERT(index < size(), "AstNodeList: Index out of bounds.");
        return items_[index].get();
    }

    void set(size_t index, AstPtr<NodeType> node) {
        TIRO_DEBUG_ASSERT(index < size(), "AstNodeList: Index out of bounds.");
        return items_[index] = std::move(node);
    }

    void append(AstPtr<NodeType> node) { items_.push_back(std::move(node)); }

private:
    std::vector<AstPtr<NodeType>> items_;
};

enum class AccessType : u8 {
    Normal,
    Optional, // Null propagation, e.g. `instance?.member`.
};

std::string_view to_string(AccessType access);

} // namespace tiro

TIRO_ENABLE_FREE_FORMAT(tiro::AstNodeFlags);
TIRO_ENABLE_FREE_TO_STRING(tiro::AstNodeType);
TIRO_ENABLE_FREE_TO_STRING(tiro::AccessType);

#endif // TIRO_AST_NODE_HPP
