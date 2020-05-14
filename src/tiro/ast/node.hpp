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
    BinaryExpr = 3,
    BlockExpr = 4,
    BreakExpr = 5,
    CallExpr = 6,
    ContinueExpr = 7,
    ElementExpr = 8,
    FuncExpr = 9,
    IfExpr = 10,
    ArrayLiteral = 11,
    BooleanLiteral = 12,
    FloatLiteral = 13,
    IntegerLiteral = 14,
    MapLiteral = 15,
    NullLiteral = 16,
    SetLiteral = 17,
    StringLiteral = 18,
    SymbolLiteral = 19,
    TupleLiteral = 20,
    FirstLiteral = 11,
    LastLiteral = 20,
    PropertyExpr = 21,
    ReturnExpr = 22,
    StringExpr = 23,
    UnaryExpr = 24,
    VarExpr = 25,
    FirstExpr = 3,
    LastExpr = 25,
    FuncDecl = 26,
    FuncItem = 27,
    ImportItem = 28,
    VarItem = 29,
    FirstItem = 27,
    LastItem = 29,
    MapItem = 30,
    ParamDecl = 31,
    AssertStmt = 32,
    EmptyStmt = 33,
    ForStmt = 34,
    ItemStmt = 35,
    WhileStmt = 36,
    FirstStmt = 32,
    LastStmt = 36,
    FirstNode = 1,
    LastNode = 36,
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

class AstNode {
public:
    virtual ~AstNode();

    AstNode(const AstNode&) = delete;
    AstNode& operator=(const AstNode&) = delete;

    AstId id() const { return id_; }
    void id(AstId new_id) { id_ = new_id; }

    SourceReference source() const { return source_; }
    void source(const SourceReference& new_source) { source_ = new_source; }

    AstNodeFlags flags() const { return flags_; }
    void flags(AstNodeFlags new_flags) { flags_ = new_flags; }

    AstNodeType type() const { return type_; }

protected:
    explicit AstNode(AstNodeType type);

private:
    const AstNodeType type_;
    AstId id_;
    SourceReference source_;
    AstNodeFlags flags_ = AstNodeFlags::None;
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
