#include "tiro/ir_gen/gen_decl.hpp"

#include "tiro/ast/ast.hpp"
#include "tiro/core/math.hpp"
#include "tiro/semantics/symbol_table.hpp"

namespace tiro {

namespace {

struct BindingVisitor {
public:
    explicit BindingVisitor(CurrentBlock& bb)
        : bb_(bb) {}

    OkResult visit_var_binding(NotNull<AstVarBinding*> b) {
        auto symbol_id = symbols().get_decl(SymbolKey::for_node(b->id()));

        if (const auto& init = b->init()) {
            auto value = bb_.compile_expr(TIRO_NN(init));
            if (!value)
                return value.failure();

            bb_.compile_assign(symbol_id, *value);
        }
        return ok;
    }

    // TODO: If the initializer is a tuple literal (i.e. known contents at compile time)
    // we can skip generating the complete tuple and assign the individual variables directly.
    // We could also implement tuple construction at compilation time (const_eval.cpp) to optimize
    // this after the fact.
    OkResult visit_tuple_binding(NotNull<AstTupleBinding*> b) {
        if (const auto& init = b->init()) {
            auto tuple = bb_.compile_expr(TIRO_NN(init));
            if (!tuple)
                return tuple.failure();

            const u32 var_count = checked_cast<u32>(b->names().size());
            if (var_count == 0)
                return ok;

            for (u32 i = 0; i < var_count; ++i) {
                auto symbol = symbols().get_decl(SymbolKey::for_element(b->id(), i));

                auto element = bb_.compile_rvalue(
                    RValue::UseLValue{LValue::make_tuple_field(*tuple, i)});
                bb_.compile_assign(symbol, element);
            }
        }
        return ok;
    }

private:
    const SymbolTable& symbols() const { return bb_.ctx().symbols(); }

private:
    CurrentBlock& bb_;
};

} // namespace

OkResult gen_var_decl(NotNull<AstVarDecl*> decl, CurrentBlock& bb) {
    BindingVisitor visitor(bb);
    for (auto binding : decl->bindings()) {
        auto result = visit(TIRO_NN(binding), visitor);
        if (!result)
            return result.failure();
    }
    return ok;
}

} // namespace tiro
