#ifndef TIRO_MIR_TRANSFORM_FUNC_HPP
#define TIRO_MIR_TRANSFORM_FUNC_HPP

#include "tiro/core/ref_counted.hpp"
#include "tiro/core/safe_int.hpp"
#include "tiro/mir/closures.hpp"
#include "tiro/mir/fwd.hpp"
#include "tiro/mir/types.hpp"
#include "tiro/syntax/ast.hpp" // TODO ast fwd

#include <memory>
#include <optional>
#include <queue>

namespace tiro::compiler::mir_transform {

/// Represents the fact that control flow terminated with the compilation
/// of the last statement or expression.
class Unreachable {};

inline constexpr Unreachable unreachable;

class Ok {};

inline constexpr Ok ok;

enum class TransformResultType : u8 { Value, Unreachable };

class Failure {
public:
    Failure(Unreachable)
        : type_(TransformResultType::Unreachable) {}

    explicit Failure(TransformResultType type)
        : type_(type) {
        TIRO_ASSERT(
            type_ != TransformResultType::Value, "Must not represent a value.");
    }

    TransformResultType type() const noexcept { return type_; }

private:
    TransformResultType type_;
};

template<typename T>
class [[nodiscard]] TransformResult final {
public:
    TransformResult(const T& value)
        : type_(TransformResultType::Value)
        , value_(value) {}

    TransformResult(T && value)
        : type_(TransformResultType::Value)
        , value_(std::move(value)) {}

    TransformResult(Failure failure)
        : type_(failure.type()) {}

    TransformResult(Unreachable)
        : type_(TransformResultType::Unreachable) {}

    const T& value() const {
        TIRO_ASSERT(is_value(), "TransformResult is not a value.");
        TIRO_ASSERT(
            value_, "Optional must hold a value if is_value() is true.");
        return *value_;
    }

    const T& operator*() const { return value(); }

    TransformResultType type() const noexcept { return type_; }

    bool is_value() const noexcept {
        return type_ == TransformResultType::Value;
    }

    bool is_unreachable() const noexcept {
        return type_ == TransformResultType::Unreachable;
    }

    Failure failure() const {
        TIRO_ASSERT(!is_value(), "Result must not hold a value.");
        return Failure(type_);
    }

    explicit operator bool() const noexcept { return is_value(); }

private:
    TransformResultType type_;
    std::optional<T> value_;
};

/// The result of compiling an expression.
/// Note: invalid (i.e. default constructed) LocalIDs are used to indicate
/// expressions that do not have a result (-> BlockExpressions in statement context or as function body).
/// This is not an error.
using ExprResult = TransformResult<mir::LocalID>;

/// The result of compiling a statement.
using StmtResult = TransformResult<Ok>;

/// Represents an active loop. The blocks inside this structure can be used
/// to jump to the end or the start of the loop (used when compiling break and continue expressions).
struct LoopContext {
    mir::BlockID jump_break;
    mir::BlockID jump_continue;
};

struct EnvContext {
    ClosureEnvID env;
    NotNull<Scope*> starter;
};

/// Compilation options for expressions.
enum class ExprOptions : int {
    Default = 0,

    /// May return an invalid local id (-> disables the debug assertion)
    MaybeInvalid = 1 << 0,
};

inline ExprOptions operator|(ExprOptions lhs, ExprOptions rhs) {
    return static_cast<ExprOptions>(
        static_cast<int>(lhs) | static_cast<int>(rhs));
}

inline ExprOptions operator&(ExprOptions lhs, ExprOptions rhs) {
    return static_cast<ExprOptions>(
        static_cast<int>(lhs) & static_cast<int>(rhs));
}

inline bool has_options(ExprOptions options, ExprOptions test) {
    return static_cast<int>(options & test) != 0;
}

class CurrentBlock final {
public:
    CurrentBlock(FunctionContext& ctx, mir::BlockID id)
        : ctx_(ctx)
        , id_(id) {
        TIRO_ASSERT(id, "Invalid block id.");
    }

    CurrentBlock(const CurrentBlock&) = delete;
    CurrentBlock& operator=(const CurrentBlock&) = delete;

    void assign(mir::BlockID id) {
        TIRO_ASSERT(id, "Invalid block id.");
        id_ = id;
    }

