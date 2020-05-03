#include "tiro/ir_gen/gen_func.hpp"

#include "tiro/core/fix.hpp"
#include "tiro/core/scope.hpp"
#include "tiro/ir/dead_code_elimination.hpp"
#include "tiro/ir/function.hpp"
#include "tiro/ir_gen/gen_expr.hpp"
#include "tiro/ir_gen/gen_func.hpp"
#include "tiro/ir_gen/gen_module.hpp"
#include "tiro/ir_gen/gen_rvalue.hpp"
#include "tiro/ir_gen/gen_stmt.hpp"
#include "tiro/semantics/symbol_table.hpp"

#include <unordered_map>
#include <vector>

namespace tiro {

ExprResult
CurrentBlock::compile_expr(NotNull<Expr*> expr, ExprOptions options) {
    return ctx_.compile_expr(expr, *this, options);
}

StmtResult CurrentBlock::compile_stmt(NotNull<ASTStmt*> stmt) {
    return ctx_.compile_stmt(stmt, *this);
}

StmtResult CurrentBlock::compile_loop_body(NotNull<Expr*> body,
    NotNull<Scope*> loop_scope, BlockId breakId, BlockId continueId) {
    return ctx_.compile_loop_body(body, loop_scope, breakId, continueId, *this);
}

LocalId CurrentBlock::compile_reference(NotNull<Symbol*> symbol) {
    return ctx_.compile_reference(symbol, id_);
}

void CurrentBlock::compile_assign(const AssignTarget& target, LocalId value) {
    return ctx_.compile_assign(target, value, id_);
}

void CurrentBlock::compile_assign(NotNull<Symbol*> symbol, LocalId value) {
    return ctx_.compile_assign(symbol, value, id_);
}

void CurrentBlock::compile_assign(const LValue& lvalue, LocalId value) {
    ctx_.compile_assign(lvalue, value, id_);
}

LocalId CurrentBlock::compile_env(ClosureEnvId env) {
    return ctx_.compile_env(env, id_);
}

LocalId CurrentBlock::compile_rvalue(const RValue& value) {
    return ctx_.compile_rvalue(value, id_);
}

LocalId CurrentBlock::define_new(const RValue& value) {
    return ctx_.define_new(value, id_);
}

LocalId CurrentBlock::memoize_value(
    const ComputedValue& key, FunctionRef<LocalId()> compute) {
    return ctx_.memoize_value(key, compute, id_);
}

void CurrentBlock::seal() {
    return ctx_.seal(id_);
}

void CurrentBlock::end(const Terminator& term) {
    return ctx_.end(term, id_);
}

FunctionIRGen::FunctionIRGen(ModuleIRGen& module,
    NotNull<ClosureEnvCollection*> envs, ClosureEnvId closure_env,
    Function& result, Diagnostics& diag, StringTable& strings)
    : module_(module)
    , envs_(envs)
    , outer_env_(closure_env)
    , result_(result)
    , diag_(diag)
    , strings_(strings) {}

const LoopContext* FunctionIRGen::current_loop() const {
    return active_loops_.empty() ? nullptr : &active_loops_.back();
}

ClosureEnvId FunctionIRGen::current_env() const {
    if (local_env_stack_.empty()) {
        return outer_env_;
    }
    return local_env_stack_.back().env;
}

void FunctionIRGen::compile_function(NotNull<FuncDecl*> func) {
    return enter_compilation([&](CurrentBlock& bb) {
        auto param_scope = TIRO_NN(func->param_scope());
        enter_env(param_scope, bb);

        // Make sure that all parameters are available.
        {
            auto params = TIRO_NN(func->params());
            const size_t param_count = params->size();
            for (size_t i = 0; i < param_count; ++i) {
                auto symbol = TIRO_NN(
                    func->params()->get(i)->declared_symbol());

                auto paramId = result_.make(Param(symbol->name()));
                auto lvalue = LValue::make_param(paramId);
                auto localId = bb.define_new(RValue::make_use_lvalue(lvalue));
                bb.compile_assign(TIRO_NN(symbol.get()), localId);
            }
        }

        // Compile the function body
        const auto body = TIRO_NN(func->body());
        if (func->body_is_value()) {
            TIRO_DEBUG_ASSERT(can_use_as_value(body->expr_type()),
                "Function body must be a value.");
            auto local = compile_expr(body, bb);
            if (local)
                bb.end(Terminator::make_return(*local, result_.exit()));
        } else {
            if (!compile_expr(body, bb, ExprOptions::MaybeInvalid)
                     .is_unreachable()) {
                auto local = bb.compile_rvalue(Constant::make_null());
                bb.end(Terminator::make_return(local, result_.exit()));
            }
        }

        exit_env(param_scope);
    });
}

void FunctionIRGen::compile_initializer(NotNull<File*> module) {
    return enter_compilation([&](CurrentBlock& bb) {
        auto module_scope = TIRO_NN(module->file_scope());
        enter_env(module_scope, bb);

        bool reachable = true;
        for (const auto item : module->items()->entries()) {
            if (auto decl = try_cast<DeclStmt>(item)) {
                auto result = bb.compile_stmt(TIRO_NN(decl));
                if (!result) {
                    reachable = false;
                    break;
                }
            }
        }

        if (reachable) {
            auto local = bb.compile_rvalue(Constant::make_null());
            bb.end(Terminator::make_return(local, result_.exit()));
        }

        exit_env(module_scope);
    });
}

void FunctionIRGen::enter_compilation(
    FunctionRef<void(CurrentBlock& bb)> compile_body) {
    result_[result_.entry()]->sealed(true);
    result_[result_.exit()]->filled(true);

    auto bb = make_current(result_.entry());

    // Make the outer environment accessible as a local.
    if (outer_env_) {
        local_env_locations_[outer_env_] = bb.define_new(
            RValue::OuterEnvironment{});
    }

    compile_body(bb);

    TIRO_DEBUG_ASSERT(
        result_[bb.id()]->terminator().type() == TerminatorType::Return,
        "The last block must perform a return.");
    TIRO_DEBUG_ASSERT(
        result_[bb.id()]->terminator().as_return().target == result_.exit(),
        "The last block at function level must always return to the exit "
        "block.");

    TIRO_DEBUG_ASSERT(
        active_loops_.empty(), "No active loops must be left behind.");
    TIRO_DEBUG_ASSERT(local_env_stack_.empty(),
        "No active environments must be left behind.");
    seal(result_.exit());

    eliminate_dead_code(result_);
}

ExprResult FunctionIRGen::compile_expr(
    NotNull<Expr*> expr, CurrentBlock& bb, ExprOptions options) {

    ExprIRGen gen(*this, options, bb);
    auto result = gen.dispatch(expr);
    if (result && !has_options(options, ExprOptions::MaybeInvalid)) {
        TIRO_DEBUG_ASSERT(result.value().valid(),
            "Expression transformation must return a valid local in this "
            "context.");
    }

    return result;
}

StmtResult
FunctionIRGen::compile_stmt(NotNull<ASTStmt*> stmt, CurrentBlock& bb) {
    StmtIRGen transformer(*this, bb);
    return transformer.dispatch(stmt);
}

StmtResult FunctionIRGen::compile_loop_body(NotNull<Expr*> body,
    NotNull<Scope*> loop_scope, BlockId breakId, BlockId continueId,
    CurrentBlock& bb) {
    active_loops_.push_back(LoopContext{breakId, continueId});
    ScopeExit clean_loop = [&]() {
        TIRO_DEBUG_ASSERT(!active_loops_.empty(),
            "Corrupted active loop stack: must not be empty.");
        TIRO_DEBUG_ASSERT(
            active_loops_.back().jump_break == breakId
                && active_loops_.back().jump_continue == continueId,
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

LocalId
FunctionIRGen::compile_reference(NotNull<Symbol*> symbol, BlockId blockId) {
    // TODO: Values of module level constants (imports, const variables can be cached as locals).
    if (auto lvalue = find_lvalue(symbol)) {
        auto local_id = compile_rvalue(
            RValue::make_use_lvalue(*lvalue), blockId);

        // Apply name if possible:
        auto local = result()[local_id];
        if (!local->name())
            local->name(symbol->name());

        return local_id;
    }

    return read_variable(symbol, blockId);
}

void FunctionIRGen::compile_assign(
    const AssignTarget& target, LocalId value, BlockId blockId) {
    switch (target.type()) {
    case AssignTargetType::LValue:
        return compile_assign(target.as_lvalue(), value, blockId);
    case AssignTargetType::Symbol:
        return compile_assign(target.as_symbol(), value, blockId);
    }

    TIRO_UNREACHABLE("Invalid assignment target type.");
}

void FunctionIRGen::compile_assign(
    NotNull<Symbol*> symbol, LocalId value, BlockId blockId) {
    auto local = result_[value];
    if (!local->name()) {
        local->name(symbol->name());
    }

    if (auto lvalue = find_lvalue(symbol)) {
        emit(Stmt::make_assign(*lvalue, value), blockId);
        return;
    }

    write_variable(symbol, value, blockId);
}

void FunctionIRGen::compile_assign(
    const LValue& lvalue, LocalId value, BlockId blockId) {
    auto stmt = Stmt::make_assign(lvalue, value);
    emit(stmt, blockId);
}

LocalId
FunctionIRGen::compile_env(ClosureEnvId env, [[maybe_unused]] BlockId block) {
    TIRO_DEBUG_ASSERT(env, "Closure environment to be compiled must be valid.");
    return get_env(env);
}

LocalId FunctionIRGen::compile_rvalue(const RValue& value, BlockId blockId) {
    RValueIRGen gen(*this, blockId);
    auto local = gen.compile(value);
    TIRO_DEBUG_ASSERT(local, "Compiled rvalues must produce valid locals.");
    return local;
}

BlockId FunctionIRGen::make_block(InternedString label) {
    return result_.make(Block(label));
}

LocalId FunctionIRGen::define_new(const RValue& value, BlockId blockId) {
    return define_new(Local(value), blockId);
}

LocalId FunctionIRGen::define_new(const Local& local, BlockId blockId) {
    auto id = result_.make(local);
    emit(Stmt::make_define(id), blockId);
    return id;
}

LocalId FunctionIRGen::memoize_value(
    const ComputedValue& key, FunctionRef<LocalId()> compute, BlockId blockId) {
    const auto value_key = std::tuple(key, blockId);

    if (auto pos = values_.find(value_key); pos != values_.end())
        return pos->second;

    const auto local = compute();
    TIRO_DEBUG_ASSERT(
        local, "The result of compute() must be a valid local id.");
    values_[value_key] = local;
    return local;
}

void FunctionIRGen::seal(BlockId blockId) {
    auto block = result_[blockId];
    TIRO_DEBUG_ASSERT(!block->sealed(), "Block was already sealed.");

    // Patch incomplete phis. See [BB+13], Section 2.3.
    if (auto pos = incomplete_phis_.find(blockId);
        pos != incomplete_phis_.end()) {

        auto& phis = pos->second;
        for (const auto& [symbol, phi] : phis) {
            add_phi_operands(symbol, phi, blockId);
        }

        incomplete_phis_.erase(pos);
    }

    block->sealed(true);
}

void FunctionIRGen::emit(const Stmt& stmt, BlockId blockId) {
    auto block = result_[blockId];

    // Insertions are forbidden once a block is filled. The exception are phi nodes
    // inserted by the variable resolution algorithm (triggered by read_variable).
    TIRO_DEBUG_ASSERT(!block->filled() || is_phi_define(result_, stmt),
        "Cannot emit a statement into a filled block.");

    // Cluster phi nodes at the start of the block.
    if (is_phi_define(result_, stmt)) {
        block->insert_stmt(block->phi_count(result_), stmt);
    } else {
        block->append_stmt(stmt);
    }
}

void FunctionIRGen::end(const Terminator& term, BlockId blockId) {
    TIRO_DEBUG_ASSERT(
        term.type() != TerminatorType::None, "Invalid terminator.");

    // Cannot add instructions after the terminator has been set.
    auto block = result_[blockId];
    if (!block->filled())
        block->filled(true);

    TIRO_DEBUG_ASSERT(block->terminator().type() == TerminatorType::None,
        "Block already has a terminator.");
    block->terminator(term);

    visit_targets(term, [&](BlockId targetId) {
        auto target = result_[targetId];
        TIRO_DEBUG_ASSERT(
            !target->sealed(), "Cannot add incoming edges to sealed blocks.");
        target->append_predecessor(blockId);
    });
}

void FunctionIRGen::write_variable(
    NotNull<Symbol*> var, LocalId value, BlockId blockId) {
    variables_[std::tuple(var.get(), blockId)] = value;
}

LocalId FunctionIRGen::read_variable(NotNull<Symbol*> var, BlockId blockId) {
    if (auto pos = variables_.find(std::tuple(var.get(), blockId));
        pos != variables_.end()) {
        return pos->second;
    }
    return read_variable_recursive(var, blockId);
}

LocalId
FunctionIRGen::read_variable_recursive(NotNull<Symbol*> var, BlockId blockId) {
    auto block = result_[blockId];

    LocalId value;
    if (!block->sealed()) {
        auto local = Local(RValue::make_phi0());
        local.name(var->name());
        value = define_new(local, blockId);
        incomplete_phis_[blockId].emplace_back(var, value);
    } else if (block->predecessor_count() == 1) {
        value = read_variable(var, block->predecessor(0));
    } else if (block->predecessor_count() == 0) {
        TIRO_DEBUG_ASSERT(blockId == result_.entry(),
            "Only the entry block has 0 predecessors.");
        TIRO_ERROR("Undefined variable: {}.", strings().dump(var->name()));
    } else {
        // Place a phi marker to break the recursion.
        // Recursive calls to read_variable will observe the Phi0 node.
        auto local = Local(RValue::make_phi0());
        local.name(var->name());
        value = define_new(local, blockId);
        write_variable(var, value, blockId);

        // Recurse into predecessor blocks.
        add_phi_operands(var, value, blockId);
    }

    write_variable(var, value, blockId);
    return value;
}

void FunctionIRGen::add_phi_operands(
    NotNull<Symbol*> var, LocalId value, BlockId blockId) {
    auto block = result_[blockId];

    // Collect the possible operands from all predecessors. Note that, because
    // of recursion, the list of operands may contain the local value itself.
    // TODO: Small vector
    std::vector<LocalId> operands;
    for (auto pred : block->predecessors()) {
        operands.push_back(read_variable(var, pred));
    }

    // Do not emit trivial phi nodes. A phi node is trivial iff its list of operands
    // only contains itself and at most one other value.
    //
    // TODO: Complete removal of nodes that turn out to be trivial is not yet implemented (requires
    // def-use tracking to replace uses).
    bool is_trivial = true;
    LocalId trivial_other;
    {
        for (const auto& operand : operands) {
            TIRO_DEBUG_ASSERT(operand, "Invalid operand to phi node.");

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
            TIRO_ERROR("Variable {} was never initialized.",
                strings().dump(var->name()));
        }

        // TODO: Remove uses of this phi that might have become trivial. See Algorithm 3 in [BB+13].
        block->remove_phi(
            result_, value, RValue::make_use_local(trivial_other));
        return;
    }

    // Emit a phi node.
    auto phi_id = result_.make(Phi(std::move(operands)));
    result_[value]->value(RValue::make_phi(phi_id));
}

static bool can_open_closure_env(ScopeType type) {
    switch (type) {
    case ScopeType::File: // For module initializers (TODO: Module scope)
    case ScopeType::Parameters:
    case ScopeType::LoopBody:
        return true;
    default:
        return false;
    }
}

void FunctionIRGen::enter_env(NotNull<Scope*> parent_scope, CurrentBlock& bb) {
    TIRO_DEBUG_ASSERT(
        can_open_closure_env(parent_scope->type()), "Invalid scope type.");

    std::vector<NotNull<Symbol*>> captured; // TODO small vec
    Fix gather_captured = [&](auto& self, NotNull<Scope*> scope) {
        if (scope != parent_scope && can_open_closure_env(scope->type()))
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
    const ClosureEnvId parent = current_env();
    const ClosureEnvId env = envs_->make(
        ClosureEnv(parent, checked_cast<u32>(captured.size())));
    for (u32 i = 0; i < captured.size(); ++i) {
        envs_->write_location(captured[i], ClosureEnvLocation(env, i));
    }

    const auto parent_local = parent ? get_env(parent)
                                     : bb.compile_rvalue(Constant::make_null());
    const auto env_local = bb.compile_rvalue(
        RValue::make_make_environment(parent_local, captured_count));
    local_env_stack_.push_back({env, parent_scope});
    local_env_locations_[env] = env_local;
}

void FunctionIRGen::exit_env(NotNull<Scope*> parent_scope) {
    TIRO_DEBUG_ASSERT(
        can_open_closure_env(parent_scope->type()), "Invalid scope type.");

    if (local_env_stack_.empty()
        || local_env_stack_.back().starter != parent_scope)
        return;

    const auto env = local_env_stack_.back().env;
    local_env_stack_.pop_back();
    local_env_locations_.erase(env);
}

std::optional<LocalId> FunctionIRGen::find_env(ClosureEnvId env) {
    auto pos = local_env_locations_.find(env);
    if (pos != local_env_locations_.end())
        return pos->second;
    return {};
}

LocalId FunctionIRGen::get_env(ClosureEnvId env) {
    auto local = find_env(env);
    TIRO_DEBUG_ASSERT(local, "Local environment was not found.");
    return *local;
}

std::optional<LValue> FunctionIRGen::find_lvalue(NotNull<Symbol*> symbol) {
    const auto scope = symbol->scope();
    if (scope->type() == ScopeType::File) { // TODO module
        auto member = module_.find_symbol(symbol);
        TIRO_DEBUG_ASSERT(member, "Failed to find member in module.");
        return LValue::make_module(member);
    }

    if (symbol->captured()) {
        auto pos = envs_->read_location(symbol);
        TIRO_DEBUG_ASSERT(
            pos, "Captured symbol without a defined location used as lvalue.");
        return get_captured_lvalue(*pos);
    }

    return {};
}

LValue FunctionIRGen::get_captured_lvalue(const ClosureEnvLocation& loc) {
    TIRO_DEBUG_ASSERT(loc.env, "Must have a valid environment id.");

    const auto& envs = *envs_;
    const auto target_id = loc.env;
    const auto& target = *envs[target_id];
    TIRO_DEBUG_ASSERT(loc.index < target.size(),
        "Index into closure environment is out of bounds.");

    // Simple case for closure environments created by this function.
    if (auto local = find_env(target_id)) {
        return LValue::make_closure(*local, 0, loc.index);
    }

    // Try to reach the target environment by moving upwards from the outer env.
    auto current_id = outer_env_;
    u32 levels = 0;
    while (current_id) {
        if (current_id == target_id) {
            const auto outer_local = find_env(outer_env_);
            TIRO_DEBUG_ASSERT(outer_local,
                "The outer environment must be stored in a local.");
            return LValue::make_closure(*outer_local, levels, loc.index);
        }

        const auto& current = *envs[current_id];
        current_id = current.parent();
        levels += 1;
    }

    TIRO_ERROR(
        "Failed to access a captured variable through the chain of closure "
        "environments.");
}

} // namespace tiro
