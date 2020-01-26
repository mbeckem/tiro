#ifndef TIRO_CODEGEN_VARIABLE_LOCATIONS
#define TIRO_CODEGEN_VARIABLE_LOCATIONS

#include "tiro/core/defs.hpp"
#include "tiro/semantics/symbol_table.hpp"
#include "tiro/syntax/ast.hpp"

#include <optional>
#include <unordered_map>
#include <vector>

namespace tiro::compiler {

struct ClosureContext {
    // Parent is null when this is the root context.
    ClosureContext* parent = nullptr;

    // The function this closure context belongs to.
    // It is currently needed to distinguish local closure context
    // objects from those passed in by an outer function.
    // TODO: Rework this design, there should be a better way
    // to see whether a context is local.
    FuncDecl* func = nullptr;

    // Index of the local variable that holds this context
    // within the function that created it.
    u32 local_index = u32(-1);

    // Number of variables in this context.
    u32 size = 0;

    explicit ClosureContext(ClosureContext* parent_, FuncDecl* func_)
        : parent(parent_)
        , func(func_) {}
};

enum class VarLocationType {
    Param,
    Local,
    Module,
    Context,
};

struct VarLocation {
    VarLocationType type;

    union {
        struct {
            u32 index;
        } param;

        struct {
            u32 index;
        } local;

        struct {
            bool constant;
            u32 index;
        } module;

        struct {
            ClosureContext* ctx;
            u32 index; // Index of the variable within ctx
        } context;
    };
};

class FunctionLocations final {
public:
    /// Computes the locations for all variables declared in this function.
    static FunctionLocations
    compute(FuncDecl* func, ClosureContext* parent_context,
        const SymbolTable& symbols, const StringTable& strings);

    /// Attempts to find the location of the given symbol entry.
    /// Returns an empty optional on failure.
    std::optional<VarLocation> get_location(const SymbolEntryPtr& entry) const;

    /// Returns the closure context started by this scope, or null.
    ClosureContext* get_closure_context(const ScopePtr& scope);

    /// Returns the number of parameters in this function.
    u32 params() const { return params_; }

    /// Returns the number of locals in this function (the number of slots required,
    /// local variables reuse slots if possible).
    u32 locals() const { return locals_; }

private:
    struct Computation;

private:
    // Links scopes to the (optional) closure context started by them.
    std::unordered_map<ScopePtr, ClosureContext> closure_contexts_;

    // Links variable declarations to their final locations within the functions.
    std::unordered_map<SymbolEntryPtr, VarLocation> locations_;

    // The number of parameters required for the function.
    u32 params_ = 0;

    // The number of locals required for the function. Local slots are reused
    // for different variables if possible.
    u32 locals_ = 0;
};

} // namespace tiro::compiler

#endif
