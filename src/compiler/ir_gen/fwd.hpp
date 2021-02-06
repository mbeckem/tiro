#ifndef TIRO_COMPILER_IR_GEN_FWD_HPP
#define TIRO_COMPILER_IR_GEN_FWD_HPP

#include "common/defs.hpp"
#include "compiler/ir/fwd.hpp"

namespace tiro::ir {

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

enum class AssignTargetType : u8;
class AssignTarget;

class RegionId;
enum class RegionType : u8;
class Region;

class CurrentBlock;
class Unreachable;
class Ok;
class Failure;

enum class TransformResultType : u8;

template<typename T>
class TransformResult;

/// The result of compiling an expression.
/// Note: invalid (i.e. default constructed) InstIds are not an error: they are used to indicate
/// expressions that do not have a result (-> BlockExpressions in statement context or as function body).
using InstResult = TransformResult<ir::InstId>;

/// The result of compiling a statement.
using OkResult = TransformResult<Ok>;

struct EnvContext;

enum class ExprOptions : int;

enum class Unsupported : u8;
class EvalResult;

} // namespace tiro::ir

#endif // TIRO_COMPILER_IR_GEN_FWD_HPP
