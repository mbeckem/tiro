#ifndef HAMMER_COMPILER_CODEGEN_HPP
#define HAMMER_COMPILER_CODEGEN_HPP

#include "hammer/ast/node.hpp"
#include "hammer/compiler/code_builder.hpp"
#include "hammer/compiler/diagnostics.hpp"
#include "hammer/compiler/opcodes.hpp"
#include "hammer/compiler/output.hpp"
#include "hammer/compiler/string_table.hpp"
#include "hammer/core/overloaded.hpp"

#include <unordered_map>

namespace hammer {

class ModuleCodegen;
class StringTable;
class FunctionCodegen;

enum class VarLocationType {
    Param,
    Local,
    Module,
    // TODO: module_variable,
};

struct VarLocation {
    // TODO nested locations (i.e. closures)
    VarLocationType type;
    union {
        struct {
            u32 index;
        } param;

        struct {
            u32 index;
        } local;

        struct {
            bool constant;
            u32 index;
        } module;
    };
};

class FunctionCodegen {
public:
    explicit FunctionCodegen(ast::FuncDecl& func, ModuleCodegen& module,
        StringTable& strings_, Diagnostics& diag_);

    void compile();

    std::unique_ptr<CompiledFunction> take_result() {
        return std::move(result_);
    }

    FunctionCodegen(const FunctionCodegen&) = delete;
    FunctionCodegen& operator=(const FunctionCodegen&) = delete;

private:
    void visit_scopes();
    void visit_scopes(ast::Node* node);

    void compile_function_body(ast::BlockExpr* body);

    // Compiles an expression.
    void compile_expr(ast::Expr* expr);

    // Compiles an expression and triggers an error if the expression's result
    // cannot be used as a value.
    void compile_expr_value(ast::Expr* expr);

    void compile_expr_impl(ast::UnaryExpr& e);
    void compile_expr_impl(ast::BinaryExpr& e);
    void compile_expr_impl(ast::VarExpr& e);
    void compile_expr_impl(ast::DotExpr& e);
    void compile_expr_impl(ast::CallExpr& e);
    void compile_expr_impl(ast::IndexExpr& e);
    void compile_expr_impl(ast::IfExpr& e);
    void compile_expr_impl(ast::ReturnExpr& e);
    void compile_expr_impl(ast::ContinueExpr& e);
    void compile_expr_impl(ast::BreakExpr& e);
    void compile_expr_impl(ast::BlockExpr& e);
    void compile_expr_impl(ast::NullLiteral& e);
    void compile_expr_impl(ast::BooleanLiteral& e);
    void compile_expr_impl(ast::IntegerLiteral& e);
    void compile_expr_impl(ast::FloatLiteral& e);
    void compile_expr_impl(ast::StringLiteral& e);
    void compile_expr_impl(ast::SymbolLiteral& e);
    void compile_expr_impl(ast::ArrayLiteral& e);
    void compile_expr_impl(ast::TupleLiteral& e);
    void compile_expr_impl(ast::MapLiteral& e);
    void compile_expr_impl(ast::SetLiteral& e);
    void compile_expr_impl(ast::FuncLiteral& e);

    void compile_stmt(ast::Stmt* stmt);
    void compile_stmt_impl(ast::EmptyStmt&) {}
    void compile_stmt_impl(ast::AssertStmt& s);
    void compile_stmt_impl(ast::WhileStmt& s);
    void compile_stmt_impl(ast::ForStmt& s);
    void compile_stmt_impl(ast::DeclStmt& s);
    void compile_stmt_impl(ast::ExprStmt& s);

    void compile_assign_expr(ast::BinaryExpr* assign);
    void
    compile_member_assign(ast::DotExpr* lhs, ast::Expr* rhs, bool push_value);
    void
    compile_index_assign(ast::IndexExpr* lhs, ast::Expr* rhs, bool push_value);
    void compile_decl_assign(ast::Decl* lhs, ast::Expr* rhs, bool push_value);

    void compile_logical_and(ast::Expr* lhs, ast::Expr* rhs);
    void compile_logical_or(ast::Expr* lhs, ast::Expr* rhs);

private:
    struct LoopContext {
        LabelID break_label;
        LabelID continue_label;
    };

    struct ConstantHasher {
        size_t operator()(const CompiledOutput* o) const noexcept {
            HAMMER_ASSERT_NOT_NULL(o);
            return o->hash();
        }
    };

    struct ConstantCompare {
        bool operator()(
            const CompiledOutput* o1, const CompiledOutput* o2) const noexcept {
            HAMMER_ASSERT_NOT_NULL(o1);
            HAMMER_ASSERT_NOT_NULL(o2);
            return o1->equals(o2);
        }
    };

private:
    VarLocation get_location(ast::Decl* decl) const;

    // Returns an existing constant index or creates a new entry in the constant table.
    u32 constant(std::unique_ptr<CompiledOutput> out);

    // Inserts unconditionally
    u32 insert_constant(std::unique_ptr<CompiledOutput> out);

private:
    ast::FuncDecl& func_;
    ModuleCodegen& module_;
    StringTable& strings_;
    Diagnostics& diag_;
    std::unique_ptr<CompiledFunction> result_;

    // Maps symbols to their storage location.
    std::unordered_map<ast::Decl*, VarLocation> decl_to_location_;

    // Used when visiting the function's scopes to compute storage locations.
    u32 next_param_ = 0;
    u32 next_local_ = 0;
    u32 max_local_ = 0;

    // Maps values to their index in the constant table.
    std::unordered_map<const CompiledOutput*, u32, ConstantHasher,
        ConstantCompare>
        constant_to_index_;

    // Current loop for "break" and "continue".
    // TODO: Labeled break / continue?
    LoopContext* current_loop_ = nullptr;

    // Writes into result_.code
    CodeBuilder builder_;
};

class ModuleCodegen {
public:
    ModuleCodegen(ast::File& file, StringTable& strings, Diagnostics& diag);

    void compile();

    std::unique_ptr<CompiledModule> take_result() { return std::move(result_); }

    VarLocation get_location(ast::Decl* decl) const;

    ModuleCodegen(const ModuleCodegen&) = delete;
    ModuleCodegen& operator=(const ModuleCodegen&) = delete;

private:
    ast::File& file_;
    StringTable& strings_;
    Diagnostics& diag_;
    std::unique_ptr<CompiledModule> result_;

    // Maps module level declarations to their location.
    std::unordered_map<ast::Decl*, VarLocation> decl_to_location_;
};

} // namespace hammer

#endif // HAMMER_COMPILER_CODEGEN_HPP
