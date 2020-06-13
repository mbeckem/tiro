#ifndef TIRO_COMPILER_IR_FWD_HPP
#define TIRO_COMPILER_IR_FWD_HPP

#include "common/defs.hpp"

namespace tiro {

class Module;

enum class ModuleMemberType : u8;
class ModuleMemberId;
class ModuleMemberData;
class ModuleMember;

enum class FunctionType : u8;
class FunctionId;
class Function;

class BlockId;
class Block;

class ParamId;
class Param;

enum class LocalType : u8;
class LocalId;
class Local;

class PhiId;
class Phi;

enum class LValueType : u8;
class LValue;

enum class ConstantType : u8;
class Constant;

enum class AggregateType : u8;
class Aggregate;

enum class AggregateMember : u8;

enum class RValueType : u8;
class RValue;

class LocalListId;
class LocalList;

enum class TerminatorType : u8;
enum class BranchType : u8;
class Terminator;

enum class StmtType : u8;
class Stmt;

enum class BinaryOpType : u8;
enum class UnaryOpType : u8;
enum class ContainerType : u8;

} // namespace tiro

#endif // TIRO_COMPILER_IR_FWD_HPP
