#include "compiler/ir_gen/func.hpp"

#include "common/fix.hpp"
#include "common/scope_guards.hpp"
#include "compiler/ast/ast.hpp"
#include "compiler/ir/function.hpp"
#include "compiler/ir_gen/compile.hpp"
#include "compiler/ir_gen/func.hpp"
#include "compiler/ir_gen/module.hpp"
#include "compiler/ir_passes/assignment_observers.hpp"
#include "compiler/ir_passes/dead_code_elimination.hpp"
#include "compiler/semantics/symbol_table.hpp"
#include "compiler/semantics/type_table.hpp"

#include <vector>

namespace tiro::ir {

void CurrentBlock::advance(InternedString label) {
    auto new_id = ctx().make_block(label);
    end(Terminator::make_jump(new_id));
    assign(new_id);
    seal();
}

InstResult CurrentBlock::compile_expr(NotNull<AstExpr*> expr, ExprOptions options) {
    return tiro::ir::compile_expr(expr, options, *this);
}

OkResult CurrentBlock::compile_stmt(NotNull<AstStmt*> stmt) {
    return tiro::ir::compile_stmt(stmt, *this);
}

InstId CurrentBlock::compile_value(const Value& value) {
    return tiro::ir::compile_value(value, *this);
}

OkResult CurrentBlock::compile_loop_body(ScopeId body_scope_id,
    FunctionRef<OkResult()> compile_body, BlockId break_id, BlockId continue_id) {
    return ctx_.compile_loop_body(body_scope_id, compile_body, break_id, continue_id, *this);
}

void CurrentBlock::compile_assign(const AssignTarget& target, InstId value) {
    return ctx_.compile_assign(target, value, *this);
}

InstId CurrentBlock::compile_read(const AssignTarget& target) {
    return ctx_.compile_read(target, *this);
}

InstId CurrentBlock::compile_env(ClosureEnvId env) {
    return ctx_.compile_env(env, id_);
}

InstId CurrentBlock::define_new(Value&& value) {
    return ctx_.define_new(std::move(value), id_);
}

InstId CurrentBlock::memoize_value(const ComputedValue& key, FunctionRef<InstId()> compute) {
    return ctx_.memoize_value(key, compute, id_);
}

void CurrentBlock::seal() {
    return ctx_.seal(id_);
}

void CurrentBlock::end(Terminator term) {
    return ctx_.end(std::move(term), id_);
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

IndexMapPtr<Region> FunctionIRGen::current_loop() {
    if (!current_loop_)
        return nullptr;

    auto region = active_regions_.ptr_to(current_loop_);
    TIRO_DEBUG_ASSERT(region->type() == RegionType::Loop,
        "The current loop id must always point to a loop region.");
    return region;
}

IndexMapPtr<Region> FunctionIRGen::current_scope() {
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
    auto id = active_regions_.push_back(Region::make_scope(current_handler(), 0, {}));
    return RegionGuard(this, id, current_scope_);
}

ClosureEnvId FunctionIRGen::current_env() const {
    if (local_env_stack_.empty()) {
        return outer_env_;
    }
    return local_env_stack_.back().env;
}

BlockId FunctionIRGen::current_handler() const {
    return current_handler_;
}

void FunctionIRGen::current_handler(BlockId handler) {
    current_handler_ = handler;
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
                auto inst_id = bb.define_new(Value::make_read(lvalue));
                bb.compile_assign(symbol_id, inst_id);
            }
        }

        // Compile the function body
        const auto body = TIRO_NN(func->body());
        if (func->body_is_value()) {
            [[maybe_unused]] auto body_type = types().get_type(body->id());
            TIRO_DEBUG_ASSERT(can_use_as_value(body_type), "Function body must be a value.");
            auto inst_id = bb.compile_expr(body);
            if (inst_id)
                bb.end(Terminator::make_return(*inst_id, result_.exit()));
        } else {
            if (!bb.compile_expr(body, ExprOptions::MaybeInvalid).is_unreachable()) {
                auto inst_id = bb.compile_value(Constant::make_null());
                bb.end(Terminator::make_return(inst_id, result_.exit()));
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
            auto inst_id = bb.compile_value(Constant::make_null());
            bb.end(Terminator::make_return(inst_id, result_.exit()));
        }

        exit_env(module_scope);
    });
}

