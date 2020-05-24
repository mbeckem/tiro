#ifndef TIRO_IR_GEN_GEN_FUNC_HPP
#define TIRO_IR_GEN_GEN_FUNC_HPP

#include "tiro/core/ref_counted.hpp"
#include "tiro/core/safe_int.hpp"
#include "tiro/ir/function.hpp"
#include "tiro/ir/fwd.hpp"
#include "tiro/ir_gen/closures.hpp"
#include "tiro/ir_gen/fwd.hpp"
#include "tiro/ir_gen/support.hpp"
#include "tiro/syntax/ast.hpp" // TODO ast fwd

#include <memory>
#include <optional>
#include <queue>

namespace tiro {

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
        TIRO_DEBUG_ASSERT(
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
        TIRO_DEBUG_ASSERT(is_value(), "TransformResult is not a value.");
        TIRO_DEBUG_ASSERT(
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
        TIRO_DEBUG_ASSERT(!is_value(), "Result must not hold a value.");
        return Failure(type_);
    }

    explicit operator bool() const noexcept { return is_value(); }

private:
    TransformResultType type_;
    std::optional<T> value_;
};

/// The result of compiling an expression.
/// Note: invalid (i.e. default constructed) LocalIds are not an error: they are used to indicate
/// expressions that do not have a result (-> BlockExpressions in statement context or as function body).
using ExprResult = TransformResult<LocalId>;

/// The result of compiling a statement.
using StmtResult = TransformResult<Ok>;

/// Represents an active loop. The blocks inside this structure can be used
/// to jump to the end or the start of the loop (used when compiling break and continue expressions).
struct LoopContext {
    BlockId jump_break;
    BlockId jump_continue;
};

struct EnvContext {
    ClosureEnvId env;
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
    CurrentBlock(FunctionIRGen& ctx, BlockId id)
        : ctx_(ctx)
        , id_(id) {
        TIRO_DEBUG_ASSERT(id, "Invalid block id.");
    }

    CurrentBlock(const CurrentBlock&) = delete;
    CurrentBlock& operator=(const CurrentBlock&) = delete;

    void assign(BlockId id) {
        TIRO_DEBUG_ASSERT(id, "Invalid block id.");
        id_ = id;
    }

    FunctionIRGen& ctx() const { return ctx_; }
    BlockId id() const { return id_; }

    ExprResult compile_expr(
        NotNull<Expr*> expr, ExprOptions options = ExprOptions::Default);

    StmtResult compile_stmt(NotNull<ASTStmt*> stmt);

    StmtResult compile_loop_body(NotNull<Expr*> body,
        NotNull<Scope*> loop_scope, BlockId break_id, BlockId continue_id);

    LocalId compile_reference(NotNull<Symbol*> symbol);

    void compile_assign(const AssignTarget& target, LocalId value);

    void compile_assign(NotNull<Symbol*> symbol, LocalId value);

    void compile_assign(const LValue& lvalue, LocalId value);

    LocalId compile_env(ClosureEnvId env);

    LocalId compile_rvalue(const RValue& value);

    LocalId define_new(const RValue& value);

    LocalId
    memoize_value(const ComputedValue& key, FunctionRef<LocalId()> compute);

    void seal();
    void end(const Terminator& term);

private:
    FunctionIRGen& ctx_;
    BlockId id_;
};

/// Context object for function transformations.
///
/// The SSA transformation (AST -> IR) in this module is done using the algorithms described in
///
///  [BB+13] Braun M., Buchwald S., Hack S., Leißa R., Mallon C., Zwinkau A. (2013):
///              Simple and Efficient Construction of Static Single Assignment Form.
///          In: Jhala R., De Bosschere K. (eds) Compiler Construction. CC 2013. Lecture Notes in Computer Science, vol 7791.
///          Springer, Berlin, Heidelberg
class FunctionIRGen final {
public:
    explicit FunctionIRGen(ModuleIRGen& module,
        NotNull<ClosureEnvCollection*> envs, ClosureEnvId closure_env,
        Function& result, Diagnostics& diag, StringTable& strings);

