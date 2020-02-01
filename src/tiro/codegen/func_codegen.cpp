#include "tiro/codegen/func_codegen.hpp"

#include "tiro/codegen/expr_codegen.hpp"
#include "tiro/codegen/module_codegen.hpp"
#include "tiro/codegen/stmt_codegen.hpp"

namespace tiro::compiler {

FunctionCodegen::FunctionCodegen(
    NotNull<FuncDecl*> func, ModuleCodegen& module, u32 index_in_module)
    : FunctionCodegen(func, nullptr, module, index_in_module) {}

FunctionCodegen::FunctionCodegen(
    NotNull<FuncDecl*> func, FunctionCodegen& parent, u32 index_in_module)
    : FunctionCodegen(func, &parent, parent.module(), index_in_module) {}

FunctionCodegen::FunctionCodegen(NotNull<FuncDecl*> func,
    FunctionCodegen* parent, ModuleCodegen& module, u32 index_in_module)
    : func_(func)
    , parent_(parent)
    , module_(module)
    , index_in_module_(index_in_module)
    , symbols_(module.symbols())
    , strings_(module.strings())
    , diag_(module.diag())
    , result_(std::make_unique<FunctionDescriptor>(
          parent ? FunctionDescriptor::TEMPLATE
                 : FunctionDescriptor::FUNCTION)) {

    if (parent_) {
        outer_context_ = parent_->current_closure_;
        current_closure_ = outer_context_;
    }

    result_->name = func->name();
}

void FunctionCodegen::compile() {
    locations_ = FunctionLocations::compute(
        TIRO_NN(func_), current_closure_, symbols_, strings_);
    result_->params = locations_.params();
    result_->locals = locations_.locals();

    auto initial = blocks_.make_block(strings_.insert("function"));
    {
        CurrentBasicBlock bb(TIRO_NN(initial));
        compile_function(TIRO_NN(func_), bb);
    }

    generate_code(TIRO_NN(initial), result_->code);
    module_.set_function(index_in_module_, TIRO_NN(std::move(result_)));
}

void FunctionCodegen::compile_function(
    NotNull<FuncDecl*> func, CurrentBasicBlock& bb) {
    ClosureContext* context = get_closure_context(func->param_scope());
    if (context)
        push_context(TIRO_NN(context), bb);

    {
        const NotNull params = TIRO_NN(func->params());

        const size_t param_count = params->size();
        for (size_t i = 0; i < param_count; ++i) {
            const NotNull param = TIRO_NN(params->get(i));
            const NotNull entry = TIRO_NN(param->declared_symbol());

            // Move captured params from the stack to the closure context.
            VarLocation loc = get_location(entry);
            if (loc.type == VarLocationType::Context) {
                TIRO_ASSERT(context != nullptr,
                    "Must have a local context if params are captured.");
                bb->append(make_instr<LoadParam>(checked_cast<u32>(i)));
                load_context(context, bb);
                bb->append(make_instr<StoreContext>(u32(0), loc.context.index));
            }
        }
    }

    compile_function_body(TIRO_NN(func->body()), bb);
    TIRO_ASSERT(bb->edge().which() == BasicBlockEdge::Which::Ret
                    || bb->edge().which() == BasicBlockEdge::Which::Never,
        "Function body must generate a return edge.");

    if (context)
        pop_context(TIRO_NN(context));
}

void FunctionCodegen::compile_function_body(
    NotNull<Expr*> body, CurrentBasicBlock& bb) {
    if (body->expr_type() == ExprType::Value) {
        generate_expr_value(body, bb);
        bb->edge(BasicBlockEdge::make_ret());
    } else {
        generate_expr_ignore(body, bb);
        if (body->expr_type() != ExprType::Never) {
            bb->append(make_instr<LoadNull>());
            bb->edge(BasicBlockEdge::make_ret());
        } else {
            bb->edge(BasicBlockEdge::make_never());
        }
    }
}

void FunctionCodegen::generate_expr_value(
    NotNull<Expr*> expr, CurrentBasicBlock& bb) {
    TIRO_ASSERT(can_use_as_value(expr->expr_type()),
        "Cannot use this expression in a value context.");
    [[maybe_unused]] const bool generated = generate_expr(expr, bb);
    TIRO_ASSERT(generated, "Must not omit generation if a value is required.");
}

void FunctionCodegen::generate_expr_ignore(
    NotNull<Expr*> expr, CurrentBasicBlock& bb) {
    const bool generated = generate_expr(expr, bb);
    if (expr->expr_type() == ExprType::Value && generated)
        bb->append(make_instr<Pop>());
}

bool FunctionCodegen::generate_expr(
    NotNull<Expr*> expr, CurrentBasicBlock& bb) {
    ExprCodegen gen(expr, bb, *this);
    const bool generated = gen.generate();
    TIRO_ASSERT(!expr->observed() || generated,
        "Can only omit generation when not observed.");
    return generated;
}

void FunctionCodegen::generate_stmt(
    NotNull<Stmt*> stmt, CurrentBasicBlock& bb) {
    StmtCodegen gen(stmt, bb, *this);
    gen.generate();
}

void FunctionCodegen::generate_load(
    const SymbolEntryPtr& entry, CurrentBasicBlock& bb) {
    TIRO_ASSERT_NOT_NULL(entry);

    VarLocation loc = get_location(entry);

    switch (loc.type) {
    case VarLocationType::Param:
        bb->append(make_instr<LoadParam>(loc.param.index));
        break;
    case VarLocationType::Local:
        bb->append(make_instr<LoadLocal>(loc.local.index));
        break;
    case VarLocationType::Module:
        bb->append(make_instr<LoadModule>(loc.module.index));
        break;
    case VarLocationType::Context: {
        if (auto local = local_context(TIRO_NN(loc.context.ctx))) {
            bb->append(make_instr<LoadLocal>(*local));
            bb->append(make_instr<LoadContext>(u32(0), loc.context.index));
        } else {
            const u32 levels = get_context_level(
                TIRO_NN(outer_context_), TIRO_NN(loc.context.ctx));
            load_context(outer_context_, bb);
            bb->append(make_instr<LoadContext>(levels, loc.context.index));
        }
        break;
    }
    }
}

void FunctionCodegen::generate_store(
    const SymbolEntryPtr& entry, CurrentBasicBlock& bb) {
    TIRO_ASSERT_NOT_NULL(entry);

    VarLocation loc = get_location(entry);
    switch (loc.type) {
    case VarLocationType::Param: {
        bb->append(make_instr<StoreParam>(loc.param.index));
        break;
    }
    case VarLocationType::Local: {
        bb->append(make_instr<StoreLocal>(loc.local.index));
        break;
    }
    case VarLocationType::Module: {
        bb->append(make_instr<StoreModule>(loc.module.index));
        break;
    }
    case VarLocationType::Context: {
        u32 levels = 0;
        if (auto local = local_context(TIRO_NN(loc.context.ctx))) {
            bb->append(make_instr<LoadLocal>(*local));
        } else {
            levels = get_context_level(
                TIRO_NN(outer_context_), TIRO_NN(loc.context.ctx));
            load_context(outer_context_, bb);
        }

        bb->append(make_instr<StoreContext>(levels, loc.context.index));
        break;
    }
    }
}

void FunctionCodegen::generate_closure(
    NotNull<FuncDecl*> decl, CurrentBasicBlock& bb) {
    // TODO: A queue of compilation jobs would be nicer than a recursive call here.
    // TODO: Lambda names in the module
    // TODO: No closure template when no capture vars.
    const u32 nested_index = module_.add_function();
    FunctionCodegen nested(decl, *this, nested_index);
    nested.compile();

    bb->append(make_instr<LoadModule>(nested_index));
    load_context(bb);
    bb->append(make_instr<MkClosure>());
}

void FunctionCodegen::generate_loop_body(const ScopePtr& body_scope,
    NotNull<BasicBlock*> loop_start, NotNull<BasicBlock*> loop_end,
    NotNull<Expr*> body, CurrentBasicBlock& bb) {
    TIRO_ASSERT_NOT_NULL(body_scope);

    LoopContext loop;
    loop.parent = current_loop_;
    loop.break_label = loop_end;
    loop.continue_label = loop_start;
    push_loop(TIRO_NN(&loop));

    {
        ClosureContext* context = get_closure_context(body_scope);
        if (context)
            push_context(TIRO_NN(context), bb);

        generate_expr_ignore(body, bb);

        if (context)
            pop_context(TIRO_NN(context));

        pop_loop(TIRO_NN(&loop));
    }
}

VarLocation FunctionCodegen::get_location(const SymbolEntryPtr& entry) {
    TIRO_ASSERT_NOT_NULL(entry);

    if (auto loc = locations_.get_location(entry))
        return *loc;

    if (parent_) {
        auto loc = parent_->get_location(entry);
        TIRO_ASSERT(loc.type == VarLocationType::Module
                        || loc.type == VarLocationType::Context,
            "Must be a module or a closure location.");
        return loc;
    }

    auto loc = module_.get_location(entry);
    TIRO_ASSERT(
        loc.type == VarLocationType::Module, "Must be a module location.");
    return loc;
}

void FunctionCodegen::load_context(
    ClosureContext* context, CurrentBasicBlock& bb) {
    if (context == outer_context_) {
        if (context) {
            bb->append(make_instr<LoadClosure>());
        } else {
            bb->append(make_instr<LoadNull>());
        }
        return;
    }

    if (auto local = local_context(TIRO_NN(context))) {
        bb->append(make_instr<LoadLocal>(*local));
        return;
    }

    TIRO_UNREACHABLE("Cannot load the given context.");
}

void FunctionCodegen::load_context(CurrentBasicBlock& bb) {
    load_context(current_closure_, bb);
}

void FunctionCodegen::push_context(
    NotNull<ClosureContext*> context, CurrentBasicBlock& bb) {
    TIRO_ASSERT(context->parent == current_closure_,
        "Must be a child of the current closure context.");
    TIRO_ASSERT(context->size > 0,
        "Frontend must never generate 0-sized context objects.");

    auto local = local_context(context);
    TIRO_ASSERT(local, "Must be a local context.");
    TIRO_ASSERT(*local < locations_.locals(), "Invalid local index.");

    load_context(bb);
    bb->append(make_instr<MkContext>(context->size));
    bb->append(make_instr<StoreLocal>(*local));

    current_closure_ = context;
}

void FunctionCodegen::pop_context(
    [[maybe_unused]] NotNull<ClosureContext*> context) {
    TIRO_ASSERT_NOT_NULL(current_closure_);
    TIRO_ASSERT(context == current_closure_, "Pop for wrong closure context.");
    current_closure_ = current_closure_->parent;
}

u32 FunctionCodegen::get_context_level(
    NotNull<ClosureContext*> start, NotNull<ClosureContext*> dst) {
    ClosureContext* ctx = start;
    u32 level = 0;
    while (ctx) {
        if (ctx == dst)
            return level;

        ++level;
        ctx = ctx->parent;
    }

    TIRO_ERROR("Failed to reach destination closure context.");
}

std::optional<u32>
FunctionCodegen::local_context(NotNull<ClosureContext*> context) {
    if (context->func == func_) {
        return context->local_index;
    }
    return {};
}

void FunctionCodegen::push_loop(NotNull<LoopContext*> loop) {
    TIRO_ASSERT(
        loop->parent == current_loop_, "Must be a child of the current loop.");
    current_loop_ = loop;
}

void FunctionCodegen::pop_loop([[maybe_unused]] NotNull<LoopContext*> loop) {
    TIRO_ASSERT_NOT_NULL(current_loop_);
    TIRO_ASSERT(loop == current_loop_, "Pop for wrong loop context.");
    current_loop_ = current_loop_->parent;
}

void FunctionCodegen::generate_code(
    NotNull<BasicBlock*> start, std::vector<byte>& out) {
    CodeBuilder builder(out);
    std::vector<NotNull<BasicBlock*>> stack;
    std::unordered_set<BasicBlock*> seen_blocks;

    auto seen = [&](BasicBlock* block) { return seen_blocks.count(block); };

    auto push = [&](NotNull<BasicBlock*> next) {
        auto inserted = seen_blocks.emplace(next).second;
        if (inserted) {
            stack.push_back(next);
        }
    };

    push(start);
    while (!stack.empty()) {
        auto block = stack.back();
        stack.pop_back();

        builder.define_block(block);

        auto code = block->code();
        for (NotNull<Instruction*> instr : code) {
            emit_instruction(instr, builder);
        }

        const auto& edge = block->edge();
        switch (edge.which()) {
        case BasicBlockEdge::Which::None:
            TIRO_ERROR("Block without a valid outgoing edge.");
        case BasicBlockEdge::Which::Jump: {
            auto target = edge.jump().target;
            if (seen(target)) {
                builder.jmp(target);
            } else {
                // Flatten the control flow - this block will be evaluated
                // in the next generation (without a jump).
                push(TIRO_NN(target));
            }
            break;
        }
        case BasicBlockEdge::Which::CondJump: {
            auto cond = edge.cond_jump();

            // FIXME make this readable.
            switch (cond.code) {
            case Opcode::JmpTrue:
                builder.jmp_true(cond.target);
                break;
            case Opcode::JmpTruePop:
                builder.jmp_true_pop(cond.target);
                break;
            case Opcode::JmpFalse:
                builder.jmp_false(cond.target);
                break;
            case Opcode::JmpFalsePop:
                builder.jmp_false_pop(cond.target);
                break;
            default:
                TIRO_ERROR("Invalid conditional jump instruction: {}.",
                    to_string(cond.code));
            }
            push(TIRO_NN(cond.target));

            if (seen(cond.fallthrough)) {
                builder.jmp(cond.fallthrough);
            } else {
                // Will be evaluated next.
                push(TIRO_NN(cond.fallthrough));
            }
            break;
        }

        case BasicBlockEdge::Which::AssertFail:
            builder.assert_fail();
            break;

        case BasicBlockEdge::Which::Never:
            break;

        case BasicBlockEdge::Which::Ret:
            builder.ret();
            break;
        }
    }

    builder.finish();
}

} // namespace tiro::compiler
