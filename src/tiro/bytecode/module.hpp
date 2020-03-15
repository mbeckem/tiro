#ifndef TIRO_BYTECODE_MODULE_HPP
#define TIRO_BYTECODE_MODULE_HPP

#include "tiro/bytecode/fwd.hpp"
#include "tiro/core/defs.hpp"
#include "tiro/core/format_stream.hpp"
#include "tiro/core/id_type.hpp"
#include "tiro/core/index_map.hpp"
#include "tiro/core/not_null.hpp"
#include "tiro/core/span.hpp"
#include "tiro/core/string_table.hpp"

#include <vector>

namespace tiro::compiler::bytecode {

TIRO_DEFINE_ID(ModuleMemberID, u32)
TIRO_DEFINE_ID(FunctionID, u32)

enum class FunctionType : u8 {
    Normal,  // Normal function
    Closure, // Function requires closure environment
};

std::string_view to_string(FunctionType type);

class Function final {
public:
    Function();
    ~Function();

    Function(Function&&) noexcept = default;
    Function& operator=(Function&&) noexcept = default;

    FunctionType type() const { return type_; }
    void type(FunctionType t) { type_ = t; }

    u32 params() const { return params_; }
    u32 params(u32 count) { params_ = count; }

    u32 locals() const { return locals_; }
    u32 locals(u32 count) { locals_ = count; }

    std::vector<byte>& code() { return code_; }
    Span<const byte> code() const { return code_; }

private:
    FunctionType type_;
    u32 params_ = 0;
    u32 locals_ = 0;
    std::vector<byte> code_;
};

/* [[[cog
    import unions
    import bytecode
    unions.define_type(bytecode.ModuleMemberType)
]]] */
/// Represents the type of a module member.
enum class ModuleMemberType : u8 {
    Integer,
    Float,
    String,
    Symbol,
    Import,
    Variable,
    Function,
};

std::string_view to_string(ModuleMemberType type);
// [[[end]]]

/* [[[cog
    import unions
    import bytecode
    unions.define_type(bytecode.ModuleMember)
]]] */
/// Represents a member of a compiled module.
class ModuleMember final {
public:
    /// Represents an integer constant.
    struct Integer final {
        i64 value;

        explicit Integer(const i64& value_)
            : value(value_) {}
    };

    /// Represents a floating point constant.
    struct Float final {
        f64 value;

        explicit Float(const f64& value_)
            : value(value_) {}
    };

    /// Represents a string constant.
    struct String final {
        InternedString value;

        explicit String(const InternedString& value_)
            : value(value_) {}
    };

    /// Represents a symbol constant.
    struct Symbol final {
        /// References a string constant.
        ModuleMemberID symbol_name;

        explicit Symbol(const ModuleMemberID& symbol_name_)
            : symbol_name(symbol_name_) {}
    };

    /// Represents an import.
    struct Import final {
        /// References a string constant.
        ModuleMemberID module_name;

        explicit Import(const ModuleMemberID& module_name_)
            : module_name(module_name_) {}
    };

    /// Represents a variable.
    struct Variable final {
        /// References a string constant.
        ModuleMemberID name;

        /// References a constant. Can be invalid (meaning: initially null).
        ModuleMemberID initial_value;

        Variable(
            const ModuleMemberID& name_, const ModuleMemberID& initial_value_)
            : name(name_)
            , initial_value(initial_value_) {}
    };

    /// Represents a function.
    struct Function final {
        /// References the compiled function.
        FunctionID value;

        explicit Function(const FunctionID& value_)
            : value(value_) {}
    };

    static ModuleMember make_integer(const i64& value);
    static ModuleMember make_float(const f64& value);
    static ModuleMember make_string(const InternedString& value);
    static ModuleMember make_symbol(const ModuleMemberID& symbol_name);
    static ModuleMember make_import(const ModuleMemberID& module_name);
    static ModuleMember make_variable(
        const ModuleMemberID& name, const ModuleMemberID& initial_value);
    static ModuleMember make_function(const FunctionID& value);