    FunctionIRGen(const FunctionIRGen&) = delete;
    FunctionIRGen& operator=(const FunctionIRGen&) = delete;

    ModuleIRGen& module() const { return module_; }
    Diagnostics& diag() const { return diag_; }
    StringTable& strings() const { return strings_; }
    Function& result() const { return result_; }
    NotNull<ClosureEnvCollection*> envs() const { return TIRO_NN(envs_.get()); }
    ClosureEnvId outer_env() const { return outer_env_; }

    /// Compilation entry point. Starts compilation of the given function.
    void compile_function(NotNull<FuncDecl*> func);

    /// Compilation entry point. Starts compilation of the decls' initializers (as a function).
    void compile_initializer(NotNull<File*> module);

private:
    void enter_compilation(FunctionRef<void(CurrentBlock& bb)> compile_body);

public:
    const LoopContext* current_loop() const;
    ClosureEnvId current_env() const;

    /// Compiles the given expression. Might not return a value (e.g. unreachable).
    ExprResult compile_expr(NotNull<Expr*> expr, CurrentBlock& bb,
        ExprOptions options = ExprOptions::Default);

    /// Compiles the given statement. Returns false if the statement terminated control flow, i.e.
    /// if the following code would be unreachable.
    StmtResult compile_stmt(NotNull<ASTStmt*> stmt, CurrentBlock& bb);

    /// Compites the given loop body. Automatically arranges for a loop context to be pushed
    /// (and popped) from the loop stack.
    /// The loop scope is needed to create a new nested closure environment if neccessary.
    StmtResult
    compile_loop_body(NotNull<Expr*> body, NotNull<Scope*> loop_scope,
        BlockId break_id, BlockId continue_id, CurrentBlock& bb);

    /// Compiles code that derefences the given symbol.
    LocalId compile_reference(NotNull<Symbol*> symbol, BlockId block);

    void
    compile_assign(const AssignTarget& target, LocalId value, BlockId block_id);

    /// Generates code that assigns the given value to the symbol.
    void
    compile_assign(NotNull<Symbol*> symbol, LocalId value, BlockId block_id);

    /// Generates code that assign the given value to the memory location specified by `lvalue`.
    void compile_assign(const LValue& lvalue, LocalId value, BlockId block_id);

    /// Compiles a reference to the given closure environment, usually for the purpose of creating
    /// a closure function object.
    LocalId compile_env(ClosureEnvId env, BlockId block);

    /// Compiles the given rvalue and returns a local SSA variable that represents that value.
    /// Performs some ad-hoc optimizations, so the resulting local will not neccessarily have exactly
    /// the given rvalue. Locals can be reused, so the returned local id may not be new.
    LocalId compile_rvalue(const RValue& value, BlockId block_id);

    /// Returns a new CurrentBlock instance that references this context.
    CurrentBlock make_current(BlockId block_id) { return {*this, block_id}; }

    /// Create a new block. Blocks must be sealed after all predecessor nodes have been linked.
    BlockId make_block(InternedString label);

    /// Defines a new local variable in the given block and returns its id.
    ///
    /// \note Only use this function if you want to actually introduce a new local variable.
    ///       Use compile_rvalue() instead to benefit from optimizations.
    LocalId define_new(const RValue& value, BlockId block_id);
    LocalId define_new(const Local& local, BlockId block_id);

    /// Returns the local value associated with the given key and block. If the key is not present, then
    /// the `compute` function will be executed to produce it.
    LocalId memoize_value(const ComputedValue& key,
        FunctionRef<LocalId()> compute, BlockId block_id);

    /// Seals the given block after all possible predecessors have been linked to it.
    /// Only when a block is sealed can we analyze the completed (nested) control flow graph.
    /// It is an error when a block is left unsealed.
    void seal(BlockId block_id);

    /// Ends the block by settings outgoing edges. The block automatically becomes filled.
    void end(const Terminator& term, BlockId block_id);

private:
    /// Emits a new statement into the given block.
    /// Must not be called if the block has already been filled.
    void emit(const Stmt& stmt, BlockId block_id);

    /// Associates the given variable with its current value in the given basic block.
    void write_variable(NotNull<Symbol*> var, LocalId value, BlockId block_id);

