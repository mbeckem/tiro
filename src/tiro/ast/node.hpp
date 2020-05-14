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
    FuncItem = 29,
    ImportItem = 30,
    VarItem = 31,
    FirstItem = 29,
    LastItem = 31,
    MapItem = 32,
    AssertStmt = 33,
    EmptyStmt = 34,
    ExprStmt = 35,
    ForStmt = 36,
    ItemStmt = 37,
    WhileStmt = 38,
    FirstStmt = 33,
    LastStmt = 38,
    FirstNode = 1,
    LastNode = 38,
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

    AstNodeList(AstNodeList&& other) noexcept;
    AstNodeList& operator=(AstNodeList&& other) noexcept;

private:
    std::vector<AstPtr<NodeType>> items_;
};

enum class AccessType : u8 {
    Normal,
    Optional, // Null propagation, e.g. `instance?.member`.
};

std::string_view to_string(AccessType access);

/* [[[cog
    from codegen.unions import define
    from codegen.ast import Property
    define(Property.tag)
]]] */
enum class AstPropertyType : u8 {
    Field,
    TupleField,
};

std::string_view to_string(AstPropertyType type);
// [[[end]]]

/* [[[cog
    from codegen.unions import define
    from codegen.ast import Property
    define(Property)
]]] */
/// Represents the name of a property.
class AstProperty final {
public:
    /// Represents an object field.
    struct Field final {
        InternedString name;

        explicit Field(const InternedString& name_)
            : name(name_) {}
    };

    /// Represents a numeric field within a tuple.
    struct TupleField final {
        u32 index;

        explicit TupleField(const u32& index_)
            : index(index_) {}
    };

    static AstProperty make_field(const InternedString& name);
    static AstProperty make_tuple_field(const u32& index);

    AstProperty(Field field);
    AstProperty(TupleField tuple_field);

    AstPropertyType type() const noexcept { return type_; }

    const Field& as_field() const;
    const TupleField& as_tuple_field() const;

    template<typename Visitor, typename... Args>
    TIRO_FORCE_INLINE decltype(auto) visit(Visitor&& vis, Args&&... args) {
        return visit_impl(
            *this, std::forward<Visitor>(vis), std::forward<Args>(args)...);
    }

    template<typename Visitor, typename... Args>
    TIRO_FORCE_INLINE decltype(auto)
    visit(Visitor&& vis, Args&&... args) const {
        return visit_impl(
            *this, std::forward<Visitor>(vis), std::forward<Args>(args)...);
    }

private:
    template<typename Self, typename Visitor, typename... Args>
    static TIRO_FORCE_INLINE decltype(auto)
    visit_impl(Self&& self, Visitor&& vis, Args&&... args);

private:
    AstPropertyType type_;
    union {
        Field field_;
        TupleField tuple_field_;
    };
};
// [[[end]]]

/* [[[cog
    from codegen.unions import implement_inlines
    from codegen.ast import Property
    implement_inlines(Property)
]]] */
template<typename Self, typename Visitor, typename... Args>
decltype(auto)
AstProperty::visit_impl(Self&& self, Visitor&& vis, Args&&... args) {
    switch (self.type()) {
    case AstPropertyType::Field:
        return vis.visit_field(self.field_, std::forward<Args>(args)...);
    case AstPropertyType::TupleField:
        return vis.visit_tuple_field(
            self.tuple_field_, std::forward<Args>(args)...);
    }
    TIRO_UNREACHABLE("Invalid AstProperty type.");
}
// [[[end]]]

} // namespace tiro

TIRO_ENABLE_FREE_FORMAT(tiro::AstNodeFlags);
TIRO_ENABLE_FREE_TO_STRING(tiro::AstNodeType);
TIRO_ENABLE_FREE_TO_STRING(tiro::AccessType);
TIRO_ENABLE_FREE_TO_STRING(tiro::AstPropertyType);

#endif // TIRO_AST_NODE_HPP
