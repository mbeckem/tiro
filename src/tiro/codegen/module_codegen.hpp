#ifndef TIRO_CODEGEN_MODULE_CODEGEN_HPP
#define TIRO_CODEGEN_MODULE_CODEGEN_HPP

#include "tiro/codegen/variable_locations.hpp"
#include "tiro/compiler/output.hpp"
#include "tiro/core/defs.hpp"
#include "tiro/core/not_null.hpp"
#include "tiro/syntax/ast.hpp"

#include <unordered_set>

namespace tiro::compiler {

class ModuleCodegen final {
public:
    explicit ModuleCodegen(InternedString name, NotNull<Root*> root,
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

    void
    set_function(u32 index, NotNull<std::unique_ptr<FunctionDescriptor>> func);

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

} // namespace tiro::compiler

#endif // TIRO_CODEGEN_MODULE_CODEGEN_HPP