    /// Returns the current SSA value for the given variable in the given block.
    LocalId read_variable(NotNull<Symbol*> var, BlockId block_id);

    /// Recursive resolution algorithm for variables. See Algorithm 2 in [BB+13].
    LocalId read_variable_recursive(NotNull<Symbol*> var, BlockId block_id);

    void
    add_phi_operands(NotNull<Symbol*> var, LocalId value, BlockId block_id);

    /// Analyze the scopes reachable from `scope` until a loop scope or nested function
    /// scope is encountered. All captured variables declared within these scopes are grouped
    /// together into the same closure environment.
    ///
    /// \pre `scope` must be either a loop or a function scope.
    void enter_env(NotNull<Scope*> scope, CurrentBlock& bb);
    void exit_env(NotNull<Scope*> scope);

    /// Returns the runtime location of the given closure environment.
    std::optional<LocalId> find_env(ClosureEnvId env);

    /// Like find_env(), but fails with an assertion error if the environment was not found.
    LocalId get_env(ClosureEnvId env);

    /// Lookup the given symbol as an lvalue of non-local type.
    /// Returns an empty optional if the symbol does not qualify (lookup as local instead).
    std::optional<LValue> find_lvalue(NotNull<Symbol*> symbol);

    /// Returns an lvalue for accessing the given closure env location.
    LValue get_captured_lvalue(const ClosureEnvLocation& loc);

private:
    // TODO: Better map implementation
    using VariableMap =
        std::unordered_map<std::tuple<Symbol*, BlockId>, LocalId, UseHasher>;

    using ValuesMap = std::unordered_map<std::tuple<ComputedValue, BlockId>,
        LocalId, UseHasher>;

    // Represents an incomplete phi nodes. These are cleaned up when a block is sealed.
    // Only incomplete control flow graphs (i.e. loops) can produce incomplete phi nodes.
    using IncompletePhi = std::tuple<NotNull<Symbol*>, LocalId>;

    // TODO: Better container.
    using IncompletePhiMap =
        std::unordered_map<BlockId, std::vector<IncompletePhi>, UseHasher>;

private:
    ModuleIRGen& module_;
    Ref<ClosureEnvCollection> envs_; // Init at top level, never null.
    ClosureEnvId outer_env_;         // Optional
    Function& result_;
    Diagnostics& diag_;
    StringTable& strings_;

    // Tracks active loops. The last context represents the innermost loop.
    std::vector<LoopContext> active_loops_;

    // Tracks active closure environments. The last context represents the innermost environment.
    std::vector<EnvContext> local_env_stack_;

    // Supports variable numbering in the function. This map holds the current value
    // for each variable declaration and block.
    VariableMap variables_;

    // Supports value numbering in this function. Every block has its own private store
    // of already-computed values. Note that these are usually not shared between blocks right now.
    ValuesMap values_;

    // Represents the set of pending incomplete phi variables.
    IncompletePhiMap incomplete_phis_;

    // Maps closure environments to the ssa local that references their runtime representation.
    // TODO: Better map implementation
    std::unordered_map<ClosureEnvId, LocalId, UseHasher> local_env_locations_;
};

/// Base class for transformers.
/// Note: this class is non-virtual on purpose. Do not use it in a polymorphic way.
class Transformer {
public:
    Transformer(FunctionIRGen& ctx, CurrentBlock& bb)
        : ctx_(ctx)
        , bb_(bb) {}

    Transformer(const Transformer&) = delete;
    Transformer& operator=(const Transformer&) = delete;

    Diagnostics& diag() const { return ctx_.diag(); }
    StringTable& strings() const { return ctx_.strings(); }
    Function& result() const { return ctx_.result(); }
    FunctionIRGen& ctx() const { return ctx_; }
    CurrentBlock& bb() const { return bb_; }

    const LoopContext* current_loop() const { return ctx_.current_loop(); }

private:
    FunctionIRGen& ctx_;
    CurrentBlock& bb_;
};

} // namespace tiro

#endif // TIRO_IR_GEN_GEN_FUNC_HPP
