#include "tiro/ir_gen/gen_module.hpp"

#include "tiro/ast/ast.hpp"
#include "tiro/ir/module.hpp"
#include "tiro/ir_gen/gen_func.hpp"

namespace tiro {

ModuleIRGen::ModuleIRGen(NotNull<AstFile*> module, Module& result,
    const SymbolTable& symbols, StringTable& strings, Diagnostics& diag)
    : module_(module)
    , symbols_(symbols)
    , strings_(strings)
    , diag_(diag)
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

ModuleMemberId ModuleIRGen::find_symbol(SymbolId symbol) const {
    if (auto pos = members_.find(symbol); pos != members_.end())
        return pos->second;
    return {};
}

ModuleMemberId ModuleIRGen::add_function(NotNull<AstFuncDecl*> decl,
    NotNull<ClosureEnvCollection*> envs, ClosureEnvId env) {
    // Generate an invalid function member for a unique id value.
    // The member will be overwritten with the actual compiled function
    // as soon as the compilation job has executed.
    auto member = result_.make(ModuleMember::make_function({}));
    jobs_.push({decl, member, ref(envs.get()), env});
    return member;
}

void ModuleIRGen::start() {
    auto file_scope_id = symbols_.get_scope(module_->id());
    auto file_scope = symbols_[file_scope_id];

    bool has_vars = false;
    for (const auto& member_id : file_scope->entries()) {
        auto symbol = symbols_[member_id];
        const auto member = [&]() {
            switch (symbol->type()) {
            case SymbolType::Variable:
                has_vars = true;
                return result_.make(
                    ModuleMember::make_variable(symbol->name()));
            case SymbolType::Import: {
                // TODO better symbols
                InternedString name = symbol->data().as_import().path;
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