void FunctionIRGen::enter_compilation(FunctionRef<void(CurrentBlock& bb)> compile_body) {
    result_[result_.entry()]->sealed(true);
    result_[result_.entry()]->filled(true);
    result_[result_.body()]->sealed(true);
    result_[result_.exit()]->filled(true);

    auto bb = make_current(result_.body());

    // Make the outer environment accessible as an instruction.
    if (outer_env_) {
        local_env_locations_[outer_env_] = bb.define_new(Value::OuterEnvironment{});
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

    // Needed for exception handlers.
    connect_assignment_observers(result_);

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

InstId FunctionIRGen::compile_reference(SymbolId symbol_id, CurrentBlock& bb) {
    // TODO: Values of module level constants (imports, const variables can be cached as instructions).
    if (auto lvalue = find_lvalue(symbol_id)) {
        auto inst_id = bb.compile_value(Value::make_read(*lvalue));

        // Apply name if possible:
        auto inst = result()[inst_id];
        if (!inst->name()) {
            auto symbol = symbols()[symbol_id];
            inst->name(symbol->name());
        }

        return inst_id;
    }

    return read_variable(symbol_id, bb.id());
}

void FunctionIRGen::compile_assign(const AssignTarget& target, InstId value, CurrentBlock& bb) {
    auto block_id = bb.id();

    switch (target.type()) {
    case AssignTargetType::LValue: {
        define_new(Value::make_write(target.as_lvalue(), value), block_id);
        return;
    }
    case AssignTargetType::Symbol: {
        auto symbol_id = target.as_symbol();

        // Initialize the name of the source value, if it does not already have one.
        auto inst = result_[value];
        if (!inst->name()) {
            auto symbol = symbols()[symbol_id];
            inst->name(symbol->name());
        }

        // Does the symbol refer to a non-ssa variable?
        if (auto lvalue = find_lvalue(symbol_id)) {
            define_new(Value::make_write(*lvalue, value), block_id);
            return;
        }

        // Simply update the SSA<->Variable mapping. Publish the assignment in case any
        // exception handler needs it.
        write_variable(symbol_id, value, block_id);
        define_new(Value::make_publish_assign(symbol_id, value), block_id);
        return;
    }
    }

    TIRO_UNREACHABLE("Invalid assignment target type.");
}

InstId FunctionIRGen::compile_read(const AssignTarget& target, CurrentBlock& bb) {
    switch (target.type()) {
    case AssignTargetType::LValue:
        return bb.compile_value(Value::make_read(target.as_lvalue()));
    case AssignTargetType::Symbol:
        return compile_reference(target.as_symbol(), bb);
    }

    TIRO_UNREACHABLE("Invalid assignment target type.");
}

InstId FunctionIRGen::compile_env(ClosureEnvId env, [[maybe_unused]] BlockId block) {
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
    BlockId original_handler;
    {
        auto& scope = get_scope();
        initial_processed = scope.processed;
        deferred_count = scope.deferred.size();
        original_handler = scope.original_handler;
        TIRO_DEBUG_ASSERT(initial_processed <= deferred_count, "Processed count must be <= size.");
    }
    ScopeSuccess restore_processed = [&] { get_scope().processed = initial_processed; };
    ScopeSuccess restore_handler = [&] { current_handler(original_handler); };

    for (u32 i = deferred_count - initial_processed; i-- > 0;) {
        AstExpr* expr = nullptr;
        BlockId handler;
        {
            auto& scope = get_scope();
            TIRO_DEBUG_ASSERT(scope.deferred.size() == deferred_count,
                "Deferred items must not be modified while processing scope exits.");
            TIRO_DEBUG_ASSERT(scope.processed == deferred_count - i - 1,
                "Recursive calls must restore the processed value");

            std::tie(expr, handler) = scope.deferred[i];
            ++scope.processed; // Signals progress to recursive calls
        }

        // This may produce more recursive calls to compile_scope_exit (or compile_scope_exit_until),
        // if the expression contains control flow expressions like return.
        current_handler(handler);
        bb.advance(strings().insert("defer-normal"));
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
    Block block(label);
    block.handler(current_handler());
    return result_.make(std::move(block));
}

BlockId FunctionIRGen::make_handler_block(InternedString label) {
    auto block_id = make_block(label);
    auto block = result_[block_id];
    seal(block_id);
    block->is_handler(true);

    auto entry_id = result_.entry();
    auto entry = result_[entry_id];
    entry->terminator().as_entry().handlers.push_back(block_id);
    block->append_predecessor(entry_id);
    return block_id;
}

InstId FunctionIRGen::define_new(Value&& value, BlockId block_id) {
    return define_new(Inst(std::move(value)), block_id);
}

InstId FunctionIRGen::define_new(Inst&& inst, BlockId block_id) {
    auto id = result_.make(std::move(inst));
    emit(id, block_id);
    return id;
}

InstId FunctionIRGen::memoize_value(
    const ComputedValue& key, FunctionRef<InstId()> compute, BlockId block_id) {
    const auto value_key = std::tuple(key, block_id);

    if (auto pos = values_.find(value_key); pos != values_.end())
        return pos->second;

    const auto inst = compute();
    TIRO_DEBUG_ASSERT(inst, "The result of compute() must be a valid instruction id.");
    values_[value_key] = inst;
    return inst;
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

void FunctionIRGen::emit(InstId inst, BlockId block_id) {
    TIRO_DEBUG_ASSERT(
        block_id != result_.entry(), "Cannot emit instructions into the entry block.");
    TIRO_DEBUG_ASSERT(block_id != result_.exit(), "Cannot emit instructions into the exit block.");

    auto block = result_[block_id];

    // Insertions are forbidden once a block is filled.
    // Exceptions are made for instructions that result from the variable resolution algorithm.
    const auto type = result_[inst]->value().type();
    TIRO_DEBUG_ASSERT(!block->filled() || type == ValueType::Phi || type == ValueType::ObserveAssign
                          || type == ValueType::Error,
        "Cannot emit an instruction into a filled block.");

    if (block->is_handler()) {
        TIRO_DEBUG_ASSERT(type != ValueType::Phi, "Handler blocks must not use phi instructions.");
    } else {
        TIRO_DEBUG_ASSERT(type != ValueType::ObserveAssign,
            "ObserveAssign instructions may only be used in handler blocks.");
    }

    // Cluster phi nodes at the start of the block.
    if (type == ValueType::Phi || type == ValueType::ObserveAssign) {
        block->insert_inst(block->phi_count(result_), inst);
    } else {
        block->append_inst(inst);
    }
}

void FunctionIRGen::end(Terminator term, BlockId block_id) {
    TIRO_DEBUG_ASSERT(term.type() != TerminatorType::None, "Invalid terminator.");

    // Cannot add instructions after the terminator has been set.
    auto block = result_[block_id];
    if (!block->filled())
        block->filled(true);

    TIRO_DEBUG_ASSERT(
        block->terminator().type() == TerminatorType::None, "Block already has a terminator.");
    block->terminator(std::move(term));

    visit_targets(block->terminator(), [&](BlockId targetId) {
        auto target = result_[targetId];
        TIRO_DEBUG_ASSERT(!target->sealed(), "Cannot add incoming edges to sealed blocks.");
        target->append_predecessor(block_id);
    });
}

void FunctionIRGen::write_variable(SymbolId var, InstId value, BlockId block_id) {
    variables_[std::tuple(var, block_id)] = value;
}

InstId FunctionIRGen::read_variable(SymbolId var, BlockId block_id) {
    if (auto pos = variables_.find(std::tuple(var, block_id)); pos != variables_.end()) {
        return pos->second;
    }
    return read_variable_recursive(var, block_id);
}

InstId FunctionIRGen::read_variable_recursive(SymbolId symbol_id, BlockId block_id) {
    TIRO_DEBUG_ASSERT(block_id != result_.entry(),
        "Variable lookup must always terminate before reaching the virtual CFG root.");

    auto block = result_[block_id];
    auto symbol = symbols()[symbol_id];
    TIRO_DEBUG_ASSERT(block->predecessor_count(),
        "The block must have at least one predecessor, since we are not at the CFG root.");

    InstId inst_id;
    if (block_id == result_.body()) {
        // We bubbled up to the start of the function body, which means the variable was never defined.
        undefined_variable(symbol_id);
        auto inst = Inst(Value::make_error());
        inst.name(symbol->name());
        inst_id = define_new(std::move(inst), block_id);
    } else if (block->is_handler()) {
        // The observe_assign value is created immediately, but without any operands. Those will be filled in
        // later after the functions has been compiled. All publish_assign instructions that may be observed
        // by the exception handler will become operands of the phi_catch here.
        auto inst = Inst(Value::make_observe_assign(symbol_id, {}));
        inst.name(symbol->name());
        inst_id = define_new(std::move(inst), block_id);
    } else if (!block->sealed()) {
        // Since the block has not been sealed yet, we cannot know all possible values of the symbol. We create
        // an empty phi node to stop the recursion here and remember its location in incomplete_phis, which will
        // be visited once the block has been sealed.
        auto inst = Inst(Value(Phi()));
        inst.name(symbol->name());
        inst_id = define_new(std::move(inst), block_id);
        incomplete_phis_[block_id].emplace_back(symbol_id, inst_id);
    } else if (block->predecessor_count() == 1) {
        inst_id = read_variable(symbol_id, block->predecessor(0));
    } else {
        // Place a phi marker to break the recursion.
        // Recursive calls to read_variable will observe the existing Phi node.
        auto inst = Inst(Value(Phi()));
        inst.name(symbol->name());
        inst_id = define_new(std::move(inst), block_id);
        write_variable(symbol_id, inst_id, block_id);

        // Recurse into predecessor blocks.
        add_phi_operands(symbol_id, inst_id, block_id);
    }

    write_variable(symbol_id, inst_id, block_id);
    return inst_id;
}

void FunctionIRGen::add_phi_operands(SymbolId symbol_id, InstId inst_id, BlockId block_id) {
    auto block = result_[block_id];
    auto symbol = symbols()[symbol_id];

    // Collect the possible operands from all predecessors. Note that, because
    // of recursion, the list of operands may contain the instruction value itself.
    LocalList::Storage operands;
    for (auto pred : block->predecessors()) {
        operands.push_back(read_variable(symbol_id, pred));
    }

    // Do not emit trivial phi nodes. A phi node is trivial iff its list of operands
    // only contains itself and at most one other value.
    //
    // TODO: Complete removal of nodes that turn out to be trivial is not yet implemented (requires
    // def-use tracking to replace uses).
    bool is_trivial = true;
    InstId trivial_other;
    {
        for (const auto& operand : operands) {
            TIRO_DEBUG_ASSERT(operand, "Invalid operand to phi node.");

            if (operand == trivial_other || operand == inst_id)
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
        block->remove_phi(result_, inst_id, Value::make_alias(trivial_other));
        return;
    }

    // Finish the phi node by setting the operands list to a valid value.
    auto list_id = result_.make(LocalList(std::move(operands)));
    result_[inst_id]->value().as_phi().operands(list_id);
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

    const auto parent_inst = parent ? get_env(parent) : bb.compile_value(Constant::make_null());
    const auto env_inst = bb.compile_value(
        Value::make_make_environment(parent_inst, captured_count));
    local_env_stack_.push_back({env, parent_scope_id});
    local_env_locations_[env] = env_inst;
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

std::optional<InstId> FunctionIRGen::find_env(ClosureEnvId env) {
    auto pos = local_env_locations_.find(env);
    if (pos != local_env_locations_.end())
        return pos->second;
    return {};
}

InstId FunctionIRGen::get_env(ClosureEnvId env) {
    auto inst = find_env(env);
    TIRO_DEBUG_ASSERT(inst, "Local environment was not found.");
    return *inst;
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
    if (auto inst = find_env(target_id)) {
        return LValue::make_closure(*inst, 0, loc.index);
    }

    // Try to reach the target environment by moving upwards from the outer env.
    auto current_id = outer_env_;
    u32 levels = 0;
    while (current_id) {
        if (current_id == target_id) {
            const auto outer_inst = find_env(outer_env_);
            TIRO_DEBUG_ASSERT(
                outer_inst, "The outer environment must be stored in an instruction.");
            return LValue::make_closure(*outer_inst, levels, loc.index);
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

} // namespace tiro::ir
