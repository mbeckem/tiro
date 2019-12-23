#include "hammer/compiler/codegen/codegen.hpp"

#include "hammer/ast/visit.hpp"
#include "hammer/ast/scope.hpp"
#include "hammer/compiler/analyzer.hpp"
#include "hammer/compiler/codegen/expr_codegen.hpp"
#include "hammer/compiler/codegen/stmt_codegen.hpp"
#include "hammer/core/defs.hpp"
#include "hammer/core/overloaded.hpp"
#include "hammer/core/scope.hpp"

namespace hammer {

FunctionCodegen::FunctionCodegen(ast::FuncDecl& func, ModuleCodegen& module,
    u32 index_in_module, StringTable& strings, Diagnostics& diag)
    : FunctionCodegen(func, nullptr, module, index_in_module, strings, diag) {}

FunctionCodegen::FunctionCodegen(ast::FuncDecl& func, FunctionCodegen& parent)
    : FunctionCodegen(func, &parent, parent.module(),
        parent.module().add_function(), parent.strings(), parent.diag()) {}

FunctionCodegen::FunctionCodegen(ast::FuncDecl& func, FunctionCodegen* parent,
    ModuleCodegen& module, u32 index_in_module, StringTable& strings,
    Diagnostics& diag)
    : func_(func)
    , parent_(parent)
    , module_(module)
    , index_in_module_(index_in_module)
    , strings_(strings)
    , diag_(diag)
    // TODO: Nested functions without captured variables dont need type == template
    , result_(std::make_unique<FunctionDescriptor>(
          parent ? FunctionDescriptor::TEMPLATE : FunctionDescriptor::FUNCTION))
    , locations_()
    , builder_(result_->code) {

    if (parent_) {
        outer_context_ = parent_->current_closure_;
        current_closure_ = outer_context_;
    }

    result_->name = func.name();
}

void FunctionCodegen::compile() {
    locations_ = FunctionLocations::compute(&func_);
    result_->params = locations_.params();
    result_->locals = locations_.locals();

    compile_function(&func_);
    builder_.finish();
    module_.set_function(index_in_module_, std::move(result_));
}

void FunctionCodegen::compile_function(ast::FuncDecl* func) {
    HAMMER_ASSERT_NOT_NULL(func);

    ClosureContext* context = get_closure_context(func);
    if (context)
        push_closure(context);

    {
        const size_t params = func->param_count();
        for (size_t i = 0; i < params; ++i) {
            ast::ParamDecl* param = func->get_param(i);

            // Move captured params from the stack to the closure context.
            VarLocation loc = get_location(param);
            if (loc.type == VarLocationType::Context) {
                HAMMER_ASSERT(context != nullptr,
                    "Must have a local context if params are captured.");
                load_context(context);
                builder_.load_param(as_u32(i));
                builder_.store_context(0, loc.context.index);
            }
        }
    }

    compile_function_body(func->body());

    if (context)
        pop_closure();
}

void FunctionCodegen::compile_function_body(ast::BlockExpr* body) {
    HAMMER_ASSERT_NOT_NULL(body);

    generate_expr(body);
    switch (body->expr_type()) {
    case ast::ExprType::Value:
        builder_.ret();
        break;
    case ast::ExprType::Never:
        // Nothing, control flow doesn't get here.
        break;
    case ast::ExprType::None:
        builder_.load_null();
        builder_.ret();
        break;
    }
}

void FunctionCodegen::generate_expr(ast::Expr* expr) {
    HAMMER_ASSERT_NOT_NULL(expr);

    ExprCodegen gen(*expr, *this);
    gen.generate();
}

void FunctionCodegen::generate_expr_value(ast::Expr* expr) {
    HAMMER_ASSERT_NOT_NULL(expr);
    HAMMER_ASSERT(expr->can_use_as_value(),
        "Cannot use this expression in a value context.");
    return generate_expr(expr);
}

void FunctionCodegen::generate_stmt(ast::Stmt* stmt) {
    HAMMER_ASSERT_NOT_NULL(stmt);

    StmtCodegen gen(*stmt, *this);
    gen.generate();
}

void FunctionCodegen::generate_load(ast::Decl* decl) {
    HAMMER_ASSERT_NOT_NULL(decl);

    VarLocation loc = get_location(decl);

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
        if (auto local = local_context(loc.context.ctx)) {
            builder_.load_local(*local);
            builder_.load_context(0, loc.context.index);
        } else {
            const u32 levels = get_context_level(
                outer_context_, loc.context.ctx);
            load_context(outer_context_);
            builder_.load_context(levels, loc.context.index);
        }
        break;
    }
    }
}

