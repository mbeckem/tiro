#include "hammer/compiler/codegen/variable_locations.hpp"

#include "hammer/ast/node_visit.hpp"
#include "hammer/compiler/analyzer.hpp"
#include "hammer/compiler/codegen/codegen.hpp"

namespace hammer {

struct FunctionLocations::Computation {
    // The function were computing variable locations for.
    ast::FuncDecl* func_ = nullptr;

    FunctionLocations result_;

    void execute() {
        compute_params();
        compute_closure_scopes();
        compute_locals();
    }

    void compute_params() {
        HAMMER_ASSERT_NOT_NULL(func_);

        const size_t params = func_->param_count();
        for (size_t i = 0; i < params; ++i) {
            ast::ParamDecl* param = func_->get_param(i);
            if (param->captured())
                continue;

            VarLocation loc;
            loc.type = VarLocationType::Param;
            loc.param.index = as_u32(i);
            insert_location(param, loc);
        }

        result_.params_ = as_u32(params);
    }

    void compute_locals() {
        HAMMER_ASSERT_NOT_NULL(func_);
        compute_locals(func_, 0);
    }

    void compute_locals(ast::Node* node, u32 next_local) {
        HAMMER_ASSERT_NOT_NULL(node);

        // Don't recurse into nested functions.
        if (isa<ast::FuncDecl>(node) && node != func_)
            return;

        // Assign a local index to the closure context (if any).
        // I believe that this could be better handled at a higher level (introduce
        // a new local variable for this context), but this will do for now.
        if (auto ctx = result_.get_closure_context(node))
            ctx->local_index = next_u32(next_local, "too many locals.");

        // Assign a local index to every (non-captured) decl in this scope.
        if (ast::Scope* scope = Analyzer::as_scope(*node)) {
            visit_non_captured_variables(scope, [&](ast::Decl* decl) {
                if (isa<ast::ParamDecl>(decl))
                    return; // Handled in compute_params()

                if (!isa<ast::VarDecl>(decl)) {
                    HAMMER_ERROR(
                        "Unsupported local declaration in function: {}.",
                        to_string(decl->kind()));
                }

                VarLocation loc;
                loc.type = VarLocationType::Local;
                loc.local.index = next_u32(next_local, "too many locals");
                insert_location(decl, loc);
            });
        }
        result_.locals_ = std::max(next_local, result_.locals_);

        // Nested scopes start with the current "next_local" value.
        // Sibling scopes will reuse locals!
        for (ast::Node& child : node->children()) {
            compute_locals(&child, next_local);
        }
    }

    // Visit all scopes and identify variables that are captured by nested functions.
    // These variables must not be allocated as locals but must instead be allocated
    // on the heap, inside a closure context. This approach ensures that nested function
    // can continue to reference the captured variables, even after the outer function
    // has already finished executing.
    //
    // Not every scope gets its own closure context (that would introduce too many allocations).
    // Instead, closure scopes are grouped are are only allocated when necessary (function scope,
    // loop scope).
    void compute_closure_scopes() { compute_closure_scopes(func_, nullptr); }

    void compute_closure_scopes(ast::Node* starter, ClosureContext* parent) {
        HAMMER_ASSERT_NOT_NULL(starter);

        std::vector<ast::Scope*> flattened_scopes;
        std::vector<ast::Node*> nested_children;
        gather_flattened_closure_scopes(
            starter, flattened_scopes, nested_children);

        ClosureContext* new_context = nullptr;
        for (ast::Scope* scope : flattened_scopes) {
            visit_captured_variables(scope, [&](ast::Decl* decl) {
                // Cannot handle other variable types right now.
                if (!isa<ast::VarDecl>(decl) && !isa<ast::ParamDecl>(decl)) {
                    HAMMER_ERROR(
                        "Unsupported captured declaration in function: {}.",
                        to_string(decl->kind()));
                }

                if (!new_context) {
                    new_context = add_closure_context(starter, parent);
                }

                VarLocation loc;
                loc.type = VarLocationType::Context;
                loc.context.ctx = new_context;
                loc.context.index = next_u32(
                    new_context->size, "too many captured variables");
                insert_location(decl, loc);
            });
        }

        for (ast::Node* nested_node : nested_children) {
            compute_closure_scopes(
                nested_node, new_context ? new_context : parent);
        }
    }

