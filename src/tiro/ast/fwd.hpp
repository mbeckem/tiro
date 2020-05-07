#ifndef TIRO_AST_FWD_HPP
#define TIRO_AST_FWD_HPP

#include "tiro/core/defs.hpp"

#include <memory>

namespace tiro {

// TODO: Support for allocators / arenas.
template<typename T>
class AstPtr;

template<typename T>
struct AstPtrDeleter;

class AstNodeBase;

class AstFile;
class AstParamDecl;
class AstFuncDecl;

enum class AstItemType : u8;
class AstItemData;
class AstItem;

enum class AstBindingType : u8;
class AstBindingData;
class AstBinding;

enum class AstStmtType : u8;
class AstStmtData;
class AstStmt;

enum class AstExprType : u8;
class AstExprData;
class AstExpr;

enum class AstPropertyType : u8;
class AstPropertyData;
class AstProperty;

enum class TokenDataType : u8;
class TokenData;

enum class TokenType : u8;
class Token;

enum class AccessType : u8;
enum class UnaryOperator : u8;
enum class BinaryOperator : u8;

} // namespace tiro

#endif // TIRO_AST_FWD_HPP
