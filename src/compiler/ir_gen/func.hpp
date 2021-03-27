#ifndef TIRO_COMPILER_IR_GEN_FUNC_HPP
#define TIRO_COMPILER_IR_GEN_FUNC_HPP

#include "common/entities/entity_storage.hpp"
#include "common/memory/ref_counted.hpp"
#include "common/safe_int.hpp"
#include "compiler/ast/fwd.hpp"
#include "compiler/diagnostics.hpp"
#include "compiler/ir/function.hpp"
#include "compiler/ir/fwd.hpp"
#include "compiler/ir_gen/closures.hpp"
#include "compiler/ir_gen/fwd.hpp"
#include "compiler/ir_gen/support.hpp"
#include "compiler/reset_value.hpp"

#include "absl/container/flat_hash_map.h"

#include <memory>
#include <optional>
#include <queue>

namespace tiro::ir {

struct FunctionContext {
    ModuleIRGen& module_gen;
    NotNull<ClosureEnvCollection*> envs;
    ClosureEnvId closure_env;
};

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
        TIRO_DEBUG_ASSERT(type_ != TransformResultType::Value, "Must not represent a value.");
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

    T& value() {
        TIRO_DEBUG_ASSERT(is_value(), "TransformResult is not a value.");
        TIRO_DEBUG_ASSERT(value_, "Optional must hold a value if is_value() is true.");
        return *value_;
    }

    const T& value() const {
        TIRO_DEBUG_ASSERT(is_value(), "TransformResult is not a value.");
        TIRO_DEBUG_ASSERT(value_, "Optional must hold a value if is_value() is true.");
        return *value_;
    }

    T& operator*() { return value(); }
    const T& operator*() const { return value(); }

    TransformResultType type() const noexcept { return type_; }

    bool is_value() const noexcept { return type_ == TransformResultType::Value; }

    bool is_unreachable() const noexcept { return type_ == TransformResultType::Unreachable; }

    Failure failure() const {
        TIRO_DEBUG_ASSERT(!is_value(), "Result must not hold a value.");
        return Failure(type_);
    }

    explicit operator bool() const noexcept { return is_value(); }

private:
    TransformResultType type_;
    std::optional<T> value_;
};

struct EnvContext {
    ClosureEnvId env;
    ScopeId starter;
};

/// Compilation options for expressions.
// TODO: Use enum flags from core module
enum class ExprOptions : int {
    Default = 0,

    /// May return an invalid inst id (-> disables the debug assertion)
    MaybeInvalid = 1 << 0,
};

inline ExprOptions operator|(ExprOptions lhs, ExprOptions rhs) {
    return static_cast<ExprOptions>(static_cast<int>(lhs) | static_cast<int>(rhs));
}