    void gather_flattened_closure_scopes(ast::Node* node,
        std::vector<ast::Scope*>& flattened_scopes,
        std::vector<ast::Node*>& nested_children) {

        HAMMER_ASSERT_NOT_NULL(node);
        if (isa<ast::FuncDecl>(node) && node != func_)
            return;

        if (ast::Scope* scope = Analyzer::as_scope(*node)) {
            flattened_scopes.push_back(scope);
        }

        // Loop bodies must start their own closure context, because their body can be executed multiple times.
        // Each iteration's variables are different and must not share locations, in case they are captured.
        //
        // Must be kept in sync with loop types!
        // FIXME: Lower all loops to the same type.
        ast::Node* body_child = nullptr;
        if (auto* while_stmt = try_cast<ast::WhileStmt>(node)) {
            body_child = while_stmt->body();
        } else if (auto* for_stmt = try_cast<ast::ForStmt>(node)) {
            body_child = for_stmt->body();
        }

        if (body_child)
            nested_children.push_back(body_child);

        for (ast::Node& child : node->children()) {
            // Recurse into children that are not the body of a loop.
            if (&child != body_child)
                gather_flattened_closure_scopes(
                    &child, flattened_scopes, nested_children);
        }
    }

    ClosureContext*
    add_closure_context(ast::Node* starter, ClosureContext* parent) {
        HAMMER_ASSERT_NOT_NULL(starter);

        if (result_.closure_contexts_.count(starter)) {
            HAMMER_ERROR(
                "There is already a closure context "
                "associated with that node.");
        }

        auto result = result_.closure_contexts_.emplace(
            starter, ClosureContext(parent, func_));

        ClosureContext* context = &result.first->second;
        return context;
    }

    void insert_location(ast::Decl* decl, VarLocation loc) {
        HAMMER_ASSERT_NOT_NULL(decl);
        HAMMER_ASSERT(result_.locations_.count(decl) == 0,
            "Location for this declaration was already computed.");

        result_.locations_.emplace(decl, loc);
    }

    template<typename Func>
    void visit_variables(ast::Scope* scope, Func&& fn) {
        HAMMER_ASSERT_NOT_NULL(scope);

        auto is_variable = [](ast::Decl* d) {
            return isa<ast::ParamDecl>(d) || isa<ast::VarDecl>(d);
        };

        for (ast::Decl* d : scope->declarations()) {
            if (is_variable(d))
                fn(d);
        }

        for (ast::Decl* d : scope->anon_declarations()) {
            if (is_variable(d))
                fn(d);
        }
    }

    template<typename Func>
    void visit_non_captured_variables(ast::Scope* scope, Func&& fn) {
        visit_variables(scope, [&](ast::Decl* decl) {
            HAMMER_ASSERT_NOT_NULL(decl);
            if (!decl->captured())
                fn(decl);
        });
    }

    template<typename Func>
    void visit_captured_variables(ast::Scope* scope, Func&& fn) {
        visit_variables(scope, [&](ast::Decl* decl) {
            HAMMER_ASSERT_NOT_NULL(decl);
            if (decl->captured())
                fn(decl);
        });
    }
};

FunctionLocations FunctionLocations::compute(ast::FuncDecl* func) {
    HAMMER_ASSERT_NOT_NULL(func);

    Computation comp;
    comp.func_ = func;
    comp.execute();
    return std::move(comp.result_);
}

std::optional<VarLocation>
FunctionLocations::get_location(ast::Decl* decl) const {
    HAMMER_ASSERT_NOT_NULL(decl);
    if (auto pos = locations_.find(decl); pos != locations_.end()) {
        return pos->second;
    }

    return {};
}

ClosureContext* FunctionLocations::get_closure_context(ast::Node* starter) {
    HAMMER_ASSERT_NOT_NULL(starter);

    if (auto pos = closure_contexts_.find(starter);
        pos != closure_contexts_.end()) {
        return &pos->second;
    }

    return nullptr;
}

} // namespace hammer
