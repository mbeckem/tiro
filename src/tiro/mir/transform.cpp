#include "tiro/mir/transform.hpp"

#include "tiro/mir/types.hpp"
#include "tiro/mir/vec_ptr.hpp"

namespace tiro::compiler {

using mir::Constant;
using mir::LValue;
using mir::RValue;
using mir::Local;
using mir::LocalID;
using mir::ScopeID;

namespace {

class CurrentBlock final {
public:
    CurrentBlock(NotNull<mir::VecPtr<mir::Block>> block)
        : block_(block) {}

    CurrentBlock(const CurrentBlock&) = delete;
    CurrentBlock& operator=(const CurrentBlock&) = delete;

    mir::VecPtr<mir::Block> get() const { return block_; }
    mir::VecPtr<mir::Block> operator->() const { return block_; }

    void assign(NotNull<mir::VecPtr<mir::Block>> block) { block_ = block; }

    void emit(const mir::Stmt& stmt) {
        TIRO_ASSERT(block_->edge().type() == mir::EdgeType::None,
            "Must not emit statements into a block which already has an "
            "outgoing edge.");
        get()->append(stmt);
    }

private:
    NotNull<mir::VecPtr<mir::Block>> block_;
};

struct Unreachable {};
struct Omitted {};

inline constexpr Unreachable unreachable;
inline constexpr Omitted omitted;

enum class ExprResultType { Local, Unreachable, Omitted };

class ExprResult final {
public:
    ExprResult(LocalID value)
        : type_(ExprResultType::Local)
        , local_(value) {}

    ExprResult(Unreachable)
        : type_(ExprResultType::Unreachable) {}

    ExprResult(Omitted)
        : type_(ExprResultType::Omitted) {}

    LocalID local() const {
        TIRO_ASSERT(is_local(), "ExprResult is not a local.");
        return local_;
    }

    LocalID operator*() const { return local(); }

    ExprResultType type() const noexcept { return type_; }

    bool is_local() const noexcept { return type_ == ExprResultType::Local; }
    explicit operator bool() const noexcept { return is_local(); }

private:
    ExprResultType type_;
    LocalID local_;
};

class Context final {
public:
    Context(mir::Function& result, StringTable& strings)
        : result_(result)
        , strings_(strings) {}

    mir::Function& result() const { return result_; }
    StringTable& strings() const { return strings_; }

private:
    mir::Function& result_;
    StringTable& strings_;
};

LocalID define(mir::Function& result, CurrentBlock& bb, const Local& local) {
    auto id = result.make(local);
    bb.emit(mir::Stmt::Define{id});
    return id;
}

class ExpressionTransformer final {
public:
    ExpressionTransformer(Context& ctx, CurrentBlock& bb)
        : ctx_(ctx)
        , bb_(bb) {}

    StringTable& strings() const { return ctx_.strings(); }
    mir::Function& result() const { return ctx_.result(); }

    // TODO
    ScopeID current_scope() const { return ScopeID(); }

    ExprResult dispatch(NotNull<Expr*> expr) {
        TIRO_ASSERT(!expr->has_error(),
            "Nodes with errors must not reach the mir transformation stage.");
        return visit(expr, *this);
    }

    ExprResult visit_binary_expr(BinaryExpr* expr) {
        (void) expr;
        TIRO_NOT_IMPLEMENTED();
    }

    ExprResult visit_block_expr(BlockExpr* expr) {
        (void) expr;
        TIRO_NOT_IMPLEMENTED();
    }

    ExprResult visit_break_expr(BreakExpr* expr) {
        (void) expr;
        TIRO_NOT_IMPLEMENTED();
    }

    ExprResult visit_call_expr(CallExpr* expr) {
        (void) expr;
        TIRO_NOT_IMPLEMENTED();
    }

    ExprResult visit_continue_expr(ContinueExpr* expr) {
        (void) expr;
        TIRO_NOT_IMPLEMENTED();
    }

    ExprResult visit_dot_expr(DotExpr* expr) {
        (void) expr;
        TIRO_NOT_IMPLEMENTED();
    }

    ExprResult visit_if_expr(IfExpr* expr) {
        (void) expr;
        TIRO_NOT_IMPLEMENTED();
    }

