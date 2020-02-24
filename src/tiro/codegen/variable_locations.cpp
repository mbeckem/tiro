#include "tiro/codegen/variable_locations.hpp"

#include "tiro/codegen/func_codegen.hpp"
#include "tiro/core/defs.hpp"
#include "tiro/core/safe_int.hpp"
#include "tiro/semantics/analyzer.hpp"

namespace tiro::compiler {

struct FunctionLocations::Computation {
    NotNull<Scope*> root_scope_;
    FunctionCodegen* container_;
    ClosureContext* parent_context_;
    const SymbolTable& symbols_;
    const StringTable& strings_;
    FunctionLocations result_;

    // TODO better design
    ParamList* params_ = nullptr;

    explicit Computation(NotNull<Scope*> root_scope, FunctionCodegen* container,
        ClosureContext* parent_context, const SymbolTable& symbols,
        const StringTable& strings)
        : root_scope_(root_scope)
        , container_(container)
        , parent_context_(parent_context)
        , symbols_(symbols)
        , strings_(strings) {}

    void execute() {
        compute_params();
        compute_closure_scopes();
        compute_locals();
    }

    void compute_params() {
        if (!params_)
            return;

        const auto params = TIRO_NN(params_);
        const size_t param_count = params->size();
        TIRO_CHECK(param_count <= std::numeric_limits<u32>::max(),
            "Too many parameters.");

        for (size_t i = 0; i < param_count; ++i) {
            const NotNull param = TIRO_NN(params->get(i));
            const NotNull entry = TIRO_NN(param->declared_symbol());

            if (entry->captured())
                continue;

            VarLocation loc;
            loc.type = VarLocationType::Param;
            loc.param.index = static_cast<u32>(i);
            insert_location(entry, loc);
        }

        result_.params_ = static_cast<u32>(param_count);
    }

    void compute_locals() { compute_locals(root_scope_, 0); }

    void compute_locals(NotNull<Scope*> scope, SafeInt<u32> next_local) {
        // Don't recurse into nested functions.
        if (scope->function() != root_scope_->function())
            return;

        // Assign a local index to the closure context (if any).
        // I believe that this could be better handled at a higher level (introduce
        // a new local variable for this context), but this will do for now.
        if (auto ctx = result_.get_closure_context(TIRO_NN(scope.get()))) {
            ctx->local_index = (next_local++).value();
        }

        // Assign a local index to every (non-captured) decl in this scope.
        for (const SymbolPtr& entry : scope->entries()) {
            if (entry->type() != SymbolType::LocalVar || entry->captured())
                continue;

            VarLocation loc;
            loc.type = VarLocationType::Local;
            loc.local.index = (next_local++).value();
            insert_location(entry, loc);
        }
        result_.locals_ = std::max(next_local.value(), result_.locals_);

        // Nested scopes start with the current "next_local" value.
        // Sibling scopes will reuse locals!
        for (const ScopePtr& child : scope->children()) {
            compute_locals(TIRO_NN(child.get()), next_local);
        }
    }

    // Visit all scopes and identify variables that are captured by nested functions.
    // These variables must not be allocated as locals but must instead be allocated
    // on the heap, inside a closure context. This approach ensures that nested function
    // can continue to reference the captured variables, even after the outer function
    // has already finished executing.
    //
    // Not every scope gets its own closure context (that would introduce too many allocations).
    // Instead, closure scopes are grouped and are only allocated when necessary (function scope,
    // loop scope).
    void compute_closure_scopes() {
        compute_closure_scopes(root_scope_, parent_context_);
    }