void FunctionCodegen::generate_store(
    ast::Decl* decl, ast::Expr* rhs, bool push_value) {
    HAMMER_ASSERT_NOT_NULL(decl);
    HAMMER_ASSERT_NOT_NULL(rhs);

    VarLocation loc = get_location(decl);
    switch (loc.type) {
    case VarLocationType::Param: {
        generate_expr_value(rhs);
        if (push_value) {
            builder_.dup();
        }

        builder_.store_param(loc.param.index);
        break;
    }
    case VarLocationType::Local: {
        generate_expr_value(rhs);
        if (push_value) {
            builder_.dup();
        }

        builder_.store_local(loc.local.index);
        break;
    }
    case VarLocationType::Module: {
        generate_expr_value(rhs);
        if (push_value) {
            builder_.dup();
        }

        builder_.store_module(loc.module.index);
        break;
    }
    case VarLocationType::Context: {
        u32 levels = 0;
        if (auto local = local_context(loc.context.ctx)) {
            builder_.load_local(*local);
        } else {
            levels = get_context_level(outer_context_, loc.context.ctx);
            load_context(outer_context_);
        }

        generate_expr_value(rhs);
        if (push_value) {
            builder_.dup();
            builder_.rot_3();
        }

        builder_.store_context(levels, loc.context.index);
        break;
    }
    }
}

void FunctionCodegen::generate_closure(ast::FuncDecl* decl) {
    HAMMER_ASSERT_NOT_NULL(decl);

    // TODO: A queue of compilation jobs would be nicer than a recursive call here.
    // TODO: Lambda names in the module
    // TODO: No closure template when no capture vars.
    FunctionCodegen nested(*decl, *this);
    nested.compile();

    builder_.load_module(nested.index_in_module());
    load_context();
    builder_.mk_closure();
}

void FunctionCodegen::generate_loop_body(
    LabelID break_label, LabelID continue_label, ast::Expr* body) {
    HAMMER_ASSERT_NOT_NULL(body);

    LoopContext loop;
    loop.parent = current_loop_;
    loop.break_label = break_label;
    loop.continue_label = continue_label;
    push_loop(&loop);

    ClosureContext* context = get_closure_context(body);
    if (context)
        push_closure(context);

    generate_expr(body);
    if (body->expr_type() == ast::ExprType::Value)
        builder_.pop();

    if (context)
        pop_closure();

    pop_loop();
}

u32 FunctionCodegen::get_context_level(
    ClosureContext* start, ClosureContext* dst) {
    HAMMER_ASSERT_NOT_NULL(dst);

    ClosureContext* ctx = start;
    u32 level = 0;
    while (ctx) {
        if (ctx == dst)
            return level;

        ++level;
        ctx = ctx->parent;
    }

    HAMMER_ERROR("Failed to reach destination closure context.");
}

VarLocation FunctionCodegen::get_location(ast::Decl* decl) {
    HAMMER_ASSERT_NOT_NULL(decl);

    if (auto loc = locations_.get_location(decl))
        return *loc;

    if (parent_) {
        auto loc = parent_->get_location(decl);
        HAMMER_ASSERT(loc.type == VarLocationType::Module
                          || loc.type == VarLocationType::Context,
            "Must be a module or a closure location.");
        return loc;
    }

    auto loc = module_.get_location(decl);
    HAMMER_ASSERT(
        loc.type == VarLocationType::Module, "Must be a module location.");
    return loc;
}

std::optional<u32> FunctionCodegen::local_context(ClosureContext* context) {
    HAMMER_ASSERT_NOT_NULL(context);

    if (context->func == &func_) {
        return context->local_index;
    }
    return {};
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

    if (auto local = local_context(context)) {
        builder_.load_local(*local);
        return;
    }

    HAMMER_UNREACHABLE("Cannot load the given context.");
}

void FunctionCodegen::load_context() {
    load_context(current_closure_);
}

void FunctionCodegen::push_loop(LoopContext* loop) {
    HAMMER_ASSERT_NOT_NULL(loop);
    HAMMER_ASSERT(
        loop->parent == current_loop_, "Must be a child of the current loop.");
    current_loop_ = loop;
}

void FunctionCodegen::pop_loop() {
    HAMMER_ASSERT_NOT_NULL(current_loop_);
    current_loop_ = current_loop_->parent;
}

