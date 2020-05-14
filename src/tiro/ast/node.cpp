#include "tiro/ast/node.hpp"

#include "tiro/core/safe_int.hpp"

#include <new>

namespace tiro {

std::string_view to_string(AstNodeType type) {
    switch (type) {
#define TIRO_CASE(Type)     \
    case AstNodeType::Type: \
        return #Type;

        /* [[[cog
            import cog
            from codegen.ast import walk_concrete_types
            names = sorted(node_type.name for node_type in walk_concrete_types())
            for name in names:
                cog.outl(f"TIRO_CASE({name})");
        ]]] */
        TIRO_CASE(ArrayLiteral)
        TIRO_CASE(AssertStmt)
        TIRO_CASE(BinaryExpr)
        TIRO_CASE(BlockExpr)
        TIRO_CASE(BooleanLiteral)
        TIRO_CASE(BreakExpr)
        TIRO_CASE(CallExpr)
        TIRO_CASE(ContinueExpr)
        TIRO_CASE(ElementExpr)
        TIRO_CASE(EmptyStmt)
        TIRO_CASE(FloatLiteral)
        TIRO_CASE(ForStmt)
        TIRO_CASE(FuncDecl)
        TIRO_CASE(FuncExpr)
        TIRO_CASE(FuncItem)
        TIRO_CASE(IfExpr)
        TIRO_CASE(ImportItem)
        TIRO_CASE(IntegerLiteral)
        TIRO_CASE(ItemStmt)
        TIRO_CASE(MapItem)
        TIRO_CASE(MapLiteral)
        TIRO_CASE(NullLiteral)
        TIRO_CASE(ParamDecl)
        TIRO_CASE(PropertyExpr)
        TIRO_CASE(ReturnExpr)
        TIRO_CASE(SetLiteral)
        TIRO_CASE(StringExpr)
        TIRO_CASE(StringLiteral)
        TIRO_CASE(SymbolLiteral)
        TIRO_CASE(TupleBinding)
        TIRO_CASE(TupleLiteral)
        TIRO_CASE(UnaryExpr)
        TIRO_CASE(VarBinding)
        TIRO_CASE(VarDecl)
        TIRO_CASE(VarExpr)
        TIRO_CASE(VarItem)
        TIRO_CASE(WhileStmt)
        // [[[end]]]

#undef TIRO_CASE
    }
    TIRO_UNREACHABLE("Invalid node type.");
}

AstNodeFlags operator|(AstNodeFlags lhs, AstNodeFlags rhs) {
    using U = std::underlying_type_t<AstNodeFlags>;
    return static_cast<AstNodeFlags>(static_cast<U>(lhs) | static_cast<U>(rhs));
}

AstNodeFlags operator&(AstNodeFlags lhs, AstNodeFlags rhs) {
    using U = std::underlying_type_t<AstNodeFlags>;
    return static_cast<AstNodeFlags>(static_cast<U>(lhs) & static_cast<U>(rhs));
}

bool test(AstNodeFlags flags, AstNodeFlags test) {
    using U = std::underlying_type_t<AstNodeFlags>;
    return static_cast<U>(flags & test) != 0;
}

void format(FormatStream& stream, AstNodeFlags flags) {
    bool first = true;
    auto format_flag = [&](AstNodeFlags value, const char* name) {
        if (test(flags, value)) {
            if (first) {
                stream.format(", ");
                first = false;
            }
            stream.format("{}", name);
        }
    };

#define TIRO_FLAG(Name) (format_flag(AstNodeFlags::Name, #Name))

    stream.format("(");
    TIRO_FLAG(HasError);
    stream.format(")");

#undef TIRO_FLAG
}

AstNode::AstNode(AstNodeType type)
    : type_(type) {
    TIRO_DEBUG_ASSERT(
        type >= AstNodeType::FirstNode && type <= AstNodeType::LastNode,
        "Invalid node type.");
}

AstNode::~AstNode() = default;

std::string_view to_string(AccessType access) {
    switch (access) {
    case AccessType::Normal:
        return "Normal";
    case AccessType::Optional:
        return "Optional";
    }
    TIRO_UNREACHABLE("Invalid access type.");
}

/* [[[cog
    from codegen.unions import implement
    from codegen.ast import Property
    implement(Property.tag, Property)
]]] */
std::string_view to_string(AstPropertyType type) {
    switch (type) {
    case AstPropertyType::Field:
        return "Field";
    case AstPropertyType::TupleField:
        return "TupleField";
    }
    TIRO_UNREACHABLE("Invalid AstPropertyType.");
}

AstProperty AstProperty::make_field(const InternedString& name) {
    return {Field{name}};
}

AstProperty AstProperty::make_tuple_field(const u32& index) {
    return {TupleField{index}};
}

AstProperty::AstProperty(Field field)
    : type_(AstPropertyType::Field)
    , field_(std::move(field)) {}

AstProperty::AstProperty(TupleField tuple_field)
    : type_(AstPropertyType::TupleField)
    , tuple_field_(std::move(tuple_field)) {}

const AstProperty::Field& AstProperty::as_field() const {
    TIRO_DEBUG_ASSERT(type_ == AstPropertyType::Field,
        "Bad member access on AstProperty: not a Field.");
    return field_;
}

const AstProperty::TupleField& AstProperty::as_tuple_field() const {
    TIRO_DEBUG_ASSERT(type_ == AstPropertyType::TupleField,
        "Bad member access on AstProperty: not a TupleField.");
    return tuple_field_;
}

// [[[end]]]

} // namespace tiro
