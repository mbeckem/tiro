#include "compiler/ir_gen/module.hpp"

#include "common/fix.hpp"
#include "compiler/ast/ast.hpp"
#include "compiler/ir/module.hpp"
#include "compiler/ir_gen/func.hpp"
#include "compiler/semantics/analysis.hpp"

namespace tiro::ir {

ModuleIRGen::ModuleIRGen(ModuleContext ctx, Module& result)
    : ctx_(ctx)
    , result_(result) {
    start();
}

NotNull<AstModule*> ModuleIRGen::module() const {
    return ctx_.ast.root();
}

const AstNodeMap& ModuleIRGen::nodes() const {
    return ctx_.ast.nodes();
}

const TypeTable& ModuleIRGen::types() const {
    return ctx_.ast.types();
}

const SymbolTable& ModuleIRGen::symbols() const {
    return ctx_.ast.symbols();
}

StringTable& ModuleIRGen::strings() const {
    return ctx_.ast.strings();
}

void ModuleIRGen::compile_module() {
    while (!jobs_.empty()) {
        FunctionJob job = std::move(jobs_.front());
        jobs_.pop();

        const auto function_type = job.env ? FunctionType::Closure : FunctionType::Normal;
        Function function(job.decl->name(), function_type, strings());
        FunctionContext fctx{*this, TIRO_NN(job.envs.get()), job.env};
        FunctionIRGen function_ctx(fctx, function);
        function_ctx.compile_function(job.decl);

        const auto function_id = result_.make(std::move(function));
        result_[job.member].data(ModuleMemberData::make_function(function_id));
    }
}

ModuleMemberId ModuleIRGen::find_symbol(SymbolId symbol) const {
    if (auto pos = symbol_to_member_.find(symbol); pos != symbol_to_member_.end())
        return pos->second;
    return {};
}

SymbolId ModuleIRGen::find_definition(ModuleMemberId member) const {
    if (auto pos = member_to_symbol_.find(member); pos != member_to_symbol_.end())
        return pos->second;
    return {};
}

ModuleMemberId ModuleIRGen::add_function(
    NotNull<AstFuncDecl*> decl, NotNull<ClosureEnvCollection*> envs, ClosureEnvId env) {
    auto symbol = symbols().get_decl(decl->id());
    if (auto pos = symbol_to_member_.find(symbol); pos != symbol_to_member_.end()) {
        // TODO: Functions may be visited more than once at the moment: Once from the toplevel function
        // crawl and once for variable initializers. This should be fixed, but in the meantime we return
        // the existing entry.
        return pos->second;
    }

    auto member = enqueue_function_job(decl, envs, env);
    link(symbol, member);
    return member;
}

bool ModuleIRGen::is_static(ScopeId scope) const {
    TIRO_DEBUG_ASSERT(scope, "invalid scope");
    const auto type = symbols()[scope].type();
    switch (type) {
    case ScopeType::Global:
    case ScopeType::Module:
    case ScopeType::File:
        return true;

    case ScopeType::Function:
    case ScopeType::ForStatement:
    case ScopeType::Block:
        return false;
    }

    TIRO_UNREACHABLE("invalid scope type");
    return false;
}

bool ModuleIRGen::is_static(SymbolId symbol) const {
    TIRO_DEBUG_ASSERT(symbol, "invalid symbol");
    const auto scope = symbols()[symbol].parent();
    return is_static(scope);
}

void ModuleIRGen::start() {
    const auto& symbols = this->symbols();

    // Called for every symbol that should be implemented using a module member.
    bool has_vars = false;
    auto visit_static_symbol = [&](auto symbol_id) {
        const auto& symbol = symbols[symbol_id];
        const auto member_id = [&]() {
            switch (symbol.type()) {
            case SymbolType::Variable:
                has_vars = true;
                return result_.make(ModuleMemberData::make_variable(symbol.name()));
            case SymbolType::Import: {
                InternedString name = symbol.data().as_import().path;
                return result_.make(ModuleMemberData::make_import(name));
            }
            case SymbolType::Function: {
                auto envs = make_ref<ClosureEnvCollection>();
                auto node = try_cast<AstFuncDecl>(nodes().get_node(symbol.node()));
                return enqueue_function_job(TIRO_NN(node), TIRO_NN(envs.get()), {});
            }
            default:
                TIRO_ERROR("Unexpected symbol type at static scope: {}.", symbol.type());
            }
        }();

        if (symbol.exported()) {
            auto& member = result_[member_id];
            member.exported(true);
        }

        link(symbol_id, member_id);
    };

    // Called for every potentially static scope (currenty global -> module -> file).
    Fix visit_static_scopes = [&](auto& self, ScopeId scope_id) {
        if (!is_static(scope_id))
            return;

        const auto& scope = symbols[scope_id];
        for (const auto& entry : scope.entries())
            visit_static_symbol(entry);
        for (const auto& child : scope.children())
            self(child);
    };
    visit_static_scopes(symbols.get_scope(module()->id()));

    // Initializer for module level variables.
    if (has_vars) {
        auto envs = make_ref<ClosureEnvCollection>();

        Function function(strings().insert("<module_init>"), FunctionType::Normal, strings());
        FunctionContext fctx{*this, TIRO_NN(envs.get()), ClosureEnvId()};
        FunctionIRGen function_ctx(fctx, function);
        function_ctx.compile_initializer(module());

        auto function_id = result_.make(std::move(function));
        auto member_id = result_.make(ModuleMemberData::make_function(function_id));
        result_.init(member_id);
    }
}

ModuleMemberId ModuleIRGen::enqueue_function_job(
    NotNull<AstFuncDecl*> decl, NotNull<ClosureEnvCollection*> envs, ClosureEnvId env) {
    // Generate an invalid function member for a unique id value.
    // The member will be overwritten with the actual compiled function
    // as soon as the compilation job has executed.
    auto member = result_.make(ModuleMemberData::make_function({}));
    jobs_.push({decl, member, ref(envs.get()), env});
    return member;
}

void ModuleIRGen::link(SymbolId symbol, ModuleMemberId member) {
    TIRO_DEBUG_ASSERT(symbol_to_member_.count(symbol) == 0, "Symbol id must be unique.");
    TIRO_DEBUG_ASSERT(member_to_symbol_.count(member) == 0, "Member id must be unique.");

    symbol_to_member_.emplace(symbol, member);
    member_to_symbol_.emplace(member, symbol);
}

} // namespace tiro::ir
