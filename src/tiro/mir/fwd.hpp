#ifndef TIRO_MIR_FWD_HPP
#define TIRO_MIR_FWD_HPP

#include "tiro/core/defs.hpp"

namespace tiro::compiler::mir {

class Function;

class BlockID;
class Block;

class ScopeID;
class Scope;

class ParamID;
class Param;

enum class LocalType : u8;
class LocalID;
class Local;

enum class LValueType : u8;
class LValue;

enum class ConstantType : u8;
class Constant;

enum class RValueType : u8;
class RValue;

class LocalListID;
class LocalList;

enum class EdgeType : u8;
enum class BranchType : u8;
class Edge;

enum class StmtType : u8;
class Stmt;

enum class BinaryOpType : u8;
enum class UnaryOpType : u8;
enum class ContainerType : u8;

} // namespace tiro::compiler::mir

#endif // TIRO_MIR_FWD_HPP
