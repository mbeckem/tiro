#ifndef TIRO_IR_GEN_GEN_MODULE_HPP
#define TIRO_IR_GEN_GEN_MODULE_HPP

#include "tiro/ast/fwd.hpp"
#include "tiro/compiler/fwd.hpp"
#include "tiro/core/safe_int.hpp"
#include "tiro/core/string_table.hpp"
#include "tiro/ir/function.hpp"
#include "tiro/ir/fwd.hpp"
#include "tiro/ir_gen/closures.hpp"
#include "tiro/semantics/fwd.hpp"

#include <queue>

namespace tiro {

class ModuleIRGen final {
public:
    /// TODO module ast node?
    explicit ModuleIRGen(NotNull<AstFile*> module, Module& result,
        const SymbolTable& symbols, StringTable& strings, Diagnostics& diag);

    Module& result() const { return result_; }
    const SymbolTable& symbols() const { return symbols_; }
    StringTable& strings() const { return strings_; }
    Diagnostics& diag() const { return diag_; }

    void compile_module();

    /// Attempts to find the given symbol at module scope.
    /// Returns an invalid id if the lookup fails.
    ModuleMemberId find_symbol(SymbolId symbol) const;

    /// Schedules compilation of the given nested function.
    /// Returns the new function's id within the module.
    ModuleMemberId add_function(NotNull<AstFuncDecl*> func,
        NotNull<ClosureEnvCollection*> envs, ClosureEnvId env);

private:
    struct FunctionJob {
        /// Function AST node.
        NotNull<AstFuncDecl*> decl;

        /// Id of the function within the module.
        ModuleMemberId member;

        /// Collection of closure environments.
        Ref<ClosureEnvCollection> envs;

        ///< Outer function environment (optional).
        ClosureEnvId env;
    };

    void start();

    // Enqueues a compilation job for the given function declaration.
    ModuleMemberId enqueue_function_job(NotNull<FuncDecl*> decl,
        NotNull<ClosureEnvCollection*> envs, ClosureEnvId env);

private:
    NotNull<AstFile*> module_;
    Module& result_;
    const SymbolTable& symbols_;
    StringTable& strings_;
    Diagnostics& diag_;

    std::queue<FunctionJob> jobs_;
    std::unordered_map<SymbolId, ModuleMemberId> members_;
};

} // namespace tiro

#endif // TIRO_IR_GEN_GEN_MODULE_HPP
