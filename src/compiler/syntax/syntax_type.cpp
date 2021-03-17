#include "compiler/syntax/syntax_type.hpp"

#include "common/assert.hpp"

namespace tiro {

std::string_view to_string(SyntaxType type) {
    switch (type) {
#define TIRO_CASE(type)    \
    case SyntaxType::type: \
        return #type;

        TIRO_CASE(Root)

        TIRO_CASE(Error)

        TIRO_CASE(File)
        TIRO_CASE(Name)
        TIRO_CASE(Literal)
        TIRO_CASE(Condition)
        TIRO_CASE(Modifiers)
        TIRO_CASE(RecordItem)
        TIRO_CASE(MapItem)

        TIRO_CASE(ReturnExpr)
        TIRO_CASE(ContinueExpr)
        TIRO_CASE(BreakExpr)
        TIRO_CASE(VarExpr)
        TIRO_CASE(UnaryExpr)
        TIRO_CASE(BinaryExpr)
        TIRO_CASE(FieldExpr)
        TIRO_CASE(TupleFieldExpr)
        TIRO_CASE(IndexExpr)
        TIRO_CASE(CallExpr)
        TIRO_CASE(GroupedExpr)
        TIRO_CASE(TupleExpr)
        TIRO_CASE(RecordExpr)
        TIRO_CASE(ArrayExpr)
        TIRO_CASE(SetExpr)
        TIRO_CASE(MapExpr)
        TIRO_CASE(IfExpr)
        TIRO_CASE(BlockExpr)
        TIRO_CASE(FuncExpr)
        TIRO_CASE(StringExpr)
        TIRO_CASE(StringFormatItem)
        TIRO_CASE(StringFormatBlock)
        TIRO_CASE(StringGroup)

        TIRO_CASE(DeferStmt)
        TIRO_CASE(AssertStmt)
        TIRO_CASE(ExprStmt)
        TIRO_CASE(VarStmt)
        TIRO_CASE(WhileStmt)
        TIRO_CASE(ForStmt)
        TIRO_CASE(ForStmtHeader)
        TIRO_CASE(ForEachStmt)

        TIRO_CASE(Var)
        TIRO_CASE(Binding)
        TIRO_CASE(BindingName)
        TIRO_CASE(BindingTuple)

        TIRO_CASE(Func)
        TIRO_CASE(ArgList)
        TIRO_CASE(ParamList)

        TIRO_CASE(ImportItem)
        TIRO_CASE(VarItem)
        TIRO_CASE(FuncItem)

#undef TIRO_CASE
    }

    TIRO_UNREACHABLE("Invalid syntax type.");
}

} // namespace tiro
