#ifndef TIRO_MIR_FWD_HPP
#define TIRO_MIR_FWD_HPP

#include "tiro/core/defs.hpp"

namespace tiro::mir {

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

} // namespace tiro::mir

namespace tiro {

struct ClosureEnvLocation;
class ClosureEnvID;
class ClosureEnv;
class ClosureEnvCollection;

class Transformer;
class ExprTransformer;
class StmtTransformer;

enum class ComputedValueType : u8;
class ComputedValue;
class RValueCompiler;

class CurrentBlock;
class FunctionContext;
class ModuleContext;

class Unreachable;
class Ok;
class Failure;

enum class TransformResultType : u8;

template<typename T>
class TransformResult;

using ExprResult = TransformResult<mir::LocalID>;
using StmtResult = TransformResult<Ok>;

struct LoopContext;
struct EnvContext;

enum class ExprOptions : int;

enum class Unsupported : u8;
class EvalResult;

class LocalVisitor;

} // namespace tiro

#endif // TIRO_MIR_FWD_HPP
