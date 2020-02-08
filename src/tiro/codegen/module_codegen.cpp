#include "tiro/codegen/module_codegen.hpp"

#include "tiro/codegen/func_codegen.hpp"
#include "tiro/semantics/analyzer.hpp"

namespace tiro::compiler {

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

    struct FunctionJob {
        NotNull<FuncDecl*> func;
        std::unique_ptr<FunctionCodegen> gen;
    };

    // TODO Queue that can be accessed for recursive compilations?
    // TODO small vec?
    std::vector<FunctionJob> func_jobs;
    std::vector<NotNull<DeclStmt*>> var_jobs;

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

            FunctionJob job{
                TIRO_NN(decl), std::make_unique<FunctionCodegen>(*this, index)};
            func_jobs.push_back(std::move(job));
            continue;
        }

        if (auto decl = try_cast<DeclStmt>(item)) {
            const auto bindings = TIRO_NN(decl->bindings());
            for (const auto binding : bindings->entries()) {
                visit_vars(TIRO_NN(binding), [&](VarDecl* var) {
                    const u32 index = add_variable(var->name());
                    insert_loc(var->declared_symbol(), index, var->is_const());
                });
            }

            var_jobs.push_back(TIRO_NN(decl));
            continue;
        }

        TIRO_ERROR("Invalid node of type {} at module level.",
            to_string(item->type()));
    }

    for (const auto& job : func_jobs) {
        job.gen->compile_function(job.func);
    }

    if (!var_jobs.empty()) {
        const u32 init_index = add_function();
        FunctionCodegen init_gen(*this, init_index);

        const auto scope = root_->root_scope();
        init_gen.compile_initializer(TIRO_NN(scope.get()), var_jobs);
        result_->init = init_index;
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
    item.get_function().value = std::move(func).get();
}

u32 ModuleCodegen::add_variable([[maybe_unused]] InternedString name) {
    // TODO put name into debug info
    const u32 index = checked_cast<u32>(result_->members.size());
    result_->members.push_back(ModuleItem::make_variable());
    return index;
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

VarLocation ModuleCodegen::get_location(NotNull<SymbolEntry*> entry) const {
    if (auto pos = entry_to_location_.find(ref(entry.get()));
        pos != entry_to_location_.end()) {
        return pos->second;
    }

    TIRO_ERROR("Failed to find location: {}.", strings_.value(entry->name()));
}

} // namespace tiro::compiler
