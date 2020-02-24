#include "tiro/mir/transform_func.hpp"

#include "tiro/core/fix.hpp"
#include "tiro/core/scope.hpp"
#include "tiro/mir/transform_expr.hpp"
#include "tiro/mir/transform_func.hpp"
#include "tiro/mir/transform_module.hpp"
#include "tiro/mir/transform_stmt.hpp"
#include "tiro/mir/types.hpp"
#include "tiro/semantics/symbol_table.hpp"

#include <unordered_map>
#include <vector>

namespace tiro::compiler::mir_transform {

[[maybe_unused]] static bool
is_phi_define(const mir::Function& func, const mir::Stmt& stmt) {
    if (stmt.type() != mir::StmtType::Define)
        return false;

    const auto& def = stmt.as_define();
    if (!def.local)
        return false;

    auto local = func[def.local];
    auto type = local->value().type();
    return type == mir::RValueType::Phi || type == mir::RValueType::Phi0;
}

ExprResult
CurrentBlock::compile_expr(NotNull<Expr*> expr, ExprOptions options) {
    return ctx_.compile_expr(expr, *this, options);
}

StmtResult CurrentBlock::compile_stmt(NotNull<Stmt*> stmt) {
    return ctx_.compile_stmt(stmt, *this);
}

StmtResult CurrentBlock::compile_loop_body(NotNull<Expr*> body,
    NotNull<Scope*> loop_scope, mir::BlockID breakID, mir::BlockID continueID) {
    return ctx_.compile_loop_body(body, loop_scope, breakID, continueID, *this);
}

mir::LocalID CurrentBlock::compile_reference(NotNull<Symbol*> symbol) {
    return ctx_.compile_reference(symbol, id_);
}

void CurrentBlock::compile_assign(NotNull<Symbol*> symbol, mir::LocalID value) {
    return ctx_.compile_assign(symbol, value, id_);
}

mir::LocalID CurrentBlock::compile_env(ClosureEnvID env) {
    return ctx_.compile_env(env, id_);
}

mir::LocalID CurrentBlock::define(const mir::Local& local) {
    return ctx_.define(local, id_);
}

void CurrentBlock::seal() {
    return ctx_.seal(id_);
}

void CurrentBlock::emit(const mir::Stmt& stmt) {
    return ctx_.emit(stmt, id_);
}

void CurrentBlock::end(const mir::Edge& edge) {
    return ctx_.end(edge, id_);
}

FunctionContext::FunctionContext(ModuleContext& module,
    NotNull<ClosureEnvCollection*> envs, ClosureEnvID closure_env,
    mir::Function& result, StringTable& strings)
    : module_(module)
    , envs_(envs)
    , outer_env_(closure_env)
    , result_(result)
    , strings_(strings) {}

const LoopContext* FunctionContext::current_loop() const {
    return active_loops_.empty() ? nullptr : &active_loops_.back();
}

ClosureEnvID FunctionContext::current_env() const {
    if (local_env_stack_.empty()) {
        return outer_env_;
    }
    return local_env_stack_.back().env;
}

void FunctionContext::compile_function(NotNull<FuncDecl*> func) {
    result_[result_.entry()]->sealed(true);
    result_[result_.exit()]->filled(true);

    auto bb = make_current(result_.entry());

    // Make the outer environment accessible as a local.
    if (outer_env_) {
        local_env_locations_[outer_env_] = bb.define(
            mir::Local(mir::RValue::OuterEnvironment{}));
    }

    const auto scope = TIRO_NN(func->param_scope());
    enter_env(scope, bb);

    // Make sure that all parameters are available.
    {
        auto params = TIRO_NN(func->params());
        const size_t param_count = params->size();
        for (size_t i = 0; i < param_count; ++i) {
            auto symbol = TIRO_NN(func->params()->get(i)->declared_symbol());

            // TODO: Only emit parameter reads that are actually used? Could tell by inspecting
            // the symbol usages.
            auto paramID = result_.make(mir::Param(symbol->name()));
            auto lvalue = mir::LValue::make_param(paramID);
            auto localID = bb.define(
                mir::Local(mir::RValue::make_use_lvalue(lvalue)));
            bb.compile_assign(TIRO_NN(symbol.get()), localID);
        }
    }

    // Compile the function body
    NotNull<Expr*> body = TIRO_NN(func->body());
    if (body->expr_type() == ExprType::Value) {
        auto local = compile_expr(body, bb);
        if (local)
            bb.end(mir::Edge::make_return(*local, result_.exit()));
    } else {
        if (!compile_expr(body, bb, ExprOptions::MaybeInvalid)
                 .is_unreachable()) {
            auto rvalue = mir::RValue(mir::Constant::Null{});
            auto local = bb.define(mir::Local(rvalue));
            bb.end(mir::Edge::make_return(local, result_.exit()));
        }
    }
    exit_env(scope);

    TIRO_ASSERT(result_[bb.id()]->edge().type() == mir::EdgeType::Return,
        "The last block must perform a return.");
    TIRO_ASSERT(result_[bb.id()]->edge().as_return().target == result_.exit(),
        "The last block at function level must always return to the exit "
        "block.");

    TIRO_ASSERT(active_loops_.empty(), "No active loops must be left behind.");
    TIRO_ASSERT(local_env_stack_.empty(),
        "No active environments must be left behind.");
    seal(result_.exit());
}

ExprResult FunctionContext::compile_expr(
    NotNull<Expr*> expr, CurrentBlock& bb, ExprOptions options) {
    ExprTransformer transformer(*this, bb);

    auto result = transformer.dispatch(expr);
    if (result && !has_options(options, ExprOptions::MaybeInvalid)) {
        TIRO_ASSERT(result.value().valid(),
            "Expression transformation must return a valid local in this "
            "context.");
    }

    return result;
}

StmtResult
FunctionContext::compile_stmt(NotNull<Stmt*> stmt, CurrentBlock& bb) {
    StmtTransformer transformer(*this, bb);
    return transformer.dispatch(stmt);
}

StmtResult FunctionContext::compile_loop_body(NotNull<Expr*> body,
    NotNull<Scope*> loop_scope, mir::BlockID breakID, mir::BlockID continueID,
    CurrentBlock& bb) {

    active_loops_.push_back(LoopContext{breakID, continueID});
    ScopeExit clean_loop = [&]() {
        TIRO_ASSERT(!active_loops_.empty(),
            "Corrupted active loop stack: must not be empty.");
        TIRO_ASSERT(active_loops_.back().jump_break == breakID
                        && active_loops_.back().jump_continue == continueID,
            "Corrupted active loop stack: unexpected top content.");
        active_loops_.pop_back();
    };

    enter_env(loop_scope, bb);
    ScopeExit clean_env = [&]() { exit_env(loop_scope); };

    auto result = compile_expr(body, bb, ExprOptions::MaybeInvalid);
    if (!result)
        return result.failure();
    return ok;
}

mir::LocalID FunctionContext::compile_reference(
    NotNull<Symbol*> symbol, mir::BlockID blockID) {
    if (auto lvalue = find_lvalue(symbol)) {
        auto local = mir::Local(mir::RValue::make_use_lvalue(*lvalue));
        local.name(symbol->name());
        return define(local, blockID);
    }

    return read_variable(symbol, blockID);
}

void FunctionContext::compile_assign(
    NotNull<Symbol*> symbol, mir::LocalID value, mir::BlockID blockID) {
    auto local = result_[value];
    if (!local->name()) {
        local->name(symbol->name());
    }

    if (auto lvalue = find_lvalue(symbol)) {
        emit(mir::Stmt::make_assign(*lvalue, value), blockID);
        return;
    }

    write_variable(symbol, value, blockID);
}

mir::LocalID FunctionContext::compile_env(
    ClosureEnvID env, [[maybe_unused]] mir::BlockID block) {
    TIRO_ASSERT(env, "Closure environment to be compiled must be valid.");
    return get_env(env);
}

mir::BlockID FunctionContext::make_block(InternedString label) {
    return result_.make(mir::Block(label));
}

mir::LocalID
FunctionContext::define(const mir::Local& local, mir::BlockID blockID) {
    if (local.value().type() == mir::RValueType::UseLocal) {
        // Omit the useless define and use the right hand side local directly.
        return local.value().as_use_local().target;
    }

    auto id = result_.make(local);
    emit(mir::Stmt::make_define(id), blockID);
    return id;
}

void FunctionContext::seal(mir::BlockID blockID) {
    auto block = result_[blockID];
    TIRO_ASSERT(!block->sealed(), "Block was already sealed.");

    // Patch incomplete phis. See [BB+13], Section 2.3.
    if (auto pos = incomplete_phis_.find(blockID);
        pos != incomplete_phis_.end()) {

        auto& phis = pos->second;
        for (const auto& [symbol, phi] : phis) {
            add_phi_operands(symbol, phi, blockID);
        }

        incomplete_phis_.erase(pos);
    }

    block->sealed(true);
}

void FunctionContext::emit(const mir::Stmt& stmt, mir::BlockID blockID) {
    auto block = result_[blockID];

    // Insertions are forbidden once a block is filled. The exception are phi nodes
    // inserted by the variable resolution algorithm (triggered by read_variable).
    TIRO_ASSERT(!block->filled() || is_phi_define(result_, stmt),
        "Cannot emit a statement into a filled block.");
    block->append_stmt(stmt);
}

void FunctionContext::end(const mir::Edge& edge, mir::BlockID blockID) {
    TIRO_ASSERT(edge.type() != mir::EdgeType::None, "Invalid out edge.");

    // Cannot add instructions after the out-edge has been set.
    auto block = result_[blockID];
    if (!block->filled())
        block->filled(true);

    TIRO_ASSERT(block->edge().type() == mir::EdgeType::None,
        "Block already has an outgoing edge.");
    block->edge(edge);

    mir::visit_targets(edge, [&](mir::BlockID targetID) {
        auto target = result_[targetID];
        TIRO_ASSERT(
            !target->sealed(), "Cannot add incoming edges to sealed blocks.");
        target->append_predecessor(blockID);
    });
}

void FunctionContext::write_variable(
    NotNull<Symbol*> var, mir::LocalID value, mir::BlockID blockID) {
    variables_[std::tuple(var.get(), blockID)] = value;
}

mir::LocalID
FunctionContext::read_variable(NotNull<Symbol*> var, mir::BlockID blockID) {
    if (auto pos = variables_.find(std::tuple(var.get(), blockID));
        pos != variables_.end()) {
        return pos->second;
    }
    return read_variable_recursive(var, blockID);
}

mir::LocalID FunctionContext::read_variable_recursive(
    NotNull<Symbol*> var, mir::BlockID blockID) {
    auto block = result_[blockID];

    mir::LocalID value;
    if (!block->sealed()) {
        auto local = mir::Local(mir::RValue::Phi0{});
        local.name(var->name());
        value = define(local, blockID);
        incomplete_phis_[blockID].emplace_back(var, value);
    } else if (block->predecessor_count() == 1) {
        value = read_variable(var, block->predecessor(0));
    } else if (block->predecessor_count() == 0) {
        TIRO_ASSERT(blockID == result_.entry(),
            "Only the entry block has 0 predecessors.");
        TIRO_ERROR("Undefined variable: {}.", strings().dump(var->name()));
    } else {
        // Place a phi marker to break the recursion.
        // Recursive calls to read_variable will observe the Phi0 node.
        auto local = mir::Local(mir::RValue::Phi0{});
        local.name(var->name());
        value = define(local, blockID);
        write_variable(var, value, blockID);

        // Recurse into predecessor blocks.
        add_phi_operands(var, value, blockID);
    }

    write_variable(var, value, blockID);
    return value;
}

void FunctionContext::add_phi_operands(
    NotNull<Symbol*> var, mir::LocalID value, mir::BlockID blockID) {
    auto block = result_[blockID];

    // Collect the possible operands from all predecessors. Note that, because
    // of recursion, the list of operands may contain the local value itself.
    // TODO: Small vector
    std::vector<mir::LocalID> operands;
    for (auto pred : block->predecessors()) {
        operands.push_back(read_variable(var, pred));
    }

    // Do not emit trivial phi nodes. A phi node is trivial iff its list of operands
    // only contains itself and at most one other value.
    bool is_trivial = true;
    mir::LocalID trivial_other;
    {
        for (const auto& operand : operands) {
            TIRO_ASSERT(operand, "Invalid operand to phi node.");

            if (operand == trivial_other || operand == value)
                continue;

            if (trivial_other) {
                is_trivial = false;
                break;
            }

            trivial_other = operand;
        }
    }

    if (is_trivial) {
        // The value can be replaced with the other value. If there is no such value, then the variable
        // is uninitialized.
        if (!trivial_other) {
            // TODO: Emit "undefined" value instead?
            TIRO_ERROR("Variable {} was never initialized.",
                strings().dump(var->name()));
        }

        // TODO: Remove uses of this phi that might have become trivial. See Algorithm 3 in [BB+13].
        result_[value]->value(mir::RValue::make_use_local(trivial_other));
        return;
    }

    // Emit a phi node.
    // Possible improvement: Deduplicate the operand list? -> use sort and std::unique
    auto phi_id = result_.make(mir::Phi(std::move(operands)));
    result_[value]->value(mir::RValue::make_phi(phi_id));
}

void FunctionContext::enter_env(
    NotNull<Scope*> parent_scope, CurrentBlock& bb) {
    TIRO_ASSERT(parent_scope->type() == ScopeType::Parameters
                    || parent_scope->type() == ScopeType::LoopBody,
        "Invalid scope type.");

    std::vector<NotNull<Symbol*>> captured; // TODO small vec
    Fix gather_captured = [&](auto& self, NotNull<Scope*> scope) {
        if (scope != parent_scope
            && (scope->type() == ScopeType::Parameters
                || scope->type() == ScopeType::LoopBody))
            return;

        for (const auto& entry : scope->entries()) {
            if (entry->captured())
                captured.push_back(TIRO_NN(entry.get()));
        }

        for (const auto& child : scope->children()) {
            self(TIRO_NN(child.get()));
        }
    };
    gather_captured(parent_scope);

    if (captured.empty())
        return;

    const u32 captured_count = checked_cast<u32>(captured.size());
    const ClosureEnvID parent = current_env();
    const ClosureEnvID env = envs_->make(
        ClosureEnv(parent, checked_cast<u32>(captured.size())));
    for (u32 i = 0; i < captured.size(); ++i) {
        envs_->write_location(captured[i], ClosureEnvLocation(env, i));
    }

    const auto parent_local = parent ? get_env(parent)
                                     : bb.define(mir::Local(
                                         mir::RValue(mir::Constant::Null{})));
    const auto env_local = bb.define(mir::Local(
        mir::RValue::make_make_environment(parent_local, captured_count)));
    local_env_stack_.push_back({env, parent_scope});
    local_env_locations_[env] = env_local;
}

void FunctionContext::exit_env(NotNull<Scope*> parent_scope) {
    TIRO_ASSERT(parent_scope->type() == ScopeType::Parameters
                    || parent_scope->type() == ScopeType::LoopBody,
        "Invalid scope type.");

    if (local_env_stack_.empty()
        || local_env_stack_.back().starter != parent_scope)
        return;

    const auto env = local_env_stack_.back().env;
    local_env_stack_.pop_back();
    local_env_locations_.erase(env);
}

std::optional<mir::LocalID> FunctionContext::find_env(ClosureEnvID env) {
    auto pos = local_env_locations_.find(env);
    if (pos != local_env_locations_.end())
        return pos->second;
    return {};
}

mir::LocalID FunctionContext::get_env(ClosureEnvID env) {
    auto local = find_env(env);
    TIRO_ASSERT(local, "Local environment was not found.");
    return *local;
}

std::optional<mir::LValue>
FunctionContext::find_lvalue(NotNull<Symbol*> symbol) {
    const auto scope = symbol->scope();
    if (scope->type() == ScopeType::File) { // TODO module
        auto member = module_.find_symbol(symbol);
        TIRO_ASSERT(member, "Failed to find member in module.");
        return mir::LValue::make_module(member);
    }

    if (symbol->captured()) {
        auto pos = envs_->read_location(symbol);
        TIRO_ASSERT(
            pos, "Captured symbol without a defined location used as lvalue.");
        return get_captured_lvalue(*pos);
    }

    return {};
}

mir::LValue
FunctionContext::get_captured_lvalue(const ClosureEnvLocation& loc) {
    TIRO_ASSERT(loc.env, "Must have a valid environment id.");

    const auto& envs = *envs_;
    const auto target_id = loc.env;
    const auto& target = *envs[target_id];
    TIRO_ASSERT(loc.index < target.size(),
        "Index into closure environment is out of bounds.");

    // Simple case for closure environments created by this function.
    if (auto local = find_env(target_id)) {
        return mir::LValue::make_closure(*local, 0, loc.index);
    }

    // Try to reach the target environment by moving upwards from the outer env.
    auto current_id = outer_env_;
    u32 levels = 0;
    while (current_id) {
        if (current_id == target_id) {
            const auto outer_local = find_env(outer_env_);
            TIRO_ASSERT(outer_local,
                "The outer environment must be stored in a local.");
            return mir::LValue::make_closure(*outer_local, levels, loc.index);
        }

        const auto& current = *envs[current_id];
        current_id = current.parent();
        levels += 1;
    }

    TIRO_ERROR(
        "Failed to access a captured variable through the chain of closure "
        "environments.");
}

} // namespace tiro::compiler::mir_transform