void FunctionCodegen::push_closure(ClosureContext* context) {
    HAMMER_ASSERT_NOT_NULL(context);
    HAMMER_ASSERT(context->parent == current_closure_,
        "Must be a child of the current closure context.");
    HAMMER_ASSERT(context->size > 0,
        "Frontend must never generate 0-sized context objects.");

    auto local = local_context(context);
    HAMMER_ASSERT(local, "Must be a local context.");
    HAMMER_ASSERT(*local < locations_.locals(), "Invalid local index.");

    load_context();
    builder_.mk_context(context->size);
    builder_.store_local(*local);

    current_closure_ = context;
}

void FunctionCodegen::pop_closure() {
    HAMMER_ASSERT_NOT_NULL(current_closure_);
    current_closure_ = current_closure_->parent;
}

ModuleCodegen::ModuleCodegen(
    ast::File& file, StringTable& strings, Diagnostics& diag)
    : file_(file)
    , strings_(strings)
    , diag_(diag)
    , result_(std::make_unique<CompiledModule>()) {
    // FIXME strip extensions and stuff, compute full name.
    result_->name = file.file_name();
}

void ModuleCodegen::compile() {
    const size_t items = file_.item_count();

    std::vector<ast::ImportDecl*> imports;
    std::vector<ast::FuncDecl*> functions;

    for (size_t i = 0; i < items; ++i) {
        ast::Node* item = file_.get_item(i);

        if (ast::ImportDecl* decl = try_cast<ast::ImportDecl>(item)) {
            imports.push_back(decl);
            continue;
        }

        if (ast::FuncDecl* decl = try_cast<ast::FuncDecl>(item)) {
            functions.push_back(decl);
            continue;
        }

        HAMMER_ERROR("Invalid node of type {} at module level.",
            to_string(item->kind()));
    }

    auto insert_loc = [&](ast::Decl* decl, u32 index, bool constant) {
        HAMMER_ASSERT(!decl_to_location_.count(decl), "Decl already indexed.");
        VarLocation loc;
        loc.type = VarLocationType::Module;
        loc.module.index = index;
        loc.module.constant = constant;
        decl_to_location_.emplace(decl, loc);
    };

    // TODO Queue that can be accessed for recursive compilations?
    std::vector<std::unique_ptr<FunctionCodegen>> jobs;

    for (auto* i : imports) {
        HAMMER_ASSERT(i->name(), "Invalid name.");
        const u32 index = add_import(i->name());
        insert_loc(i, index, true);
    }

    for (auto* func : functions) {
        const u32 index = add_function();
        insert_loc(func, index, true);

        // Don't compile yet, gather locations for other module-level functions.
        jobs.push_back(std::make_unique<FunctionCodegen>(
            *func, *this, index, strings_, diag_));
    }

    for (const auto& job : jobs) {
        job->compile();
    }

    // Validate all function slots.
    for (const auto& member : result_->members) {
        if (member.which() == ModuleItem::Which::Function) {
            const ModuleItem::Function& func = member.get_function();
            HAMMER_CHECK(func.value,
                "Logic error: function pointer was not set to a valid value.");
        }
    }
}

u32 ModuleCodegen::add_function() {
    HAMMER_ASSERT_NOT_NULL(result_);

    const u32 index = as_u32(result_->members.size());
    result_->members.push_back(ModuleItem::make_func(nullptr));
    return index;
}

void ModuleCodegen::set_function(
    u32 index, std::unique_ptr<FunctionDescriptor> func) {
    HAMMER_ASSERT_NOT_NULL(result_);
    HAMMER_ASSERT(
        index < result_->members.size(), "Function index out of bounds.");

    ModuleItem& item = result_->members[index];
    HAMMER_ASSERT(item.which() == ModuleItem::Which::Function,
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
    HAMMER_ASSERT_NOT_NULL(result_);

    if (auto pos = consts.find(value); pos != consts.end()) {
        return pos->second;
    }

    const u32 index = as_u32(result_->members.size());
    result_->members.push_back(ModuleItem(value));
    consts.emplace(value, index);
    return index;
}

VarLocation ModuleCodegen::get_location(ast::Decl* decl) const {
    HAMMER_ASSERT_NOT_NULL(decl);
    if (auto pos = decl_to_location_.find(decl);
        pos != decl_to_location_.end()) {
        return pos->second;
    }

    HAMMER_ERROR("Failed to find location: {}", strings_.value(decl->name()));
}

} // namespace hammer