    void
    compute_closure_scopes(NotNull<Scope*> top_scope, ClosureContext* parent) {
        // Can be grouped into a single closure context allocation
        std::vector<NotNull<Scope*>> flattened_scopes;
        // Need new closure context allocations (e.g. loop body)
        std::vector<NotNull<Scope*>> nested_scopes;

        gather_flattened_closure_scopes(
            top_scope, flattened_scopes, nested_scopes);

        ClosureContext* new_context = nullptr;
        SafeInt<u32> captured_variables = 0;
        for (const auto scope : flattened_scopes) {
            for (const SymbolPtr& entry : scope->entries()) {
                if ((entry->type() != SymbolType::LocalVar
                        && entry->type() != SymbolType::ParameterVar)
                    || !entry->captured())
                    continue;

                if (!new_context)
                    new_context = add_closure_context(top_scope, parent);

                VarLocation loc;
                loc.type = VarLocationType::Context;
                loc.context.ctx = new_context;
                loc.context.index = (captured_variables++).value();
                insert_location(entry, loc);
            }
        }

        if (new_context)
            new_context->size = captured_variables.value();

        for (const auto& nested_scope : nested_scopes) {
            compute_closure_scopes(
                nested_scope, new_context ? new_context : parent);
        }
    }

    void gather_flattened_closure_scopes(NotNull<Scope*> parent,
        std::vector<NotNull<Scope*>>& flattened_scopes,
        std::vector<NotNull<Scope*>>& nested_scopes) {
        TIRO_ASSERT(parent->function() == root_scope_->function(),
            "Parent must point into this function.");

        flattened_scopes.push_back(parent);
        for (const auto& child : parent->children()) {
            // Ignore nested functions
            if (child->function() != root_scope_->function())
                continue;

            // Loop bodies must start their own closure context, because their body can be executed multiple times.
            // Each iteration's variables are different and must not share locations, in case they are captured.
            if (child->type() == ScopeType::LoopBody) {
                nested_scopes.push_back(TIRO_NN(child.get()));
                continue;
            }

            gather_flattened_closure_scopes(
                TIRO_NN(child.get()), flattened_scopes, nested_scopes);
        }
    }

    ClosureContext*
    add_closure_context(NotNull<Scope*> scope, ClosureContext* parent) {
        const auto ptr = ref(scope.get()); // TODO heterogenous container

        if (result_.closure_contexts_.count(ptr)) {
            TIRO_ERROR(
                "There is already a closure context "
                "associated with that scope.");
        }

        auto result = result_.closure_contexts_.emplace(
            ptr, ClosureContext(parent, container_));

        ClosureContext* context = &result.first->second;
        return context;
    }

    void insert_location(const SymbolPtr& entry, VarLocation loc) {
        TIRO_ASSERT_NOT_NULL(entry);
        TIRO_ASSERT(result_.locations_.count(entry) == 0,
            "Location for this declaration was already computed.");
        result_.locations_.emplace(entry, loc);
    }
};

FunctionLocations FunctionLocations::compute(NotNull<FuncDecl*> func,
    FunctionCodegen* container, ClosureContext* parent_context,
    const SymbolTable& symbols, const StringTable& strings) {

    const auto root_scope = func->param_scope();

    Computation comp(
        TIRO_NN(root_scope.get()), container, parent_context, symbols, strings);
    comp.params_ = func->params();
    comp.execute();
    return std::move(comp.result_);
}

FunctionLocations FunctionLocations::compute(NotNull<Scope*> scope,
    FunctionCodegen* container, ClosureContext* parent_context,
    const SymbolTable& symbols, const StringTable& strings) {

    Computation comp(scope, container, parent_context, symbols, strings);
    comp.execute();
    return std::move(comp.result_);
}

std::optional<VarLocation>
FunctionLocations::get_location(NotNull<Symbol*> entry) const {
    // TODO: Heterogenous lookup with a better container type.
    if (auto pos = locations_.find(ref(entry.get())); pos != locations_.end()) {
        return pos->second;
    }

    return {};
}

ClosureContext* FunctionLocations::get_closure_context(NotNull<Scope*> scope) {
    // TODO: Heterogenous lookup with a better container type.
    if (auto pos = closure_contexts_.find(ref(scope.get()));
        pos != closure_contexts_.end()) {
        return &pos->second;
    }

    return nullptr;
}

} // namespace tiro::compiler
