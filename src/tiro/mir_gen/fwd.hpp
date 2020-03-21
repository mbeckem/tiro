#ifndef TIRO_MIR_GEN_FWD_HPP
#define TIRO_MIR_GEN_FWD_HPP

#include "tiro/core/defs.hpp"
#include "tiro/mir/fwd.hpp"

namespace tiro {

struct ClosureEnvLocation;
class ClosureEnvID;
class ClosureEnv;
class ClosureEnvCollection;

class Transformer;
class RValueMIRGen;
class ExprMIRGen;
class StmtMIRGen;
class FunctionMIRGen;
class ModuleMIRGen;

enum class ComputedValueType : u8;
class ComputedValue;

class CurrentBlock;
class Unreachable;
class Ok;
class Failure;

enum class TransformResultType : u8;

template<typename T>
class TransformResult;

using ExprResult = TransformResult<LocalID>;
using StmtResult = TransformResult<Ok>;

struct LoopContext;
struct EnvContext;

enum class ExprOptions : int;

enum class Unsupported : u8;
class EvalResult;

} // namespace tiro

#endif // TIRO_MIR_GEN_FWD_HPP
