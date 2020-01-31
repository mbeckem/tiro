#include "tiro/codegen/codegen.hpp"

#include "tiro/codegen/expr_codegen.hpp"
#include "tiro/codegen/stmt_codegen.hpp"
#include "tiro/core/defs.hpp"
#include "tiro/core/math.hpp"
#include "tiro/core/scope.hpp"
#include "tiro/semantics/analyzer.hpp"

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
          parent ? FunctionDescriptor::TEMPLATE : FunctionDescriptor::FUNCTION))
    , builder_(result_->code) {

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

    compile_function(TIRO_NN(func_));
    builder_.finish();
    module_.set_function(index_in_module_, TIRO_NN(std::move(result_)));
}

void FunctionCodegen::compile_function(NotNull<FuncDecl*> func) {
    ClosureContext* context = get_closure_context(func->param_scope());
    if (context)
        push_context(TIRO_NN(context));

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
                builder_.load_param(checked_cast<u32>(i));
                load_context(context);
                builder_.store_context(0, loc.context.index);
            }
        }
    }

    compile_function_body(TIRO_NN(func->body()));

    if (context)
        pop_context(TIRO_NN(context));
}

void FunctionCodegen::compile_function_body(NotNull<Expr*> body) {
    if (body->expr_type() == ExprType::Value) {
        generate_expr_value(body);
        builder_.ret();
    } else {
        generate_expr_ignore(body);
        if (body->expr_type() != ExprType::Never) {
            builder_.load_null();
            builder_.ret();
        }
    }
}

void FunctionCodegen::generate_expr_value(NotNull<Expr*> expr) {
    TIRO_ASSERT(can_use_as_value(expr->expr_type()),
        "Cannot use this expression in a value context.");
    [[maybe_unused]] const bool generated = generate_expr(expr);
    TIRO_ASSERT(generated, "Must not omit generation if a value is required.");
}

void FunctionCodegen::generate_expr_ignore(NotNull<Expr*> expr) {
    const bool generated = generate_expr(expr);
    if (expr->expr_type() == ExprType::Value && generated)
        builder_.pop();
}

bool FunctionCodegen::generate_expr(NotNull<Expr*> expr) {
    ExprCodegen gen(expr, *this);
    const bool generated = gen.generate();
    TIRO_ASSERT(!expr->observed() || generated,
        "Can only omit generation when not observed.");
    return generated;
}

void FunctionCodegen::generate_stmt(NotNull<Stmt*> stmt) {
    StmtCodegen gen(stmt, *this);
    gen.generate();
}

void FunctionCodegen::generate_load(const SymbolEntryPtr& entry) {
    TIRO_ASSERT_NOT_NULL(entry);

    VarLocation loc = get_location(entry);

    switch (loc.type) {
    case VarLocationType::Param:
        builder_.load_param(loc.param.index);
        break;
    case VarLocationType::Local:
        builder_.load_local(loc.local.index);
        break;
    case VarLocationType::Module:
        builder_.load_module(loc.module.index);
        break;
    case VarLocationType::Context: {
        if (auto local = local_context(TIRO_NN(loc.context.ctx))) {
            builder_.load_local(*local);
            builder_.load_context(0, loc.context.index);
        } else {
            const u32 levels = get_context_level(
                TIRO_NN(outer_context_), TIRO_NN(loc.context.ctx));
            load_context(outer_context_);
            builder_.load_context(levels, loc.context.index);
        }
        break;
    }
    }
}

void FunctionCodegen::generate_store(const SymbolEntryPtr& entry) {
    TIRO_ASSERT_NOT_NULL(entry);

    VarLocation loc = get_location(entry);
    switch (loc.type) {
    case VarLocationType::Param: {
        builder_.store_param(loc.param.index);
        break;
    }
    case VarLocationType::Local: {
        builder_.store_local(loc.local.index);
        break;
    }
    case VarLocationType::Module: {
        builder_.store_module(loc.module.index);
        break;
    }
    case VarLocationType::Context: {
        u32 levels = 0;
        if (auto local = local_context(TIRO_NN(loc.context.ctx))) {
            builder_.load_local(*local);
        } else {
            levels = get_context_level(
                TIRO_NN(outer_context_), TIRO_NN(loc.context.ctx));
            load_context(outer_context_);
        }

        builder_.store_context(levels, loc.context.index);
        break;
    }
    }
}

void FunctionCodegen::generate_closure(NotNull<FuncDecl*> decl) {
    // TODO: A queue of compilation jobs would be nicer than a recursive call here.
    // TODO: Lambda names in the module
    // TODO: No closure template when no capture vars.
    const u32 nested_index = module_.add_function();
    FunctionCodegen nested(decl, *this, nested_index);
    nested.compile();

    builder_.load_module(nested_index);
    load_context();
    builder_.mk_closure();
}

