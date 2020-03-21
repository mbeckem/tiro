#ifndef TIRO_CODEGEN_FUNC_CODEGEN_HPP
#define TIRO_CODEGEN_FUNC_CODEGEN_HPP

#include "tiro/codegen/basic_block.hpp"
#include "tiro/codegen/instructions.hpp"
#include "tiro/codegen/variable_locations.hpp"
#include "tiro/compiler/output.hpp"
#include "tiro/core/defs.hpp"
#include "tiro/core/not_null.hpp"
#include "tiro/syntax/ast.hpp"

namespace tiro {

class ModuleCodegen;

struct LoopContext {
    LoopContext* parent = nullptr;
    BasicBlock* break_label = nullptr;
    BasicBlock* continue_label = nullptr;
};

class FunctionCodegen final {
public:
    // For top level functions
    explicit FunctionCodegen(ModuleCodegen& module, u32 index_in_module);

    // For nested functions
    explicit FunctionCodegen(FunctionCodegen& parent, u32 index_in_module);

private:
    // common constructor
    explicit FunctionCodegen(
        FunctionCodegen* parent, ModuleCodegen& module, u32 index_in_module);

public:
    /// Compilation entry point. Called to generate and emit the given function declaration.
    void compile_function(NotNull<FuncDecl*> func);

    /// Compilation entry point. Called to generate and emit the given list of var declarations
    /// for the module initializer (as a function called <module_init>).
    void compile_initializer(
        NotNull<Scope*> module_scope, Span<const NotNull<DeclStmt*>> init);

    ModuleCodegen& module() { return module_; }
    u32 index_in_module() const { return index_in_module_; }

    SymbolTable& symbols() { return symbols_; }
    StringTable& strings() { return strings_; }
    Diagnostics& diag() { return diag_; }
    BasicBlockStorage& blocks() { return blocks_; }

    NotNull<BasicBlock*> make_block(InternedString title) {
        return blocks_.make_block(title);
    }

    template<typename InstructionT, typename... Args>
    NotNull<InstructionT*> make_instr(Args&&... args) {
        return instructions_.make<InstructionT>(std::forward<Args>(args)...);
    }

    FunctionCodegen(const FunctionCodegen&) = delete;
    FunctionCodegen& operator=(const FunctionCodegen&) = delete;

    /// Generates bytecode for the given expr.
    /// Returns false if if the expr generation was omitted because it was not observed.
    bool generate_expr(NotNull<Expr*> expr, CurrentBasicBlock& bb);

    /// Same as generate_expr(), but contains a debug assertion that checks
    /// that the given expression can in fact be used in a value context.
    /// Errors conditions like these are caught in the analyzer, but are checked
    /// again in here (in development builds) for extra safety.
    void generate_expr_value(NotNull<Expr*> expr, CurrentBasicBlock& bb);

    /// Generates code to produce an expression but ignores the result.
    void generate_expr_ignore(NotNull<Expr*> expr, CurrentBasicBlock& bb);

public:
    /// Generates bytecode for a statement.
    void generate_stmt(NotNull<ASTStmt*> stmt, CurrentBasicBlock& bb);

    /// Generates bytecode to load the given symbol.
    void generate_load(NotNull<Symbol*> entry, CurrentBasicBlock& bb);

    /// Generates bytecode to store the current value (top of the stack) into the given entry.
    /// If `push_value` is true, then the value will also be pushed onto the stack.
    void generate_store(NotNull<Symbol*> entry, CurrentBasicBlock& bb);

    /// Generates code to create a closure from the given nested function decl.
    void generate_closure(NotNull<FuncDecl*> decl, CurrentBasicBlock& bb);

    /// Call this function to emit the bytecode for a loop body.
    /// Loop bodies must be handled by this function because they may open their own closure context.
    void generate_loop_body(const ScopePtr& body_scope,
        NotNull<BasicBlock*> loop_start, NotNull<BasicBlock*> loop_end,
        NotNull<Expr*> body, CurrentBasicBlock& bb);

private:
    void compile_function(NotNull<Scope*> scope, ParamList* params,
        NotNull<Expr*> body, CurrentBasicBlock& bb);
    void compile_function_body(NotNull<Expr*> body, CurrentBasicBlock& bb);

    void emit(NotNull<BasicBlock*> initial);

    // Returns the closure context started by this scope, or null.
    ClosureContext* get_closure_context(NotNull<Scope*> scope) {
        return locations_.get_closure_context(scope);
    }

public:
    // Returns the location of the symbol. Errors if no matching
    // location entry was found.
    VarLocation get_location(NotNull<Symbol*> entry);

    // Load the given context. Only works for the outer context (passed in by the parent function)
    // or local context objects. Can be null if the outer context is also null.
    void load_context(ClosureContext* context, CurrentBasicBlock& bb);

    // Loads the current context.
    void load_context(CurrentBasicBlock& current);

    // Attempts to reach the context `dst` from the `start` context.
    // Returns the number of levels to that context (i.e. 0 if dst == `start` etc.).
    // It is an error if the context cannot be reached.
    u32 get_context_level(
        NotNull<ClosureContext*> start, NotNull<ClosureContext*> dst);

    // Returns the local index for the given context, if the context is local to this function.
    std::optional<u32> local_context(NotNull<ClosureContext*> context);

    // Push a closure context on the context stack.
    void push_context(NotNull<ClosureContext*> context, CurrentBasicBlock& bb);

    // Pop the current closure context. Debug asserts context is on top.
    void pop_context(NotNull<ClosureContext*> context);

    // Push a loop context on the loop stack.
    void push_loop(NotNull<LoopContext*> loop);

    // Pop the current loop context. Debug asserts context is on top.
    void pop_loop(NotNull<LoopContext*> loop);

    LoopContext* current_loop() { return current_loop_; }

private:
    // The function codgen object for the surrounding function, if any.
    // Important for closures.
    FunctionCodegen* parent_ = nullptr;

    // Module codegen object of the surrounding module.
    ModuleCodegen& module_;

    // Our index inside the surrounding module's member list.
    u32 index_in_module_ = 0;

    SymbolTable& symbols_;
    StringTable& strings_;
    Diagnostics& diag_;

    // Manages memory of instruction instances.
    InstructionStorage instructions_;

    // Manages memory of basic block instances.
    BasicBlockStorage blocks_;

    // The compilation result.
    std::unique_ptr<FunctionDescriptor> result_;

    // Locations of all variables defined in this function.
    FunctionLocations locations_;

    // The closure context captured from the outer function (if any).
    ClosureContext* outer_context_ = nullptr;

    // The current closure context - this behaves like a stack.
    ClosureContext* current_closure_ = nullptr;

    // Current loop for "break" and "continue".
    // TODO: Labeled break / continue?
    LoopContext* current_loop_ = nullptr;
};

} // namespace tiro

#endif // TIRO_CODEGEN_FUNC_CODEGEN_HPP
