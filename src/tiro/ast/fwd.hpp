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

enum class AstStmtType : u8;
class AstStmtData;
class AstStmt;

enum class AstDeclType : u8;
class AstDeclData;
class AstDecl;

enum class AstExprType : u8;
class AstExprData;
class AstExpr;

enum class AstPropertyType : u8;
class AstProperty;

enum class AccessType : u8;
enum class UnaryOperator : u8;
enum class BinaryOperator : u8;

} // namespace tiro

#endif // TIRO_AST_FWD_HPP
