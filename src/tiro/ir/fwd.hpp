#ifndef TIRO_IRFWD_HPP
#define TIRO_IRFWD_HPP

#include "tiro/core/defs.hpp"

namespace tiro {

class Module;

enum class ModuleMemberType : u8;
class ModuleMemberID;
class ModuleMember;

enum class FunctionType : u8;
class FunctionID;
class Function;

class BlockID;
class Block;

class ParamID;
class Param;

enum class LocalType : u8;
class LocalID;
class Local;

class PhiID;
class Phi;

enum class LValueType : u8;
class LValue;

enum class ConstantType : u8;
class Constant;

enum class RValueType : u8;
class RValue;

class LocalListID;
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

namespace tiro {

class LocalVisitor;

} // namespace tiro

#endif // TIRO_IRFWD_HPP
