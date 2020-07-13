#ifndef TIRO_COMPILER_IR_GEN_MODULE_HPP
#define TIRO_COMPILER_IR_GEN_MODULE_HPP

#include "common/hash.hpp"
#include "common/safe_int.hpp"
#include "common/string_table.hpp"
#include "compiler/ast/fwd.hpp"
#include "compiler/fwd.hpp"
#include "compiler/ir/function.hpp"
#include "compiler/ir/fwd.hpp"
#include "compiler/ir_gen/closures.hpp"
#include "compiler/semantics/fwd.hpp"

#include "absl/container/flat_hash_map.h"

#include <queue>

namespace tiro {

struct ModuleContext {
    std::string_view source_file; // TODO: This will have to be multiple files soon
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

    std::string_view source_file() const { return ctx_.source_file; }
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
    absl::flat_hash_map<SymbolId, ModuleMemberId, UseHasher> symbol_to_member_;

    // Defining symbol for module member.
    absl::flat_hash_map<ModuleMemberId, SymbolId, UseHasher> member_to_symbol_;
};

} // namespace tiro

#endif // TIRO_COMPILER_IR_GEN_MODULE_HPP
