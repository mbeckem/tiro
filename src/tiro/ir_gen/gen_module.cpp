#include "tiro/ir_gen/gen_module.hpp"

#include "tiro/ir_gen/gen_func.hpp"

namespace tiro {

static InternedString
imported_name(NotNull<ImportDecl*> decl, StringTable& strings) {
    std::string joined_string;
    for (auto element : decl->path_elements()) {
        if (!joined_string.empty())
            joined_string += ".";
        joined_string += strings.value(element);
    }
    return strings.insert(joined_string);
}

ModuleIRGen::ModuleIRGen(NotNull<Root*> module, Module& result,
    Diagnostics& diag, StringTable& strings)
    : module_(module)
    , diag_(diag)
    , strings_(strings)
    , result_(result) {
    start();
}

void ModuleIRGen::compile_module() {
    while (!jobs_.empty()) {
        FunctionJob job = std::move(jobs_.front());
        jobs_.pop();

        const auto function_type = job.env ? FunctionType::Closure
                                           : FunctionType::Normal;
        Function function(job.decl->name(), function_type, strings_);
        FunctionIRGen function_ctx(
            *this, TIRO_NN(job.envs.get()), job.env, function, diag_, strings_);
        function_ctx.compile_function(job.decl);

        const auto function_id = result_.make(std::move(function));
        *result_[job.member] = ModuleMember::make_function(function_id);
    }
}

ModuleMemberID ModuleIRGen::find_symbol(NotNull<Symbol*> symbol) const {
    if (auto pos = members_.find(symbol); pos != members_.end())
        return pos->second;
    return {};
}

ModuleMemberID ModuleIRGen::add_function(NotNull<FuncDecl*> decl,
    NotNull<ClosureEnvCollection*> envs, ClosureEnvID env) {
    // Generate an invalid function member for a unique id value.
    // The member will be overwritten with the actual compiled function
    // as soon as the compilation job has executed.
    auto member = result_.make(ModuleMember::make_function({}));
    jobs_.push({decl, member, ref(envs.get()), env});
    return member;
}

void ModuleIRGen::start() {
    // FIXME need a module scope!

    auto file = TIRO_NN(module_->file());
    auto file_scope = file->file_scope();

    bool has_vars = false;

    for (const auto& module_member : file_scope->entries()) {
        const auto symbol = TIRO_NN(module_member.get());
        const auto member = [&]() {
            switch (symbol->type()) {
            case SymbolType::ModuleVar:
                has_vars = true;
                return result_.make(
                    ModuleMember::make_variable(symbol->name()));
            case SymbolType::Import: {
                // TODO better symbols
                InternedString name = imported_name(
                    TIRO_NN(must_cast<ImportDecl>(symbol->decl())), strings_);
                return result_.make(ModuleMember::make_import(name));
            }
            case SymbolType::Function: {
                auto envs = make_ref<ClosureEnvCollection>();
                return add_function(
                    TIRO_NN(must_cast<FuncDecl>(symbol->decl())),
                    TIRO_NN(envs.get()), {});
            }
            default:
                TIRO_ERROR("Unexpected symbol type at module scope: {}.",
                    symbol->type());
            }
        }();

        members_.emplace(TIRO_NN(module_member.get()), member);
    }

    // Initializer for module level variables.
    if (has_vars) {
        auto envs = make_ref<ClosureEnvCollection>();

        Function function(
            strings_.insert("<module_init>"), FunctionType::Normal, strings_);
        FunctionIRGen function_ctx(
            *this, TIRO_NN(envs.get()), {}, function, diag_, strings_);
        function_ctx.compile_initializer(file);

        auto function_id = result_.make(std::move(function));
        auto member_id = result_.make(ModuleMember::make_function(function_id));
        result_.init(member_id);
    }
}

} // namespace tiro