    FunctionContext& ctx() const { return ctx_; }
    mir::BlockID id() const { return id_; }

    ExprResult compile_expr(
        NotNull<Expr*> expr, ExprOptions options = ExprOptions::Default);

    StmtResult compile_stmt(NotNull<Stmt*> stmt);

    StmtResult
    compile_loop_body(NotNull<Expr*> body, NotNull<Scope*> loop_scope,
        mir::BlockID breakID, mir::BlockID continueID);

    mir::LocalID compile_reference(NotNull<Symbol*> symbol);

    void compile_assign(NotNull<Symbol*> symbol, mir::LocalID value);

    mir::LocalID compile_env(ClosureEnvID env);

    mir::LocalID define(const mir::Local& local);
    void seal();
    void emit(const mir::Stmt& stmt);
    void end(const mir::Edge& edge);

private:
    FunctionContext& ctx_;
    mir::BlockID id_;
};

/// Context object for function transformations.
///
/// The SSA transformation (AST -> MIR) in this module is done using the algorithms described in
///
///  [BB+13] Braun M., Buchwald S., Hack S., Leißa R., Mallon C., Zwinkau A. (2013):
///              Simple and Efficient Construction of Static Single Assignment Form.
///          In: Jhala R., De Bosschere K. (eds) Compiler Construction. CC 2013. Lecture Notes in Computer Science, vol 7791.
///          Springer, Berlin, Heidelberg
class FunctionContext final {
public:
    explicit FunctionContext(ModuleContext& module,
        NotNull<ClosureEnvCollection*> envs, ClosureEnvID closure_env,
        mir::Function& result, StringTable& strings);

    ModuleContext& module() const { return module_; }
    StringTable& strings() const { return strings_; }
    mir::Function& result() const { return result_; }
    NotNull<ClosureEnvCollection*> envs() const { return TIRO_NN(envs_.get()); }

    const LoopContext* current_loop() const;

    ClosureEnvID current_env() const;

    void compile_function(NotNull<FuncDecl*> func);

    /// Compiles the given expression. May not return a value (e.g. unreachable).
    ExprResult compile_expr(NotNull<Expr*> expr, CurrentBlock& bb,
        ExprOptions options = ExprOptions::Default);

    /// Compiles the given statement. Returns false if the statement terminated control flow, i.e.
    /// if the following code would be unreachable.
    StmtResult compile_stmt(NotNull<Stmt*> stmt, CurrentBlock& bb);

    /// Compites the given loop body. Automatically arranges for a loop context to be pushed
    /// (and popped) from the loop stack.
    /// The loop scope is needed to create a new nested closure environment if neccessary.
    StmtResult
    compile_loop_body(NotNull<Expr*> body, NotNull<Scope*> loop_scope,
        mir::BlockID breakID, mir::BlockID continueID, CurrentBlock& bb);

    /// Compiles code that derefences the given symbol.
    mir::LocalID compile_reference(NotNull<Symbol*> symbol, mir::BlockID block);

    /// Generates code that assigns the given value to the symbol.
    void compile_assign(
        NotNull<Symbol*> symbol, mir::LocalID value, mir::BlockID blockID);

    /// Compiles a reference to the given closure environment, usually for the purpose of creating
    /// a closure function object.
    mir::LocalID compile_env(ClosureEnvID env, mir::BlockID block);

    /// Returns a new CurrentBlock instance that references this context.
    CurrentBlock make_current(mir::BlockID blockID) { return {*this, blockID}; }

    /// Create a new block. Blocks must be sealed after all predecessor nodes have been linked.
    mir::BlockID make_block(InternedString label);

    /// Defines a new local variable in the given block and returns its id.
    /// Performs on the fly copy propagation.
    // TODO: more ad hoc optimizations like value numbering, constant folding and cse.
    // TODO: Infer the current scope and only take the rvalue as an argument.
    // FIXE: Make this private and provide optimizing public functions (for every rvalue type):
    mir::LocalID define(const mir::Local& local, mir::BlockID blockID);

    /// Seals the given block after all possible predecessors have been linked to it.
    /// Only when a block is sealed can we analyze the completed (nested) control flow graph.
    /// It is an error when a block is left unsealed.
    void seal(mir::BlockID blockID);

