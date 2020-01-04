#include "hammer/compiler/codegen/variable_locations.hpp"

#include "hammer/compiler/semantics/analyzer.hpp"
#include "hammer/core/defs.hpp"
#include "hammer/core/safe_int.hpp"

namespace hammer::compiler {

struct FunctionLocations::Computation {
    const NodePtr<FuncDecl>& func_;
    const SymbolTable& symbols_;
    const StringTable& strings_;
    FunctionLocations result_;

    Computation(const NodePtr<FuncDecl>& func, const SymbolTable& symbols,
        const StringTable& strings)
        : func_(func)
        , symbols_(symbols)
        , strings_(strings) {}

    void execute() {
        compute_params();
        compute_closure_scopes();
        compute_locals();
    }

    void compute_params() {
        HAMMER_ASSERT_NOT_NULL(func_);
        HAMMER_ASSERT_NOT_NULL(func_->params());

        const auto params = func_->params();
        const size_t param_count = params->size();
        HAMMER_CHECK(param_count <= std::numeric_limits<u32>::max(),
            "Too many parameters.");

        for (size_t i = 0; i < param_count; ++i) {
            const auto param = params->get(i);
            const auto entry = param->declared_symbol();
            HAMMER_ASSERT_NOT_NULL(entry);

            if (entry->captured())
                continue;

            VarLocation loc;
            loc.type = VarLocationType::Param;
            loc.param.index = static_cast<u32>(i);
            insert_location(entry, loc);
        }

        result_.params_ = static_cast<u32>(param_count);
    }

    void compute_locals() {
        HAMMER_ASSERT_NOT_NULL(func_);
        compute_locals(func_->param_scope(), 0);
    }

    void compute_locals(const ScopePtr& scope, SafeInt<u32> next_local) {
        HAMMER_ASSERT_NOT_NULL(scope);

        // Don't recurse into nested functions.
        if (scope->function() != func_)
            return;

        // Assign a local index to the closure context (if any).
        // I believe that this could be better handled at a higher level (introduce
        // a new local variable for this context), but this will do for now.
        if (auto ctx = result_.get_closure_context(scope)) {
            ctx->local_index = (next_local++).value();
        }

        // Assign a local index to every (non-captured) decl in this scope.
        for (const SymbolEntryPtr& entry : scope->entries()) {
            if (entry->captured())
                continue;

            const auto& decl = entry->decl();

            // Handled elsewhere: params are analyzed in compute_params()
            // and function decl are not assigned a local index (they
            // are compiled independently).
            if (isa<ParamDecl>(decl) || isa<FuncDecl>(decl))
                continue;

            if (!isa<VarDecl>(decl)) {
                HAMMER_ERROR("Unsupported in function: {}.",
                    to_string(decl->type())); // TODO local function, class
            }

            VarLocation loc;
            loc.type = VarLocationType::Local;
            loc.local.index = (next_local++).value();
            insert_location(entry, loc);
        }
        result_.locals_ = std::max(next_local.value(), result_.locals_);

        // Nested scopes start with the current "next_local" value.
        // Sibling scopes will reuse locals!
        for (const ScopePtr& child : scope->children()) {
            compute_locals(child, next_local);
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
        compute_closure_scopes(func_->param_scope(), nullptr);
    }

    void
    compute_closure_scopes(const ScopePtr& top_scope, ClosureContext* parent) {
        HAMMER_ASSERT_NOT_NULL(top_scope);

        // Can be grouped into a single closure context allocation
        std::vector<ScopePtr> flattened_scopes;
        // Need new closure context allocations (e.g. loop body)
        std::vector<ScopePtr> nested_scopes;

        gather_flattened_closure_scopes(
            top_scope, flattened_scopes, nested_scopes);

        ClosureContext* new_context = nullptr;
        SafeInt<u32> captured_variables = 0;
        for (const ScopePtr& scope : flattened_scopes) {
            for (const SymbolEntryPtr& entry : scope->entries()) {
                if (!entry->captured())
                    continue;

                const auto& decl = entry->decl();
                // Cannot handle other variable types right now.
                if (!isa<VarDecl>(decl) && !isa<ParamDecl>(decl)) {
                    HAMMER_ERROR(
                        "Unsupported captured declaration in function: {}.",
                        to_string(decl->type()));
                }

                if (!new_context) {
                    new_context = add_closure_context(top_scope, parent);
                }

                VarLocation loc;
                loc.type = VarLocationType::Context;
                loc.context.ctx = new_context;
                loc.context.index = (captured_variables++).value();
                insert_location(entry, loc);
            }
        }

        if (new_context)
            new_context->size = captured_variables.value();

        for (const ScopePtr& nested_scope : nested_scopes) {
            compute_closure_scopes(
                nested_scope, new_context ? new_context : parent);
        }
    }

    void gather_flattened_closure_scopes(const ScopePtr& parent,
        std::vector<ScopePtr>& flattened_scopes,
        std::vector<ScopePtr>& nested_scopes) {

        HAMMER_ASSERT_NOT_NULL(parent);
        HAMMER_ASSERT(parent->function() == func_,
            "Parent must point into this function.");

        flattened_scopes.push_back(parent);
        for (auto child : parent->children()) {
            if (child->function() != func_)
                continue;

            // Loop bodies must start their own closure context, because their body can be executed multiple times.
            // Each iteration's variables are different and must not share locations, in case they are captured.
            if (child->type() == ScopeType::LoopBody) {
                nested_scopes.push_back(child);
                continue;
            }

            gather_flattened_closure_scopes(
                child, flattened_scopes, nested_scopes);
        }
    }

    ClosureContext*
    add_closure_context(const ScopePtr& scope, ClosureContext* parent) {
        HAMMER_ASSERT_NOT_NULL(scope);

        if (result_.closure_contexts_.count(scope)) {
            HAMMER_ERROR(
                "There is already a closure context "
                "associated with that scope.");
        }

        auto result = result_.closure_contexts_.emplace(
            scope, ClosureContext(parent, func_));

        ClosureContext* context = &result.first->second;
        return context;
    }

    void insert_location(const SymbolEntryPtr& entry, VarLocation loc) {
        HAMMER_ASSERT_NOT_NULL(entry);
        HAMMER_ASSERT(result_.locations_.count(entry) == 0,
            "Location for this declaration was already computed.");
        result_.locations_.emplace(entry, loc);
    }
};

FunctionLocations FunctionLocations::compute(const NodePtr<FuncDecl>& func,
    const SymbolTable& symbols, const StringTable& strings) {
    HAMMER_ASSERT_NOT_NULL(func);

    Computation comp(func, symbols, strings);
    comp.execute();
    return std::move(comp.result_);
}

std::optional<VarLocation>
FunctionLocations::get_location(const SymbolEntryPtr& entry) const {
    HAMMER_ASSERT_NOT_NULL(entry);
    if (auto pos = locations_.find(entry); pos != locations_.end()) {
        return pos->second;
    }

    return {};
}

ClosureContext* FunctionLocations::get_closure_context(const ScopePtr& scope) {
    HAMMER_ASSERT_NOT_NULL(scope);

    if (auto pos = closure_contexts_.find(scope);
        pos != closure_contexts_.end()) {
        return &pos->second;
    }

    return nullptr;
}

} // namespace hammer::compiler
