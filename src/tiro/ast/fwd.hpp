#ifndef TIRO_AST_FWD_HPP
#define TIRO_AST_FWD_HPP

#include "tiro/core/defs.hpp"

#include <memory>

namespace tiro {

enum class AstNodeType : u8;

class AstId;

enum class AstPropertyType : u8;
class AstProperty;

enum class TokenDataType : u8;
class TokenData;

enum class TokenType : u8;
class Token;

class TokenTypes;

enum class AccessType : u8;
enum class UnaryOperator : u8;
enum class BinaryOperator : u8;

template<typename T>
class AstPtr;

template<typename T>
struct AstPtrDeleter;

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
class AstElementExpr;
class AstEmptyStmt;
class AstExpr;
class AstFloatLiteral;
class AstForStmt;
class AstFuncDecl;
class AstFuncExpr;
class AstFuncItem;
class AstIfExpr;
class AstImportItem;
class AstIntegerLiteral;
class AstItem;
class AstItemStmt;
class AstLiteral;
class AstMapItem;
class AstMapLiteral;
class AstNode;
class AstNullLiteral;
class AstParamDecl;
class AstPropertyExpr;
class AstReturnExpr;
class AstSetLiteral;
class AstStmt;
class AstStringExpr;
class AstStringLiteral;
class AstSymbolLiteral;
class AstTupleBinding;
class AstTupleLiteral;
class AstUnaryExpr;
class AstVarBinding;
class AstVarExpr;
class AstVarItem;
class AstWhileStmt;
// [[[end]]]

} // namespace tiro

#endif // TIRO_AST_FWD_HPP
