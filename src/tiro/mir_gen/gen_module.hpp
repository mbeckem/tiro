#ifndef TIRO_MIR_GEN_GEN_MODULE_HPP
#define TIRO_MIR_GEN_GEN_MODULE_HPP

#include "tiro/core/safe_int.hpp"
#include "tiro/core/string_table.hpp"
#include "tiro/mir/fwd.hpp"
#include "tiro/mir/types.hpp"
#include "tiro/mir_gen/closures.hpp"
#include "tiro/syntax/ast.hpp"

#include <queue>

namespace tiro {

class ModuleMIRGen final {
public:
    /// TODO module ast node?
    explicit ModuleMIRGen(NotNull<Root*> module, Module& result,
        Diagnostics& diag, StringTable& strings);

    Diagnostics& diag() const { return diag_; }
    StringTable& strings() const { return strings_; }
    Module& result() const { return result_; }

    void compile_module();

    /// Attempts to find the given symbol at module scope.
    /// Returns an invalid id if the lookup fails.
    ModuleMemberID find_symbol(NotNull<Symbol*> symbol) const;

    /// Schedules compilation of the given nested function.
    /// Returns the new function's id within the module.
    ModuleMemberID add_function(NotNull<FuncDecl*> func,
        NotNull<ClosureEnvCollection*> envs, ClosureEnvID env);

private:
    struct FunctionJob {
        /// Function AST node.
        NotNull<FuncDecl*> decl;

        /// ID of the function within the module.
        ModuleMemberID member;

        /// Collection of closure environments.
        Ref<ClosureEnvCollection> envs;

        ///< Outer function environment (optional).
        ClosureEnvID env;
    };

    void add_symbols();

    // Enqueues a compilation job for the given function declaration.
    ModuleMemberID enqueue_function_job(NotNull<FuncDecl*> decl,
        NotNull<ClosureEnvCollection*> envs, ClosureEnvID env);

private:
    NotNull<Root*> module_;
    Diagnostics& diag_;
    StringTable& strings_;
    Module& result_;

    std::queue<FunctionJob> jobs_;
    std::unordered_map<NotNull<Symbol*>, ModuleMemberID> members_;
};

} // namespace tiro

#endif // TIRO_MIR_GEN_GEN_MODULE_HPP
