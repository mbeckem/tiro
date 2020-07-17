#include "compiler/ir_gen/func.hpp"

#include "common/fix.hpp"
#include "common/scope.hpp"
#include "compiler/ast/ast.hpp"
#include "compiler/ir/dead_code_elimination.hpp"
#include "compiler/ir/function.hpp"
#include "compiler/ir_gen/compile.hpp"
#include "compiler/ir_gen/func.hpp"
#include "compiler/ir_gen/module.hpp"
#include "compiler/semantics/symbol_table.hpp"
#include "compiler/semantics/type_table.hpp"

#include <vector>

namespace tiro {

[[maybe_unused]] static bool is_error(const Function& func, const Stmt& stmt) {
    if (stmt.type() == StmtType::Define) {
        auto local_id = stmt.as_define().local;
        return func[local_id]->value().type() == RValueType::Error;
    }
    return false;
}

LocalResult CurrentBlock::compile_expr(NotNull<AstExpr*> expr, ExprOptions options) {
    return tiro::compile_expr(expr, options, *this);
}

OkResult CurrentBlock::compile_stmt(NotNull<AstStmt*> stmt) {
    return tiro::compile_stmt(stmt, *this);
}

LocalId CurrentBlock::compile_rvalue(const RValue& rvalue) {
    return tiro::compile_rvalue(rvalue, *this);
}

OkResult CurrentBlock::compile_loop_body(ScopeId body_scope_id,
    FunctionRef<OkResult()> compile_body, BlockId break_id, BlockId continue_id) {
    return ctx_.compile_loop_body(body_scope_id, compile_body, break_id, continue_id, *this);
}

void CurrentBlock::compile_assign(const AssignTarget& target, LocalId value) {
    return ctx_.compile_assign(target, value, *this);
}

LocalId CurrentBlock::compile_read(const AssignTarget& target) {
    return ctx_.compile_read(target, *this);
}

LocalId CurrentBlock::compile_env(ClosureEnvId env) {
    return ctx_.compile_env(env, id_);
}

LocalId CurrentBlock::define_new(const RValue& value) {
    return ctx_.define_new(value, id_);
}

LocalId CurrentBlock::memoize_value(const ComputedValue& key, FunctionRef<LocalId()> compute) {
    return ctx_.memoize_value(key, compute, id_);
}

void CurrentBlock::seal() {
    return ctx_.seal(id_);
}

void CurrentBlock::end(const Terminator& term) {
    return ctx_.end(term, id_);
}

FunctionIRGen::RegionGuard::RegionGuard(FunctionIRGen* func, RegionId new_id, RegionId& slot)
    : func_(func)
    , new_id_(new_id)
    , restore_old_(replace_value(slot, new_id)) {
    TIRO_DEBUG_ASSERT(func_->active_regions_.back_key() == new_id_,
        "The new region must be at the top of the stack.");
}

FunctionIRGen::RegionGuard::~RegionGuard() {
    TIRO_DEBUG_ASSERT(func_->active_regions_.back_key() == new_id_,
        "The region to be removed must be at the top of the stack.");
    func_->active_regions_.pop_back();
    // restore_old_id_ destructor does its thing.
}

FunctionIRGen::FunctionIRGen(FunctionContext ctx, Function& result)
    : module_gen_(ctx.module_gen)
    , envs_(ctx.envs)
    , outer_env_(ctx.closure_env)
    , result_(result) {}

std::string_view FunctionIRGen::source_file() const {
    return module_gen_.source_file();
}

ModuleIRGen& FunctionIRGen::module_gen() const {
    return module_gen_;
}

const AstNodeMap& FunctionIRGen::nodes() const {
    return module_gen_.nodes();
}

const TypeTable& FunctionIRGen::types() const {
    return module_gen_.types();
}

const SymbolTable& FunctionIRGen::symbols() const {
    return module_gen_.symbols();
}

StringTable& FunctionIRGen::strings() const {
    return module_gen_.strings();
}

Diagnostics& FunctionIRGen::diag() const {
    return module_gen_.diag();
}

VecPtr<Region> FunctionIRGen::current_loop() {
    if (!current_loop_)
        return nullptr;

    auto region = active_regions_.ptr_to(current_loop_);
    TIRO_DEBUG_ASSERT(region->type() == RegionType::Loop,
        "The current loop id must always point to a loop region.");
    return region;
}

VecPtr<Region> FunctionIRGen::current_scope() {
    if (!current_scope_)
        return nullptr;

    auto region = active_regions_.ptr_to(current_scope_);
    TIRO_DEBUG_ASSERT(region->type() == RegionType::Scope,
        "The current scope id must always point to a scope region.");
    return region;
}

FunctionIRGen::RegionGuard FunctionIRGen::enter_loop(BlockId jump_break, BlockId jump_continue) {
    auto id = active_regions_.push_back(Region::make_loop(jump_break, jump_continue));
    return RegionGuard(this, id, current_loop_);
}

FunctionIRGen::RegionGuard FunctionIRGen::enter_scope() {
    auto id = active_regions_.push_back(Region::make_scope({}, 0));
    return RegionGuard(this, id, current_scope_);
}

ClosureEnvId FunctionIRGen::current_env() const {
    if (local_env_stack_.empty()) {
        return outer_env_;
    }
    return local_env_stack_.back().env;
}

void FunctionIRGen::compile_function(NotNull<AstFuncDecl*> func) {
    return enter_compilation([&](CurrentBlock& bb) {
        auto param_scope = symbols().get_scope(func->id());
        enter_env(param_scope, bb);

        // Make sure that all parameters are available.
        {
            for (auto param : func->params()) {
                auto symbol_id = symbols().get_decl(param->id());
                auto symbol = symbols()[symbol_id];

                auto param_id = result_.make(Param(symbol->name()));
                auto lvalue = LValue::make_param(param_id);
                auto local_id = bb.define_new(RValue::make_use_lvalue(lvalue));
                bb.compile_assign(symbol_id, local_id);
            }
        }

        // Compile the function body
        const auto body = TIRO_NN(func->body());
        if (func->body_is_value()) {
            [[maybe_unused]] auto body_type = types().get_type(body->id());
            TIRO_DEBUG_ASSERT(can_use_as_value(body_type), "Function body must be a value.");
            auto local = bb.compile_expr(body);
            if (local)
                bb.end(Terminator::make_return(*local, result_.exit()));
        } else {
            if (!bb.compile_expr(body, ExprOptions::MaybeInvalid).is_unreachable()) {
                auto local = bb.compile_rvalue(Constant::make_null());
                bb.end(Terminator::make_return(local, result_.exit()));
            }
        }

        exit_env(param_scope);
    });
}

void FunctionIRGen::compile_initializer(NotNull<AstFile*> module) {
    return enter_compilation([&](CurrentBlock& bb) {
        auto module_scope = symbols().get_scope(module->id());
        enter_env(module_scope, bb);

        bool reachable = true;
        for (auto stmt : module->items()) {
            auto decl_stmt = try_cast<AstDeclStmt>(stmt);
            if (!decl_stmt)
                continue;

            auto var_decl = try_cast<AstVarDecl>(decl_stmt->decl());
            if (!var_decl)
                continue;

            auto result = compile_var_decl(TIRO_NN(var_decl), bb);
            if (!result) {
                reachable = false;
                break;
            }
        }

        if (reachable) {
            auto local = bb.compile_rvalue(Constant::make_null());
            bb.end(Terminator::make_return(local, result_.exit()));
        }

        exit_env(module_scope);
    });
}

void FunctionIRGen::enter_compilation(FunctionRef<void(CurrentBlock& bb)> compile_body) {
    result_[result_.entry()]->sealed(true);
    result_[result_.exit()]->filled(true);

    auto bb = make_current(result_.entry());

    // Make the outer environment accessible as a local.
    if (outer_env_) {
        local_env_locations_[outer_env_] = bb.define_new(RValue::OuterEnvironment{});
    }

    compile_body(bb);

    TIRO_DEBUG_ASSERT(result_[bb.id()]->terminator().type() == TerminatorType::Return,
        "The last block must perform a return.");
    TIRO_DEBUG_ASSERT(result_[bb.id()]->terminator().as_return().target == result_.exit(),
        "The last block at function level must always return to the exit "
        "block.");

    TIRO_DEBUG_ASSERT(active_regions_.empty(), "No active regions must be left behind.");
    TIRO_DEBUG_ASSERT(local_env_stack_.empty(), "No active environments must be left behind.");
    seal(result_.exit());

    eliminate_dead_code(result_);
}

OkResult FunctionIRGen::compile_loop_body(ScopeId loop_scope_id,
    FunctionRef<OkResult()> compile_body, BlockId break_id, BlockId continue_id, CurrentBlock& bb) {
    TIRO_DEBUG_ASSERT(symbols()[loop_scope_id]->is_loop_scope(),
        "Loop body's scope must be marked as a loop scope.");

    auto loop_guard = enter_loop(break_id, continue_id);
    enter_env(loop_scope_id, bb);
    ScopeExit clean_env = [&]() { exit_env(loop_scope_id); };

    return compile_body();
}

LocalId FunctionIRGen::compile_reference(SymbolId symbol_id, CurrentBlock& bb) {
    // TODO: Values of module level constants (imports, const variables can be cached as locals).
    if (auto lvalue = find_lvalue(symbol_id)) {
        auto local_id = bb.compile_rvalue(RValue::make_use_lvalue(*lvalue));

        // Apply name if possible:
        auto local = result()[local_id];
        if (!local->name()) {
            auto symbol = symbols()[symbol_id];
            local->name(symbol->name());
        }

        return local_id;
    }

    return read_variable(symbol_id, bb.id());
}

void FunctionIRGen::compile_assign(const AssignTarget& target, LocalId value, CurrentBlock& bb) {
    auto block_id = bb.id();

    switch (target.type()) {
    case AssignTargetType::LValue: {
        auto stmt = Stmt::make_assign(target.as_lvalue(), value);
        emit(stmt, block_id);
        return;
    }
    case AssignTargetType::Symbol: {
        auto symbol_id = target.as_symbol();
        auto local = result_[value];
        if (!local->name()) {
            auto symbol = symbols()[symbol_id];
            local->name(symbol->name());
        }

        if (auto lvalue = find_lvalue(symbol_id)) {
            emit(Stmt::make_assign(*lvalue, value), block_id);
            return;
        }

        write_variable(symbol_id, value, block_id);
        return;
    }
    }

    TIRO_UNREACHABLE("Invalid assignment target type.");
}

LocalId FunctionIRGen::compile_read(const AssignTarget& target, CurrentBlock& bb) {
    switch (target.type()) {
    case AssignTargetType::LValue:
        return bb.compile_rvalue(RValue::make_use_lvalue(target.as_lvalue()));
    case AssignTargetType::Symbol:
        return compile_reference(target.as_symbol(), bb);
    }

    TIRO_UNREACHABLE("Invalid assignment target type.");
}

LocalId FunctionIRGen::compile_env(ClosureEnvId env, [[maybe_unused]] BlockId block) {
    TIRO_DEBUG_ASSERT(env, "Closure environment to be compiled must be valid.");
    return get_env(env);
}

OkResult FunctionIRGen::compile_scope_exit(RegionId scope_id, CurrentBlock& bb) {
    // Using offset based addressing instead of raw pointers/references to ensure
    // that references remain valid. Calls to compile_expr below may push additonal
    // items to the active_regions_ stack which would invalidate our references.
    auto get_scope = [&]() -> Region::Scope& { return active_regions_[scope_id].as_scope(); };

    u32 initial_processed;
    u32 deferred_count;
    {
        auto& scope = get_scope();
        initial_processed = scope.processed;
        deferred_count = scope.deferred.size();
        TIRO_DEBUG_ASSERT(initial_processed <= deferred_count, "Processed count must be <= size.");
    }
    ScopeExit restore_processed = [&] { get_scope().processed = initial_processed; };

    for (u32 i = deferred_count - initial_processed; i-- > 0;) {
        AstExpr* expr = nullptr;
        {
            auto& scope = get_scope();
            TIRO_DEBUG_ASSERT(scope.deferred.size() == deferred_count,
                "Deferred items must not be modified while processing scope exits.");
            TIRO_DEBUG_ASSERT(scope.processed == deferred_count - i - 1,
                "Recursive calls must restore the processed value");

            expr = scope.deferred[i];
            ++scope.processed; // Signals progress to recursive calls
        }

        // This may produce more recursive calls to compile_scope_exit (or compile_scope_exit_until),
        // if the expression contains control flow expressions like return.
        auto result = bb.compile_expr(TIRO_NN(expr), ExprOptions::MaybeInvalid);
        if (!result)
            return result.failure();
    }
    return ok;
}

OkResult FunctionIRGen::compile_scope_exit_until(RegionId target, CurrentBlock& bb) {
    TIRO_DEBUG_ASSERT(!target || target.value() < active_regions_.size(), "Invalid target index.");

    // inclusive index:
    const size_t until = target ? target.value() + 1 : 0;
    for (size_t i = active_regions_.size(); i-- > until;) {
        auto key = RegionId(i);

        auto region_data = active_regions_.ptr_to(key);
        if (region_data->type() == RegionType::Scope) {
            auto result = compile_scope_exit(key, bb);
            if (!result)
                return result;
        }
    }
    return ok;
}

BlockId FunctionIRGen::make_block(InternedString label) {
    return result_.make(Block(label));
}

LocalId FunctionIRGen::define_new(const RValue& value, BlockId block_id) {
    return define_new(Local(value), block_id);
}

LocalId FunctionIRGen::define_new(const Local& local, BlockId block_id) {
    auto id = result_.make(local);
    emit(Stmt::make_define(id), block_id);
    return id;
}

LocalId FunctionIRGen::memoize_value(
    const ComputedValue& key, FunctionRef<LocalId()> compute, BlockId block_id) {
    const auto value_key = std::tuple(key, block_id);

    if (auto pos = values_.find(value_key); pos != values_.end())
        return pos->second;

    const auto local = compute();
    TIRO_DEBUG_ASSERT(local, "The result of compute() must be a valid local id.");
    values_[value_key] = local;
    return local;
}

void FunctionIRGen::seal(BlockId block_id) {
    auto block = result_[block_id];
    TIRO_DEBUG_ASSERT(!block->sealed(), "Block was already sealed.");

    // Patch incomplete phis. See [BB+13], Section 2.3.
    if (auto pos = incomplete_phis_.find(block_id); pos != incomplete_phis_.end()) {

        auto& phis = pos->second;
        for (const auto& [symbol, phi] : phis) {
            add_phi_operands(symbol, phi, block_id);
        }

        incomplete_phis_.erase(pos);
    }

    block->sealed(true);
}

void FunctionIRGen::emit(const Stmt& stmt, BlockId block_id) {
    auto block = result_[block_id];

    // Insertions are forbidden once a block is filled. The exception are phi nodes
    // inserted by the variable resolution algorithm (triggered by read_variable).
    TIRO_DEBUG_ASSERT(!block->filled() || is_phi_define(result_, stmt) || is_error(result_, stmt),
        "Cannot emit a statement into a filled block.");

    // Cluster phi nodes at the start of the block.
    if (is_phi_define(result_, stmt)) {
        block->insert_stmt(block->phi_count(result_), stmt);
    } else {
        block->append_stmt(stmt);
    }
}

void FunctionIRGen::end(const Terminator& term, BlockId block_id) {
    TIRO_DEBUG_ASSERT(term.type() != TerminatorType::None, "Invalid terminator.");

    // Cannot add instructions after the terminator has been set.
    auto block = result_[block_id];
    if (!block->filled())
        block->filled(true);

    TIRO_DEBUG_ASSERT(
        block->terminator().type() == TerminatorType::None, "Block already has a terminator.");
    block->terminator(term);

    visit_targets(term, [&](BlockId targetId) {
        auto target = result_[targetId];
        TIRO_DEBUG_ASSERT(!target->sealed(), "Cannot add incoming edges to sealed blocks.");
        target->append_predecessor(block_id);
    });
}

void FunctionIRGen::write_variable(SymbolId var, LocalId value, BlockId block_id) {
    variables_[std::tuple(var, block_id)] = value;
}

LocalId FunctionIRGen::read_variable(SymbolId var, BlockId block_id) {
    if (auto pos = variables_.find(std::tuple(var, block_id)); pos != variables_.end()) {
        return pos->second;
    }
    return read_variable_recursive(var, block_id);
}

LocalId FunctionIRGen::read_variable_recursive(SymbolId symbol_id, BlockId block_id) {
    auto block = result_[block_id];
    auto symbol = symbols()[symbol_id];

    LocalId value;
    if (!block->sealed()) {
        auto local = Local(RValue::make_phi0());
        local.name(symbol->name());
        value = define_new(local, block_id);
        incomplete_phis_[block_id].emplace_back(symbol_id, value);
    } else if (block->predecessor_count() == 1) {
        value = read_variable(symbol_id, block->predecessor(0));
    } else if (block->predecessor_count() == 0) {
        TIRO_DEBUG_ASSERT(block_id == result_.entry(), "Only the entry block has 0 predecessors.");
        undefined_variable(symbol_id);

        auto local = Local(RValue::make_error());
        local.name(symbol->name());
        value = define_new(local, block_id);
    } else {
        // Place a phi marker to break the recursion.
        // Recursive calls to read_variable will observe the Phi0 node.
        auto local = Local(RValue::make_phi0());
        local.name(symbol->name());
        value = define_new(local, block_id);
        write_variable(symbol_id, value, block_id);

        // Recurse into predecessor blocks.
        add_phi_operands(symbol_id, value, block_id);
    }

    write_variable(symbol_id, value, block_id);
    return value;
}

void FunctionIRGen::add_phi_operands(SymbolId symbol_id, LocalId value_id, BlockId block_id) {
    auto block = result_[block_id];
    auto symbol = symbols()[symbol_id];

    // Collect the possible operands from all predecessors. Note that, because
    // of recursion, the list of operands may contain the local value itself.
    // TODO: Small vector
    std::vector<LocalId> operands;
    for (auto pred : block->predecessors()) {
        operands.push_back(read_variable(symbol_id, pred));
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

            if (operand == trivial_other || operand == value_id)
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
            TIRO_ERROR("Variable {} was never initialized.", strings().dump(symbol->name()));
        }

        // TODO: Remove uses of this phi that might have become trivial. See Algorithm 3 in [BB+13].
        block->remove_phi(result_, value_id, RValue::make_use_local(trivial_other));
        return;
    }

