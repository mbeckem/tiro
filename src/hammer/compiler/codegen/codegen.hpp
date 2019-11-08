#ifndef HAMMER_COMPILER_CODEGEN_CODEGEN_HPP
#define HAMMER_COMPILER_CODEGEN_CODEGEN_HPP

#include "hammer/ast/node.hpp"
#include "hammer/compiler/code_builder.hpp"
#include "hammer/compiler/codegen/variable_locations.hpp"
#include "hammer/compiler/diagnostics.hpp"
#include "hammer/compiler/output.hpp"
#include "hammer/compiler/string_table.hpp"

#include <unordered_set>

namespace hammer {

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
    explicit FunctionCodegen(ast::FuncDecl& func, ModuleCodegen& module,
        u32 index_in_module, StringTable& strings, Diagnostics& diag);

    explicit FunctionCodegen(ast::FuncDecl& decl, FunctionCodegen& parent);

private:
    explicit FunctionCodegen(ast::FuncDecl& func, FunctionCodegen* parent,
        ModuleCodegen& module, u32 index_in_module, StringTable& strings,
        Diagnostics& diag);

public:
    void compile();

    ModuleCodegen& module() { return module_; }
    u32 index_in_module() const { return index_in_module_; }
    Diagnostics& diag() { return diag_; }
    StringTable& strings() { return strings_; }
    CodeBuilder& builder() { return builder_; }

    FunctionCodegen(const FunctionCodegen&) = delete;
    FunctionCodegen& operator=(const FunctionCodegen&) = delete;

public:
    /// Generates bytecode for the given expr.
    void generate_expr(ast::Expr* expr);

    /// Same as generate_expr(), but contains a debug assertion that checks
    /// that the given expression can in fact be used in a value context.
    /// Errors conditions like these are caught in the analyzer, but are checked
    /// again in here (in development builds) for extra safety.
    void generate_expr_value(ast::Expr* expr);

    /// Generates bytecode for a statement.
    void generate_stmt(ast::Stmt* stmt);

    /// Generates bytecode to load the given declaration.
    void generate_load(ast::Decl* decl);

    /// Generates bytecode to store the given `rhs` expression in the given location.
    /// If `push_value` is true, then the value of `expr` will also be pushed onto the stack.
    void generate_store(ast::Decl* decl, ast::Expr* rhs, bool push_value);

    /// Generates code to create a closure from the given nested function decl.
    void generate_closure(ast::FuncDecl* decl);

    /// Call this function to emit the bytecode for a loop body.
    /// Loop bodies must be handled by this function because they may open their own closure context.
    void generate_loop_body(
        LabelID break_label, LabelID continue_label, ast::Expr* body);

private:
    void compile_function(ast::FuncDecl* func);
    void compile_function_body(ast::BlockExpr* body);

public:
    // Attempts to reach the context `dst` from the `start` context.
    // Returns the number of levels to that context (i.e. 0 if dst == `start` etc.).
    // It is an error if the context cannot be reached.
    u32 get_context_level(ClosureContext* start, ClosureContext* dst);

    // Returns the location of the declaration. Errors if no matching
    // location entry was found.
    VarLocation get_location(ast::Decl* decl);

    // Returns the local index for the given context, if the context is local to this function.
    std::optional<u32> local_context(ClosureContext* context);

    // Load the given context. Only works for the outer context (passed in by the parent function)
    // or local context objects.
    void load_context(ClosureContext* context);

    // Loads the current context.
    void load_context();

    void push_loop(LoopContext* loop);
    void pop_loop();
    LoopContext* current_loop() { return current_loop_; }

    void push_closure(ClosureContext* context);
    void pop_closure();

private:
    // Returns the closure context started by this node, or null.
    ClosureContext* get_closure_context(ast::Node* node) {
        return locations_.get_closure_context(node);
    }

private:
    // The function we're compiling.
    ast::FuncDecl& func_;

    // The function codgen object for the surrounding function, if any.
    // Important for closures.
    FunctionCodegen* parent_ = nullptr;

    ModuleCodegen& module_;
    u32 index_in_module_ = 0;

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
    ModuleCodegen(ast::File& file, StringTable& strings, Diagnostics& diag);

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

    VarLocation get_location(ast::Decl* decl) const;

    ModuleCodegen(const ModuleCodegen&) = delete;
    ModuleCodegen& operator=(const ModuleCodegen&) = delete;

private:
    template<typename T>
    using ConstantPool = std::unordered_map<T, u32, UseHasher>;

    template<typename T>
    u32 add_constant(ConstantPool<T>& consts, const T& value);

private:
    ast::File& file_;
    StringTable& strings_;
    Diagnostics& diag_;
    std::unique_ptr<CompiledModule> result_;

    ConstantPool<ModuleItem::Integer> const_integers_;
    ConstantPool<ModuleItem::Float> const_floats_;
    ConstantPool<ModuleItem::String> const_strings_;
    ConstantPool<ModuleItem::Symbol> const_symbols_;
    ConstantPool<ModuleItem::Import> const_imports_;

    // Maps module level declarations to their location.
    std::unordered_map<ast::Decl*, VarLocation> decl_to_location_;
};

// TODO move somewhere useful
inline u32 as_u32(size_t n) {
    HAMMER_CHECK(
        n <= std::numeric_limits<u32>::max(), "Size is out of range: {}.", n);
    return static_cast<u32>(n);
}

// See above
inline u32 next_u32(u32& counter, const char* msg) {
    HAMMER_CHECK(counter != std::numeric_limits<u32>::max(),
        "Counter overflow: {}.", msg);
    return counter++;
}

} // namespace hammer

#endif // HAMMER_COMPILER_CODEGEN_CODEGEN_HPP
