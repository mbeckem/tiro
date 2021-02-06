#ifndef TIRO_COMPILER_IR_FWD_HPP
#define TIRO_COMPILER_IR_FWD_HPP

#include "common/defs.hpp"

namespace tiro::ir {

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

class InstId;
class Inst;

class Phi;

enum class LValueType : u8;
class LValue;

enum class ConstantType : u8;
class Constant;

enum class AggregateType : u8;
class Aggregate;

enum class AggregateMember : u8;

enum class ValueType : u8;
class Value;

class LocalListId;
class LocalList;

class RecordId;
class Record;

enum class TerminatorType : u8;
enum class BranchType : u8;
class Terminator;

enum class BinaryOpType : u8;
enum class UnaryOpType : u8;
enum class ContainerType : u8;

} // namespace tiro::ir

#endif // TIRO_COMPILER_IR_FWD_HPP