    // Emit a phi node.
    auto phi_id = result_.make(Phi(std::move(operands)));
    result_[value_id]->value(RValue::make_phi(phi_id));
}

void FunctionIRGen::enter_env(ScopeId parent_scope_id, CurrentBlock& bb) {
    TIRO_DEBUG_ASSERT(can_open_closure_env(parent_scope_id), "Invalid scope type.");

    std::vector<SymbolId> captured; // TODO small vec
    Fix gather_captured = [&](auto& self, ScopeId scope_id) {
        if (scope_id != parent_scope_id && can_open_closure_env(scope_id))
            return;

        auto scope = symbols()[scope_id];
        for (const auto& entry_id : scope->entries()) {
            auto entry = symbols()[entry_id];
            if (entry->captured())
                captured.push_back(entry_id);
        }

        for (const auto& child_id : scope->children()) {
            self(child_id);
        }
    };
    gather_captured(parent_scope_id);

    if (captured.empty())
        return;

    const u32 captured_count = checked_cast<u32>(captured.size());
    const ClosureEnvId parent = current_env();
    const ClosureEnvId env = envs_->make(ClosureEnv(parent, checked_cast<u32>(captured.size())));
    for (u32 i = 0; i < captured.size(); ++i) {
        envs_->write_location(captured[i], ClosureEnvLocation(env, i));
    }

    const auto parent_local = parent ? get_env(parent) : bb.compile_rvalue(Constant::make_null());
    const auto env_local = bb.compile_rvalue(
        RValue::make_make_environment(parent_local, captured_count));
    local_env_stack_.push_back({env, parent_scope_id});
    local_env_locations_[env] = env_local;
}

