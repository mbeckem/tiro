#include "tiro/ast/ast.hpp"

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
        TIRO_CASE(EmptyItem)
        TIRO_CASE(EmptyStmt)
        TIRO_CASE(ExprStmt)
        TIRO_CASE(File)
        TIRO_CASE(FloatLiteral)
        TIRO_CASE(ForStmt)
        TIRO_CASE(FuncDecl)
        TIRO_CASE(FuncExpr)
        TIRO_CASE(FuncItem)
        TIRO_CASE(IfExpr)
        TIRO_CASE(ImportItem)
        TIRO_CASE(IntegerLiteral)
        TIRO_CASE(MapItem)
        TIRO_CASE(MapLiteral)
        TIRO_CASE(NullLiteral)
        TIRO_CASE(NumericIdentifier)
        TIRO_CASE(ParamDecl)
        TIRO_CASE(PropertyExpr)
        TIRO_CASE(ReturnExpr)
        TIRO_CASE(SetLiteral)
        TIRO_CASE(StringExpr)
        TIRO_CASE(StringIdentifier)
        TIRO_CASE(StringLiteral)
        TIRO_CASE(SymbolLiteral)
        TIRO_CASE(TupleBinding)
        TIRO_CASE(TupleLiteral)
        TIRO_CASE(UnaryExpr)
        TIRO_CASE(VarBinding)
        TIRO_CASE(VarDecl)
        TIRO_CASE(VarExpr)
        TIRO_CASE(VarItem)
        TIRO_CASE(VarStmt)
        TIRO_CASE(WhileStmt)
        // [[[end]]]

#undef TIRO_CASE
    }
    TIRO_UNREACHABLE("Invalid node type.");
}

void format(AstNodeFlags flags, FormatStream& stream) {
    bool first = true;
    auto format_flag = [&](AstNodeFlags value, const char* name) {
        if ((flags & value) != AstNodeFlags::None) {
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

} // namespace tiro
