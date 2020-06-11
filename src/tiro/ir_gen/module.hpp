#ifndef TIRO_IR_GEN_MODULE_HPP
#define TIRO_IR_GEN_MODULE_HPP

#include "tiro/ast/fwd.hpp"
#include "tiro/compiler/fwd.hpp"
#include "tiro/core/hash.hpp"
#include "tiro/core/safe_int.hpp"
#include "tiro/core/string_table.hpp"
#include "tiro/ir/function.hpp"
#include "tiro/ir/fwd.hpp"
#include "tiro/ir_gen/closures.hpp"
#include "tiro/semantics/fwd.hpp"

#include <queue>

namespace tiro {

struct ModuleContext {
    NotNull<AstFile*> module;
    const AstNodeMap& nodes;
    const SymbolTable& symbols;
    const TypeTable& types;
    StringTable& strings;
    Diagnostics& diag;
};

class ModuleIRGen final {
public:
    /// TODO module ast node?
    explicit ModuleIRGen(ModuleContext ctx, Module& result);

    NotNull<AstFile*> module() const { return ctx_.module; }
    const AstNodeMap& nodes() const { return ctx_.nodes; }
    const TypeTable& types() const { return ctx_.types; }
    const SymbolTable& symbols() const { return ctx_.symbols; }
    StringTable& strings() const { return ctx_.strings; }
    Diagnostics& diag() const { return ctx_.diag; }

    Module& result() const { return result_; }

    void compile_module();

    /// Attempts to find the given symbol at module scope.
    /// Returns an invalid id if the lookup fails.
    ModuleMemberId find_symbol(SymbolId symbol) const;

    /// Returns the symbol that defined the given module member.
    /// Returns an invalid id if no symbol was found.
    SymbolId find_definition(ModuleMemberId member) const;

    /// Schedules compilation of the given nested function.
    /// Returns the new function's id within the module.
    ModuleMemberId
    add_function(NotNull<AstFuncDecl*> func, NotNull<ClosureEnvCollection*> envs, ClosureEnvId env);

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
    ModuleMemberId enqueue_function_job(
        NotNull<AstFuncDecl*> decl, NotNull<ClosureEnvCollection*> envs, ClosureEnvId env);

    void link(SymbolId symbol, ModuleMemberId member);

private:
    ModuleContext ctx_;
    Module& result_;

    std::queue<FunctionJob> jobs_;

    // Module Member defined by symbol.
    std::unordered_map<SymbolId, ModuleMemberId, UseHasher> symbol_to_member_;

    // Defining symbol for module member.
    std::unordered_map<ModuleMemberId, SymbolId, UseHasher> member_to_symbol_;
};

} // namespace tiro

#endif // TIRO_IR_GEN_MODULE_HPP
