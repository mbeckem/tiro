#ifndef HAMMER_COMPILER_CODEGEN_VARIABLE_LOCATIONS
#define HAMMER_COMPILER_CODEGEN_VARIABLE_LOCATIONS

#include "hammer/ast/fwd.hpp"
#include "hammer/core/defs.hpp"

#include <optional>
#include <unordered_map>
#include <vector>

namespace hammer {

struct ClosureContext {
    // Parent is null when this is the root context.
    ClosureContext* parent = nullptr;

    // The function this closure context belongs to.
    // It is currently needed to distinguish local closure context
    // objects from those passed in by an outer function.
    // TODO: Rework this design, there should be a better way
    // to see whether a context is local.
    ast::FuncDecl* func = nullptr;

    // Index of the local variable that holds this context
    // within the function that created it.
    u32 local_index = u32(-1);

    // Number of variables in this context.
    u32 size = 0;

    ClosureContext(ClosureContext* parent_, ast::FuncDecl* func_)
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

class FunctionLocations {
public:
    /**
     * Computes the locations for all variables declared in this function.
     */
    static FunctionLocations compute(ast::FuncDecl* func);

    /**
     * Attempts to find the location of the given declaration.
     * Returns an empty optional on failure.
     */
    std::optional<VarLocation> get_location(ast::Decl* decl) const;

    /// Returns the closure context started by this node, or null.
    ClosureContext* get_closure_context(ast::Node* starter);

    /// Returns the number of parameters in this function.
    u32 params() const { return params_; }

    /// Returns the number of locals in this function (the number of slots required,
    /// local variables reuse slots if possible).
    u32 locals() const { return locals_; }

private:
    struct Computation;

private:
    // Links nodes to the (optional) closure context started by them.
    std::unordered_map<ast::Node*, ClosureContext> closure_contexts_;

    // Links variable declarations to their final locations within the functions.
    std::unordered_map<ast::Decl*, VarLocation> locations_;

    // The number of parameters required for the function.
    u32 params_ = 0;

    // The number of locals required for the function. Local slots are reused
    // for different variables if possible.
    u32 locals_ = 0;
};

} // namespace hammer

#endif