    ExprResult visit_index_expr(IndexExpr* expr) {
        (void) expr;
        TIRO_NOT_IMPLEMENTED();
    }

    ExprResult visit_interpolated_string_expr(InterpolatedStringExpr* expr) {
        (void) expr;
        TIRO_NOT_IMPLEMENTED();
    }

    ExprResult visit_array_literal(ArrayLiteral* expr) {
        (void) expr;
        TIRO_NOT_IMPLEMENTED();
    }

    ExprResult visit_boolean_literal(BooleanLiteral* expr) {
        auto value = expr->value() ? Constant(Constant::True{})
                                   : Constant(Constant::False{});
        auto local = Local::temp(current_scope(), RValue(value));
        return define(local);
    }

    ExprResult visit_float_literal(FloatLiteral* expr) {
        auto local = Local::temp(
            current_scope(), RValue(Constant::Float{expr->value()}));
        return define(local);
    }

    ExprResult visit_func_literal(FuncLiteral* expr) {
        (void) expr;
        TIRO_NOT_IMPLEMENTED();
    }

    ExprResult visit_integer_literal(IntegerLiteral* expr) {
        auto local = Local::temp(
            current_scope(), RValue(Constant::Integer{expr->value()}));
        return define(local);
    }

    ExprResult visit_map_literal(MapLiteral* expr) {
        (void) expr;
        TIRO_NOT_IMPLEMENTED();
    }

    ExprResult visit_null_literal([[maybe_unused]] NullLiteral* expr) {
        auto local = Local::temp(current_scope(), RValue(Constant::Null{}));
        return define(local);
    }

    ExprResult visit_set_literal(SetLiteral* expr) {
        (void) expr;
        TIRO_NOT_IMPLEMENTED();
    }

    ExprResult visit_string_literal(StringLiteral* expr) {
        TIRO_ASSERT(expr->value(), "Invalid string literal.");

        auto local = Local::temp(
            current_scope(), RValue(Constant::String{expr->value()}));
        return define(local);
    }

    ExprResult visit_symbol_literal(SymbolLiteral* expr) {
        TIRO_ASSERT(expr->value(), "Invalid symbol literal.");

        auto local = Local::temp(
            current_scope(), RValue(Constant::Symbol{expr->value()}));
        return define(local);
    }

    ExprResult visit_tuple_literal(TupleLiteral* expr) {
        (void) expr;
        TIRO_NOT_IMPLEMENTED();
    }

    ExprResult visit_return_expr(ReturnExpr* expr) {
        LocalID local;
        if (auto inner = expr->inner()) {
            // TODO: Dont omit, unreachable is OK.
            auto result = dispatch(TIRO_NN(inner));
            if (!result)
                return result;
            local = *result;
        } else {
            local = define(
                Local::temp(current_scope(), RValue(Constant::Null{})));
        }

        bb_.emit(mir::Stmt::SetReturn{local});
        bb_->edge(mir::Edge::Return{});
        return unreachable;
    }

    ExprResult visit_string_sequence_expr(StringSequenceExpr* expr) {
        (void) expr;
        TIRO_NOT_IMPLEMENTED();
    }

    ExprResult visit_tuple_member_expr(TupleMemberExpr* expr) {
        (void) expr;
        TIRO_NOT_IMPLEMENTED();
    }

    ExprResult visit_unary_expr(UnaryExpr* expr) {
        (void) expr;
        TIRO_NOT_IMPLEMENTED();
    }

    ExprResult visit_var_expr(VarExpr* expr) {
        (void) expr;
        TIRO_NOT_IMPLEMENTED();
    }

private:
    LocalID define(const Local& local) {
        return compiler::define(result(), bb_, local);
    }

private:
    Context& ctx_;
    CurrentBlock& bb_;
};

} // namespace

static void
transform_expression(Context& ctx, CurrentBlock& bb, NotNull<Expr*> expr) {
    ExpressionTransformer tx(ctx, bb);
    tx.dispatch(expr);
}

mir::Function transform(NotNull<FuncDecl*> func, StringTable& strings) {
    mir::Function mir(strings);

    (void) transform_expression;
    (void) func;

    return mir;
}

} // namespace tiro::compiler