    ModuleMember(const Integer& integer);
    ModuleMember(const Float& f);
    ModuleMember(const String& string);
    ModuleMember(const Symbol& symbol);
    ModuleMember(const Import& import);
    ModuleMember(const Variable& variable);
    ModuleMember(const Function& function);

    ModuleMemberType type() const noexcept { return type_; }

    void format(FormatStream& stream) const;

    const Integer& as_integer() const;
    const Float& as_float() const;
    const String& as_string() const;
    const Symbol& as_symbol() const;
    const Import& as_import() const;
    const Variable& as_variable() const;
    const Function& as_function() const;

    template<typename Visitor>
    TIRO_FORCE_INLINE decltype(auto) visit(Visitor&& vis) const {
        return visit_impl(*this, std::forward<Visitor>(vis));
    }

private:
    template<typename Self, typename Visitor>
    static TIRO_FORCE_INLINE decltype(auto)
    visit_impl(Self&& self, Visitor&& vis);

private:
    ModuleMemberType type_;
    union {
        Integer integer_;
        Float float_;
        String string_;
        Symbol symbol_;
        Import import_;
        Variable variable_;
        Function function_;
    };
};

bool operator==(const ModuleMember& lhs, const ModuleMember& rhs);
bool operator!=(const ModuleMember& lhs, const ModuleMember& rhs);
// [[[end]]]

/// Represents a compiled bytecode module.
/// Modules can be loaded into the vm for execution.
class Module final {
public:
    Module();
    ~Module();

    Module(Module&&) noexcept = default;
    Module& operator=(Module&&) noexcept = default;

    InternedString name() const { return name_; }
    void name(InternedString n) { name_ = n; }

    ModuleMemberID make(const ModuleMember& member);
    FunctionID make(Function&& fn);

    NotNull<IndexMapPtr<ModuleMember>> operator[](ModuleMemberID id);
    NotNull<IndexMapPtr<const ModuleMember>>
    operator[](ModuleMemberID id) const;

    NotNull<IndexMapPtr<Function>> operator[](FunctionID id);
    NotNull<IndexMapPtr<const Function>> operator[](FunctionID id) const;

private:
    InternedString name_;
    IndexMap<ModuleMember, IDMapper<ModuleMemberID>> members_;
    IndexMap<Function, IDMapper<FunctionID>> functions_;
};

/* [[[cog
    import unions
    import bytecode
    unions.define_inlines(bytecode.ModuleMember)
]]] */
template<typename Self, typename Visitor>
decltype(auto) ModuleMember::visit_impl(Self&& self, Visitor&& vis) {
    switch (self.type()) {
    case ModuleMemberType::Integer:
        return vis.visit_integer(self.integer_);
    case ModuleMemberType::Float:
        return vis.visit_float(self.float_);
    case ModuleMemberType::String:
        return vis.visit_string(self.string_);
    case ModuleMemberType::Symbol:
        return vis.visit_symbol(self.symbol_);
    case ModuleMemberType::Import:
        return vis.visit_import(self.import_);
    case ModuleMemberType::Variable:
        return vis.visit_variable(self.variable_);
    case ModuleMemberType::Function:
        return vis.visit_function(self.function_);
    }
    TIRO_UNREACHABLE("Invalid ModuleMember type.");
}
// [[[end]]]

} // namespace tiro::compiler::bytecode

TIRO_ENABLE_FREE_TO_STRING(tiro::compiler::bytecode::FunctionType)
TIRO_ENABLE_FREE_TO_STRING(tiro::compiler::bytecode::ModuleMemberType)
TIRO_ENABLE_MEMBER_FORMAT(tiro::compiler::bytecode::ModuleMember)

#endif // TIRO_CODEGEN_2_FWD_HPP
