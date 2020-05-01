#ifndef TIRO_AST_FWD_HPP
#define TIRO_AST_FWD_HPP

#include "tiro/core/defs.hpp"

#include <memory>

namespace tiro {

// TODO: Support for allocators / arenas.
template<typename T>
using ASTPtr = std::unique_ptr<T>;

enum class ASTStmtType : u8;
class ASTStmtData;
class ASTStmt;

enum class ASTDeclType : u8;
class ASTDeclData;
class ASTDecl;

enum class ASTExprType : u8;
class ASTExprData;
class ASTExpr;

enum class AccessType : u8;

} // namespace tiro

#endif // TIRO_AST_FWD_HPP
