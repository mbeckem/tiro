#ifndef TIRO_MIR_TRANSFORM_MODULE_HPP
#define TIRO_MIR_TRANSFORM_MODULE_HPP

#include "tiro/compiler/string_table.hpp"
#include "tiro/core/safe_int.hpp"
#include "tiro/mir/closures.hpp"
#include "tiro/mir/fwd.hpp"
#include "tiro/mir/types.hpp"
#include "tiro/syntax/ast.hpp"

#include <queue>

namespace tiro::compiler::mir_transform {

struct NestedFunction {
    NotNull<FuncDecl*> func;
    ClosureEnvID env; // Optional
};

class ModuleContext final {
public:
    /// TODO module ast node?
    explicit ModuleContext(
        NotNull<Root*> module, mir::Module& result, StringTable& strings);

    StringTable& strings() const { return strings_; }
    mir::Module& result() const { return result_; }

    void compile_module();

    /// Attempts to find the given symbol at module scope.
    /// Returns an invalid id if the lookup fails.
    mir::ModuleMemberID find_symbol(NotNull<Symbol*> symbol) const;

    /// Schedules compilation of the given nested function.
    /// Returns the new function's id within the module.
    mir::ModuleMemberID add_function(NotNull<FuncDecl*> func,
        NotNull<ClosureEnvCollection*> envs, ClosureEnvID env);

private:
    struct FunctionJob {
        /// Function AST node.
        NotNull<FuncDecl*> decl;

        /// ID of the function within the module.
        mir::ModuleMemberID member;

        /// Collection of closure environments.
        Ref<ClosureEnvCollection> envs;

        ///< Outer function environment (optional).
        ClosureEnvID env;
    };

    void add_symbols();

    // Enqueues a compilation job for the given function declaration.
    mir::ModuleMemberID enqueue_function_job(NotNull<FuncDecl*> decl,
        NotNull<ClosureEnvCollection*> envs, ClosureEnvID env);

private:
    NotNull<Root*> module_;
    StringTable& strings_;
    mir::Module& result_;

    std::queue<FunctionJob> jobs_;
    std::unordered_map<NotNull<Symbol*>, mir::ModuleMemberID> members_;
};

} // namespace tiro::compiler::mir_transform

#endif // TIRO_MIR_TRANSFORM_MODULE_HPP
