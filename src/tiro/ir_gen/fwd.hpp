#ifndef TIRO_IR_GEN_FWD_HPP
#define TIRO_IR_GEN_FWD_HPP

#include "tiro/core/defs.hpp"
#include "tiro/ir/fwd.hpp"

namespace tiro {

struct ClosureEnvLocation;
class ClosureEnvId;
class ClosureEnv;
class ClosureEnvCollection;

class Transformer;
class RValueIRGen;
class ExprIRGen;
class StmtIRGen;
class FunctionIRGen;
class ModuleIRGen;

enum class ComputedValueType : u8;
class ComputedValue;

class CurrentBlock;
class Unreachable;
class Ok;
class Failure;

enum class TransformResultType : u8;

template<typename T>
class TransformResult;

using ExprResult = TransformResult<LocalId>;
using StmtResult = TransformResult<Ok>;

struct LoopContext;
struct EnvContext;

enum class ExprOptions : int;

enum class Unsupported : u8;
class EvalResult;

} // namespace tiro

#endif // TIRO_IR_GEN_FWD_HPP