void FunctionIRGen::exit_env(ScopeId parent_scope) {
    TIRO_DEBUG_ASSERT(can_open_closure_env(parent_scope), "Invalid scope type.");

    if (local_env_stack_.empty() || local_env_stack_.back().starter != parent_scope)
        return;

    const auto env = local_env_stack_.back().env;
    local_env_stack_.pop_back();
    local_env_locations_.erase(env);
}

bool FunctionIRGen::can_open_closure_env(ScopeId scope_id) const {
    auto scope = symbols()[scope_id];

    switch (scope->type()) {
    case ScopeType::File: // For module initializers (TODO: Module scope)
    case ScopeType::Function:
        return true;

    default:
        return scope->is_loop_scope();
    }
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

std::optional<LValue> FunctionIRGen::find_lvalue(SymbolId symbol_id) {
    auto symbol = symbols()[symbol_id];
    auto scope = symbols()[symbol->parent()];

    if (scope->type() == ScopeType::File) { // TODO module
        auto member = module_gen_.find_symbol(symbol_id);
        TIRO_DEBUG_ASSERT(member, "Failed to find member in module.");
        return LValue::make_module(member);
    }

    if (symbol->captured()) {
        auto pos = envs_->read_location(symbol_id);
        TIRO_DEBUG_ASSERT(pos, "Captured symbol without a defined location used as lvalue.");
        return get_captured_lvalue(*pos);
    }

    return {};
}

LValue FunctionIRGen::get_captured_lvalue(const ClosureEnvLocation& loc) {
    TIRO_DEBUG_ASSERT(loc.env, "Must have a valid environment id.");

    const auto& envs = *envs_;
    const auto target_id = loc.env;
    TIRO_DEBUG_ASSERT(
        loc.index < envs[target_id]->size(), "Index into closure environment is out of bounds.");

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
            TIRO_DEBUG_ASSERT(outer_local, "The outer environment must be stored in a local.");
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

void FunctionIRGen::undefined_variable(SymbolId symbol_id) {
    auto symbol = symbols()[symbol_id];
    auto node = nodes().get_node(symbol->node());
    diag().reportf(Diagnostics::Error, node->source(),
        "Symbol '{}' can be uninitialized before its first use.", strings().dump(symbol->name()));
}

} // namespace tiro
