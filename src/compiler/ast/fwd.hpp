#ifndef TIRO_COMPILER_AST_FWD_HPP
#define TIRO_COMPILER_AST_FWD_HPP

#include "common/defs.hpp"

#include <memory>

namespace tiro {

enum class TokenDataType : u8;
enum class TokenType : u8;
class TokenData;
class Token;
class TokenTypes;

enum class AccessType : u8;
enum class UnaryOperator : u8;
enum class BinaryOperator : u8;

enum class AstNodeType : u8;
class AstId;
class AstNodeMap;
class MutableAstVisitor;

template<typename T>
using AstPtr = std::unique_ptr<T>;

template<typename T>
struct AstNodeTraits;

template<typename T>
class AstNodeList;

/* [[[cog
    import cog
    from codegen.ast import walk_types
    cpp_names = sorted(node_type.cpp_name for node_type in walk_types())
    for cpp_name in cpp_names:
        cog.outl(f"class {cpp_name};");
]]] */
class AstArrayLiteral;
class AstAssertStmt;
class AstBinaryExpr;
class AstBinding;
class AstBlockExpr;
class AstBooleanLiteral;
class AstBreakExpr;
class AstCallExpr;
class AstContinueExpr;
class AstDecl;
class AstElementExpr;
class AstEmptyItem;
class AstEmptyStmt;
class AstExportItem;
class AstExpr;
class AstExprStmt;
class AstFile;
class AstFloatLiteral;
class AstForStmt;
class AstFuncDecl;
class AstFuncExpr;
class AstFuncItem;
class AstIdentifier;
class AstIfExpr;
class AstImportItem;
class AstIntegerLiteral;
class AstItem;
class AstLiteral;
class AstMapItem;
class AstMapLiteral;
class AstNode;
class AstNullLiteral;
class AstNumericIdentifier;
class AstParamDecl;
class AstPropertyExpr;
class AstReturnExpr;
class AstSetLiteral;
class AstStmt;
class AstStringExpr;
class AstStringGroupExpr;
class AstStringIdentifier;
class AstStringLiteral;
class AstSymbolLiteral;
class AstTupleBinding;
class AstTupleLiteral;
class AstUnaryExpr;
class AstVarBinding;
class AstVarDecl;
class AstVarExpr;
class AstVarItem;
class AstVarStmt;
class AstWhileStmt;
// [[[end]]]

} // namespace tiro

#endif // TIRO_COMPILER_AST_FWD_HPP