#ifndef HAMMER_COMPILER_CODEGEN_CODEGEN_HPP
#define HAMMER_COMPILER_CODEGEN_CODEGEN_HPP

#include "hammer/compiler/codegen/code_builder.hpp"
#include "hammer/compiler/codegen/variable_locations.hpp"
#include "hammer/compiler/diagnostics.hpp"
#include "hammer/compiler/output.hpp"
#include "hammer/compiler/string_table.hpp"

#include <unordered_set>

namespace hammer::compiler {

class ModuleCodegen;
class StringTable;
class FunctionCodegen;

struct LoopContext {
    LoopContext* parent = nullptr;
    LabelID break_label;
    LabelID continue_label;
};

class FunctionCodegen final {
public:
    // For top level functions
    explicit FunctionCodegen(const NodePtr<FuncDecl>& func,
        ModuleCodegen& module, u32 index_in_module);

    // For nested functions
    explicit FunctionCodegen(const NodePtr<FuncDecl>& decl,
        FunctionCodegen& parent, u32 index_in_module);

private:
    // common constructor
    explicit FunctionCodegen(const NodePtr<FuncDecl>& func,
        FunctionCodegen* parent, ModuleCodegen& module, u32 index_in_module);

public:
    void compile();

    ModuleCodegen& module() { return module_; }
    u32 index_in_module() const { return index_in_module_; }

    SymbolTable& symbols() { return symbols_; }
    StringTable& strings() { return strings_; }
    Diagnostics& diag() { return diag_; }
    CodeBuilder& builder() { return builder_; }

    FunctionCodegen(const FunctionCodegen&) = delete;
    FunctionCodegen& operator=(const FunctionCodegen&) = delete;

    /// Generates bytecode for the given expr.
    /// Returns false if if the expr generation was omitted because it was not observed.
    bool generate_expr(const NodePtr<Expr>& expr);

    /// Same as generate_expr(), but contains a debug assertion that checks
    /// that the given expression can in fact be used in a value context.
    /// Errors conditions like these are caught in the analyzer, but are checked
    /// again in here (in development builds) for extra safety.
    void generate_expr_value(const NodePtr<Expr>& expr);

    /// Generates code to produce an expression but ignores the result.
    void generate_expr_ignore(const NodePtr<Expr>& expr);

public:
    /// Generates bytecode for a statement.
    void generate_stmt(const NodePtr<Stmt>& stmt);

    /// Generates bytecode to load the given symbol.
    void generate_load(const SymbolEntryPtr& entry);

    /// Generates bytecode to store the current value (top of the stack) into the given entry.
    /// If `push_value` is true, then the value will also be pushed onto the stack.
    void generate_store(const SymbolEntryPtr& entry);

    /// Generates code to create a closure from the given nested function decl.
    void generate_closure(const NodePtr<FuncDecl>& decl);

    /// Call this function to emit the bytecode for a loop body.
    /// Loop bodies must be handled by this function because they may open their own closure context.
    void generate_loop_body(LabelID break_label, LabelID continue_label,
        const ScopePtr& body_scope, const NodePtr<Expr>& body);

private:
    void compile_function(const NodePtr<FuncDecl>& func);
    void compile_function_body(const NodePtr<Expr>& body);

    // Returns the closure context started by this scope, or null.
    ClosureContext* get_closure_context(const ScopePtr& scope) {
        return locations_.get_closure_context(scope);
    }

public:
    // Returns the location of the symbol. Errors if no matching
    // location entry was found.
    VarLocation get_location(const SymbolEntryPtr& entry);

    // Load the given context. Only works for the outer context (passed in by the parent function)
    // or local context objects.
    void load_context(ClosureContext* context);

    // Loads the current context.
    void load_context();

    // Attempts to reach the context `dst` from the `start` context.
    // Returns the number of levels to that context (i.e. 0 if dst == `start` etc.).
    // It is an error if the context cannot be reached.
    u32 get_context_level(ClosureContext* start, ClosureContext* dst);

    // Returns the local index for the given context, if the context is local to this function.
    std::optional<u32> local_context(ClosureContext* context);

    // Push a closure context on the context stack.
    void push_context(ClosureContext* context);

    // Pop the current closure context. Debug asserts context is on top.
    void pop_context(ClosureContext* context);

    // Push a loop context on the loop stack.
    void push_loop(LoopContext* loop);

    // Pop the current loop context. Debug asserts context is on top.
    void pop_loop(LoopContext* loop);

    LoopContext* current_loop() { return current_loop_; }

private:
    // The function we're compiling.
    NodePtr<FuncDecl> func_;

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

    // Writes into result_.code
    CodeBuilder builder_;
};

class ModuleCodegen final {
public:
    explicit ModuleCodegen(InternedString name, const NodePtr<Root>& root,
        SymbolTable& symbols, StringTable& strings, Diagnostics& diag);

    SymbolTable& symbols() { return symbols_; }
    StringTable& strings() { return strings_; }
    Diagnostics& diag() { return diag_; }

    void compile();

    std::unique_ptr<CompiledModule> take_result() { return std::move(result_); }

    // Adds a function to the module (at the end) and returns its index.
    // This is called from inside the compilation process.
    // TODO: Cleanup messy recursion
    u32 add_function();
    void set_function(u32 index, std::unique_ptr<FunctionDescriptor> func);

    u32 add_integer(i64 value);
    u32 add_float(f64 value);
    u32 add_string(InternedString str);
    u32 add_symbol(InternedString sym);
    u32 add_import(InternedString imp);

    // Returns the location of the given symbol (at module scope).
    // Results in a runtime error if the entry cannot be found.
    VarLocation get_location(const SymbolEntryPtr& entry) const;

    ModuleCodegen(const ModuleCodegen&) = delete;
    ModuleCodegen& operator=(const ModuleCodegen&) = delete;

private:
    template<typename T>
    using ConstantPool = std::unordered_map<T, u32, UseHasher>;

    template<typename T>
    u32 add_constant(ConstantPool<T>& consts, const T& value);

private:
    NodePtr<Root> root_;
    SymbolTable& symbols_;
    StringTable& strings_;
    Diagnostics& diag_;
    std::unique_ptr<CompiledModule> result_;

    // Maps reusable module items to their location in the compiled module.
    // If the same value is needed again, we reuse the existing value.
    ConstantPool<ModuleItem::Integer> const_integers_;
    ConstantPool<ModuleItem::Float> const_floats_;
    ConstantPool<ModuleItem::String> const_strings_;
    ConstantPool<ModuleItem::Symbol> const_symbols_;
    ConstantPool<ModuleItem::Import> const_imports_;

    // Maps module level declarations to their location.
    std::unordered_map<SymbolEntryPtr, VarLocation> entry_to_location_;
};

} // namespace hammer::compiler

#endif // HAMMER_COMPILER_CODEGEN_CODEGEN_HPP