void FunctionCodegen::generate_loop_body(LabelID break_label,
    LabelID continue_label, const ScopePtr& body_scope, NotNull<Expr*> body) {
    TIRO_ASSERT_NOT_NULL(body_scope);

    LoopContext loop;
    loop.parent = current_loop_;
    loop.break_label = break_label;
    loop.continue_label = continue_label;
    loop.start_balance = builder_.balance();
    push_loop(TIRO_NN(&loop));

    ClosureContext* context = get_closure_context(body_scope);
    if (context)
        push_context(TIRO_NN(context));

    generate_expr_ignore(body);

    if (context)
        pop_context(TIRO_NN(context));

    pop_loop(TIRO_NN(&loop));

    TIRO_CHECK(builder_.balance() == loop.start_balance,
        "Loop did not maintain balance of values on the stack.");
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

void FunctionCodegen::load_context(ClosureContext* context) {
    if (context == outer_context_) {
        if (context) {
            builder_.load_closure();
        } else {
            builder_.load_null();
        }
        return;
    }

    if (auto local = local_context(TIRO_NN(context))) {
        builder_.load_local(*local);
        return;
    }

    TIRO_UNREACHABLE("Cannot load the given context.");
}

void FunctionCodegen::load_context() {
    load_context(current_closure_);
}

void FunctionCodegen::push_context(NotNull<ClosureContext*> context) {
    TIRO_ASSERT(context->parent == current_closure_,
        "Must be a child of the current closure context.");
    TIRO_ASSERT(context->size > 0,
        "Frontend must never generate 0-sized context objects.");

    auto local = local_context(context);
    TIRO_ASSERT(local, "Must be a local context.");
    TIRO_ASSERT(*local < locations_.locals(), "Invalid local index.");

    load_context();
    builder_.mk_context(context->size);
    builder_.store_local(*local);

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

ModuleCodegen::ModuleCodegen(InternedString name, NotNull<Root*> root,
    SymbolTable& symbols, StringTable& strings, Diagnostics& diag)
    : root_(root)
    , symbols_(symbols)
    , strings_(strings)
    , diag_(diag)
    , result_(std::make_unique<CompiledModule>()) {
    TIRO_ASSERT(name, "Invalid module name.");
    result_->name = name;
}

void ModuleCodegen::compile() {
    auto insert_loc = [&](const SymbolEntryPtr& entry, u32 index,
                          bool constant) {
        TIRO_ASSERT_NOT_NULL(entry);
        TIRO_ASSERT(!entry_to_location_.count(entry), "Decl already indexed.");

        VarLocation loc;
        loc.type = VarLocationType::Module;
        loc.module.index = index;
        loc.module.constant = constant;
        entry_to_location_.emplace(entry, loc);
    };

    const NotNull file = TIRO_NN(root_->file());
    const NotNull items = TIRO_NN(file->items());

    // TODO Queue that can be accessed for recursive compilations?
    std::vector<std::unique_ptr<FunctionCodegen>> jobs;

    for (const auto item : items->entries()) {
        if (auto decl = try_cast<ImportDecl>(item)) {
            TIRO_ASSERT(decl->name(), "Invalid name.");
            TIRO_ASSERT(!decl->path_elements().empty(),
                "Must have at least one import path element.");

            std::string joined_string;
            for (auto element : decl->path_elements()) {
                if (!joined_string.empty())
                    joined_string += ".";
                joined_string += strings_.value(element);
            }

            const u32 index = add_import(strings_.insert(joined_string));
            insert_loc(decl->declared_symbol(), index, true);
            continue;
        }

        if (auto decl = try_cast<FuncDecl>(item)) {
            const u32 index = add_function();
            insert_loc(decl->declared_symbol(), index, true);

            // Don't compile yet, gather locations for other module-level functions.
            jobs.push_back(
                std::make_unique<FunctionCodegen>(TIRO_NN(decl), *this, index));
            continue;
        }

        TIRO_ERROR("Invalid node of type {} at module level.",
            to_string(item->type()));
    }

    for (const auto& job : jobs) {
        job->compile();
    }

    // Validate all function slots.
    for (const auto& member : result_->members) {
        if (member.which() == ModuleItem::Which::Function) {
            const ModuleItem::Function& func = member.get_function();
            TIRO_CHECK(func.value,
                "Logic error: function pointer was not set to a valid "
                "value.");
        }
    }
}

u32 ModuleCodegen::add_function() {
    TIRO_ASSERT_NOT_NULL(result_);

    const u32 index = checked_cast<u32>(result_->members.size());
    result_->members.push_back(ModuleItem::make_func(nullptr));
    return index;
}

void ModuleCodegen::set_function(
    u32 index, NotNull<std::unique_ptr<FunctionDescriptor>> func) {
    TIRO_ASSERT_NOT_NULL(result_);
    TIRO_ASSERT(
        index < result_->members.size(), "Function index out of bounds.");

    ModuleItem& item = result_->members[index];
    TIRO_ASSERT(item.which() == ModuleItem::Which::Function,
        "Module member is not a function.");
    item.get_function().value = std::move(func);
}

u32 ModuleCodegen::add_integer(i64 value) {
    return add_constant(const_integers_, ModuleItem::Integer(value));
}

u32 ModuleCodegen::add_float(f64 value) {
    return add_constant(const_floats_, ModuleItem::Float(value));
}

u32 ModuleCodegen::add_string(InternedString str) {
    return add_constant(const_strings_, ModuleItem::String(str));
}

u32 ModuleCodegen::add_symbol(InternedString sym) {
    const u32 str = add_string(sym);
    return add_constant(const_symbols_, ModuleItem::Symbol(str));
}

u32 ModuleCodegen::add_import(InternedString imp) {
    const u32 str = add_string(imp);
    return add_constant(const_imports_, ModuleItem::Import(str));
}

template<typename T>
u32 ModuleCodegen::add_constant(ConstantPool<T>& consts, const T& value) {
    TIRO_ASSERT_NOT_NULL(result_);

    if (auto pos = consts.find(value); pos != consts.end()) {
        return pos->second;
    }

    const u32 index = checked_cast<u32>(result_->members.size());
    result_->members.push_back(ModuleItem(value));
    consts.emplace(value, index);
    return index;
}

VarLocation ModuleCodegen::get_location(const SymbolEntryPtr& entry) const {
    TIRO_ASSERT_NOT_NULL(entry);
    if (auto pos = entry_to_location_.find(entry);
        pos != entry_to_location_.end()) {
        return pos->second;
    }

    TIRO_ERROR("Failed to find location: {}.", strings_.value(entry->name()));
}

} // namespace tiro::compiler