    /// Emits a new statement into the given block.
    /// Must not be called if the block has already been filled.
    void emit(const mir::Stmt& stmt, mir::BlockID blockID);

    /// Ends the block by settings outgoing edge. The block automatically becomes filled.
    void end(const mir::Edge& edge, mir::BlockID blockID);

private:
    /// Associates the given variable with its current value in the given basic block.
    void write_variable(
        NotNull<Symbol*> var, mir::LocalID value, mir::BlockID blockID);

    /// Returns the current SSA value for the given variable in the given block.
    mir::LocalID read_variable(NotNull<Symbol*> var, mir::BlockID blockID);

    /// Recursive resolution algorithm for variables. See Algorithm 2 in [BB+13].
    mir::LocalID
    read_variable_recursive(NotNull<Symbol*> var, mir::BlockID blockID);

    void add_phi_operands(
        NotNull<Symbol*> var, mir::LocalID value, mir::BlockID blockID);

    /// Analyze the scopes reachable from `scope` until a loop scope or nested function
    /// scope is encountered. All captured variables declared within these scopes are grouped
    /// together into the same closure environment.
    ///
    /// \pre `scope` must be either a loop or a function scope.
    void enter_env(NotNull<Scope*> scope, CurrentBlock& bb);
    void exit_env(NotNull<Scope*> scope);

    /// Returns the runtime location of the given closure environment.
    std::optional<mir::LocalID> find_env(ClosureEnvID env);

    /// Like find_env(), but fails with an assertion error if the environment was not found.
    mir::LocalID get_env(ClosureEnvID env);

    /// Lookup the given symbol as an lvalue of non-local type.
    /// Returns an empty optional if the symbol does not qualify (lookup as local instead).
    std::optional<mir::LValue> find_lvalue(NotNull<Symbol*> symbol);

    /// Returns an lvalue for accessing the given closure env location.
    mir::LValue get_captured_lvalue(const ClosureEnvLocation& loc);

private:
    // TODO: Better map implementation
    using VariableMap = std::unordered_map<std::tuple<Symbol*, mir::BlockID>,
        mir::LocalID, UseHasher>;

    // Represents an incomplete phi nodes. These are cleaned up when a block is sealed.
    // Only incomplete control flow graphs (i.e. loops) can produce incomplete phi nodes.
    using IncompletePhi = std::tuple<NotNull<Symbol*>, mir::LocalID>;

    // TODO: Better container.
    using IncompletePhiMap =
        std::unordered_map<mir::BlockID, std::vector<IncompletePhi>, UseHasher>;

private:
    ModuleContext& module_;
    Ref<ClosureEnvCollection> envs_; // Init at top level, never null.
    ClosureEnvID outer_env_;         // Optional
    mir::Function& result_;
    StringTable& strings_;

    // Tracks active loops. The last context represents the innermost loop.
    std::vector<LoopContext> active_loops_;

    // Tracks active closure environments. The last context represents the innermost environment.
    std::vector<EnvContext> local_env_stack_;

    // Supports global value numbering in the function. This map holds the current value
    // for each variable declaration and block.
    VariableMap variables_;

    // Represents the set of pending incomplete phi variables.
    IncompletePhiMap incomplete_phis_;

    // Maps closure environments to the ssa local that references their runtime representation.
    // TODO: Better map implementation
    std::unordered_map<ClosureEnvID, mir::LocalID, UseHasher>
        local_env_locations_;
};

/// Base class for transformers.
/// TODO: Get rid of the implicit basic block parameter?
class Transformer {
protected:
    Transformer(FunctionContext& ctx, CurrentBlock& bb)
        : ctx_(ctx)
        , bb_(bb) {}

    StringTable& strings() const { return ctx_.strings(); }
    mir::Function& result() const { return ctx_.result(); }
    FunctionContext& ctx() const { return ctx_; }
    CurrentBlock& bb() const { return bb_; }

    const LoopContext* current_loop() const { return ctx_.current_loop(); }

private:
    FunctionContext& ctx_;
    CurrentBlock& bb_;
};

} // namespace tiro::compiler::mir_transform

#endif // TIRO_MIR_TRANSFORM_FUNC_HPP
