#ifndef TIRO_COMPILER_IR_GEN_FWD_HPP
#define TIRO_COMPILER_IR_GEN_FWD_HPP

#include "common/defs.hpp"
#include "compiler/ir/fwd.hpp"

namespace tiro {

struct ModuleContext;
struct FunctionContext;

struct ClosureEnvLocation;
class ClosureEnvId;
class ClosureEnv;
class ClosureEnvCollection;

class Transformer;
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

using LocalResult = TransformResult<LocalId>;
using OkResult = TransformResult<Ok>;

struct LoopContext;
struct EnvContext;

enum class ExprOptions : int;

enum class Unsupported : u8;
class EvalResult;

} // namespace tiro

#endif // TIRO_COMPILER_IR_GEN_FWD_HPP