inline ExprOptions operator&(ExprOptions lhs, ExprOptions rhs) {
    return static_cast<ExprOptions>(static_cast<int>(lhs) & static_cast<int>(rhs));
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

    /// Terminates the current block with a jump to a new block.
    /// The new block will become the current block of this cursor.
    /// The purpose of this function is to apply the label and the context's current handler.
    ///
    /// NOTE: the new block will be sealed.
    void advance(InternedString label);

    FunctionIRGen& ctx() const { return ctx_; }
    BlockId id() const { return id_; }

    InstResult compile_expr(NotNull<AstExpr*> expr, ExprOptions options = ExprOptions::Default);

    OkResult compile_stmt(NotNull<AstStmt*> stmt);

    InstId compile_value(const Value& value);

    OkResult compile_loop_body(ScopeId body_scope_id, FunctionRef<OkResult()> compile_body,
        BlockId break_id, BlockId continue_id);

    void compile_assign(const AssignTarget& target, InstId value);

    InstId compile_read(const AssignTarget& target);

    InstId compile_env(ClosureEnvId env);

    InstId define_new(Value&& value);

    InstId memoize_value(const ComputedValue& key, FunctionRef<InstId()> compute);

    void seal();
    void end(Terminator term);

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
    explicit FunctionIRGen(FunctionContext ctx, Function& result);

    FunctionIRGen(const FunctionIRGen&) = delete;
    FunctionIRGen& operator=(const FunctionIRGen&) = delete;

    std::string_view source_file() const;
    ModuleIRGen& module_gen() const;
    const AstNodeMap& nodes() const;
    const TypeTable& types() const;
    const SymbolTable& symbols() const;
    StringTable& strings() const;
    Diagnostics& diag() const;

    NotNull<ClosureEnvCollection*> envs() const { return TIRO_NN(envs_.get()); }
    ClosureEnvId outer_env() const { return outer_env_; }
    Function& result() const { return result_; }

    /// Compilation entry point. Starts compilation of the given function.
    void compile_function(NotNull<AstFuncDecl*> func);

    /// Compilation entry point. Starts compilation of the decls' initializers (as a function).
    void compile_initializer(NotNull<AstFile*> module);

    /// Returns a new CurrentBlock instance that references this context.
    CurrentBlock make_current(BlockId block_id) { return {*this, block_id}; }

    /// Create a new block. Blocks must be sealed after all predecessor nodes have been linked.
    BlockId make_block(InternedString label);

    /// Creates a new handler block.
    BlockId make_handler_block(InternedString label);

private:
    void enter_compilation(FunctionRef<void(CurrentBlock& bb)> compile_body);

public:
    /// A class that represents an entered region. The guard will leave the region
    /// automatically when destroyed.
    class [[nodiscard]] RegionGuard final {
    public:
        ~RegionGuard();

        RegionGuard(const RegionGuard&) = delete;
        RegionGuard& operator=(const RegionGuard&) = delete;

        RegionId id() const { return new_id_; }

    private:
        friend FunctionIRGen;

        RegionGuard(FunctionIRGen * func, RegionId new_id, RegionId & slot);

    private:
        FunctionIRGen* func_;              // Parent instance
        RegionId new_id_;                  // Expected to be on top on destruction
        ResetValue<RegionId> restore_old_; // Restore old scope / loop
    };

    // Note: these functions raise debug assertions if there is no loop / scope.
    EntityPtr<Region> current_loop();
    EntityPtr<Region> current_scope();

    RegionId current_loop_id() const { return current_loop_; }
    RegionId current_scope_id() const { return current_scope_; }

    RegionGuard enter_loop(BlockId jump_break, BlockId jump_continue);
    RegionGuard enter_scope();

    ClosureEnvId current_env() const;

    /// Returns the current exception handler. Blocks created through this object will
    /// inherit the current handler.
    BlockId current_handler() const;

    /// Sets the current handler to the specified block id, which may be invalid to signal "no handler" (the default).
    void current_handler(BlockId handler_id);

    /// Compiles the loop body in the given basic block. Automatically manages the required
    /// loop context (including the closure environment).
    ///
    /// \param body_scope_id The scope id of the loop's body. Required for the closure environment.
    /// \param compile_body Callback is executed when the loop context is set up. The result is returned.
    /// \param break_id Jump label for "break" expressions in the loop body.
    /// \param continue_id Jump label for "continue" expressions in the loop body.
    /// \param bb The current basic block.
    OkResult compile_loop_body(ScopeId body_scope_id, FunctionRef<OkResult()> compile_body,
        BlockId break_id, BlockId continue_id, CurrentBlock& bb);

    /// Compiles code that derefences the given symbol.
    InstId compile_reference(SymbolId symbol, CurrentBlock& bb);

    /// Generates code that implements the given assignment (i.e. target = value).
    void compile_assign(const AssignTarget& target, InstId value, CurrentBlock& bb);

    /// Generates code that reads from the given target location.
    InstId compile_read(const AssignTarget& target, CurrentBlock& bb);

    /// Compiles a reference to the given closure environment, usually for the purpose of creating
    /// a closure function object.
    InstId compile_env(ClosureEnvId env, BlockId block);

    /// Emits code required to leave the given scope.
    OkResult compile_scope_exit(RegionId scope_id, CurrentBlock& bb);

    /// Emits code to leave all scopes until the target region has been reached.
    /// This *does not* include the target region. The target may be invalid, in which
    /// case all scopes will be exited.
    OkResult compile_scope_exit_until(RegionId target, CurrentBlock& bb);

    /// Defines a new instruction in the given block and returns its id.
    ///
    /// \note Only use this function if you want to actually introduce a new instruction variable.
    ///       Use compile_value() instead to benefit from optimizations.
    InstId define_new(Value&& value, BlockId block_id);
    InstId define_new(Inst&& local, BlockId block_id);

    /// Returns the instruction id associated with the given key and block. If the key is not present, then
    /// the `compute` function will be executed to produce it.
    InstId memoize_value(const ComputedValue& key, FunctionRef<InstId()> compute, BlockId block_id);

    /// Seals the given block after all possible predecessors have been linked to it.
    /// Only when a block is sealed can we analyze the completed (nested) control flow graph.
    /// It is an error when a block is left unsealed.
    void seal(BlockId block_id);

    /// Ends the block by settings outgoing edges. The block automatically becomes filled.
    void end(Terminator term, BlockId block_id);

private:
    /// Emits a new instruction into the given block.
    /// Must not be called if the block has already been filled.
    /// An instruction may only be emitted exactly once.
    void emit(InstId inst, BlockId block_id);

    /// Associates the given variable with its current value in the given basic block.
    void write_variable(SymbolId var, InstId value, BlockId block_id);

    /// Returns the current SSA value for the given variable in the given block.
    InstId read_variable(SymbolId var, BlockId block_id);

    /// Recursive resolution algorithm for variables. See Algorithm 2 in [BB+13].
    InstId read_variable_recursive(SymbolId var, BlockId block_id);

    void add_phi_operands(SymbolId var, InstId value, BlockId block_id);

    /// Analyze the scopes reachable from `scope` until a loop scope or nested function
    /// scope is encountered. All captured variables declared within these scopes are grouped
    /// together into the same closure environment.
    ///
    /// \pre `scope` must be either a loop or a function scope.
    void enter_env(ScopeId scope, CurrentBlock& bb);
    void exit_env(ScopeId scope);

    bool can_open_closure_env(ScopeId scope) const;

    /// Returns the runtime location of the given closure environment.
    std::optional<InstId> find_env(ClosureEnvId env);

    /// Like find_env(), but fails with an assertion error if the environment was not found.
    InstId get_env(ClosureEnvId env);

    /// Lookup the given symbol as an lvalue of non-instruction type.
    /// Returns an empty optional if the symbol does not qualify (lookup as an ssa instruction instead).
    std::optional<LValue> find_lvalue(SymbolId symbol);

    /// Returns an lvalue for accessing the given closure env location.
    LValue get_captured_lvalue(const ClosureEnvLocation& loc);

    /// Called when an undefined variable is encountered to produce a diagnostic message.
    // TODO: Pass usage information around the code so we can print where use of the undefined
    // variable happens.
    void undefined_variable(SymbolId symbol_id);

private:
    using VariableMap = absl::flat_hash_map<std::tuple<SymbolId, BlockId>, InstId, UseHasher>;

    using ValuesMap = absl::flat_hash_map<std::tuple<ComputedValue, BlockId>, InstId, UseHasher>;

    // Represents an incomplete phi nodes. These are cleaned up when a block is sealed.
    // Only incomplete control flow graphs (i.e. loops) can produce incomplete phi nodes.
    using IncompletePhi = std::tuple<SymbolId, InstId>;

    using IncompletePhiMap = absl::flat_hash_map<BlockId, std::vector<IncompletePhi>, UseHasher>;

private:
    ModuleIRGen& module_gen_;
    Ref<ClosureEnvCollection> envs_; // Init at top level, never null.
    ClosureEnvId outer_env_;         // Optional
    Function& result_;

    // Tracks active regions (as a stack). Used to implement non-instructions actions like jump instructions
    // out of loops or evaluation of deferred expressions on scope exit.
    EntityStorage<Region, RegionId> active_regions_;

    // Currently active (inner-most) block scope (if any).
    RegionId current_scope_;

    // Currently active (inner-most) loop (if any).
    RegionId current_loop_;

    // Active exception handler.
    BlockId current_handler_;

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

    // Maps closure environments to the ssa instruction that references their runtime representation.
    absl::flat_hash_map<ClosureEnvId, InstId, UseHasher> local_env_locations_;
};

/// Base class for transformers, to avoid having to re-type all accessors all over again.
/// Note: this class is non-virtual on purpose. Do not use it in a polymorphic way.
class Transformer {
public:
    Transformer(FunctionIRGen& ctx)
        : ctx_(ctx) {}

    Transformer(const Transformer&) = delete;
    Transformer& operator=(const Transformer&) = delete;

    const AstNodeMap& nodes() const { return ctx_.nodes(); }
    const TypeTable& types() const { return ctx_.types(); }
    const SymbolTable& symbols() const { return ctx_.symbols(); }
    StringTable& strings() const { return ctx_.strings(); }
    Diagnostics& diag() const { return ctx_.diag(); }
    Function& result() const { return ctx_.result(); }
    FunctionIRGen& ctx() const { return ctx_; }

private:
    FunctionIRGen& ctx_;
};

} // namespace tiro::ir

#endif // TIRO_COMPILER_IR_GEN_FUNC_HPP
