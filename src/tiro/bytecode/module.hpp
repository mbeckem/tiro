#ifndef TIRO_BYTECODE_MODULE_HPP
#define TIRO_BYTECODE_MODULE_HPP

#include "tiro/bytecode/fwd.hpp"
#include "tiro/bytecode/instruction.hpp"
#include "tiro/core/defs.hpp"
#include "tiro/core/format.hpp"
#include "tiro/core/hash.hpp"
#include "tiro/core/id_type.hpp"
#include "tiro/core/index_map.hpp"
#include "tiro/core/not_null.hpp"
#include "tiro/core/span.hpp"
#include "tiro/core/string_table.hpp"
#include <vector>

namespace tiro {

TIRO_DEFINE_ID(CompiledFunctionID, u32)

enum class CompiledFunctionType : u8 {
    Normal,  // Normal function
    Closure, // Function requires closure environment
};

std::string_view to_string(CompiledFunctionType type);

class CompiledFunction final {
public:
    CompiledFunction();
    ~CompiledFunction();

    CompiledFunction(CompiledFunction&&) noexcept = default;
    CompiledFunction& operator=(CompiledFunction&&) noexcept = default;

    // Name can be invalid for anonymous entries.
    InternedString name() const { return name_; }
    void name(InternedString value) { name_ = value; }

    CompiledFunctionType type() const { return type_; }
    void type(CompiledFunctionType t) { type_ = t; }

    u32 params() const { return params_; }
    void params(u32 count) { params_ = count; }

    u32 locals() const { return locals_; }
    void locals(u32 count) { locals_ = count; }

    std::vector<byte>& code() { return code_; }
    Span<const byte> code() const { return code_; }

private:
    InternedString name_;
    CompiledFunctionType type_ = CompiledFunctionType::Normal;
    u32 params_ = 0;
    u32 locals_ = 0;
    std::vector<byte> code_;
};

void dump_function(const CompiledFunction& func, const StringTable& strings,
    FormatStream& stream);

/* [[[cog
    import unions
    import bytecode
    unions.define_type(bytecode.CompiledModuleMemberType)
]]] */
/// Represents the type of a module member.
enum class CompiledModuleMemberType : u8 {
    Integer,
    Float,
    String,
    Symbol,
    Import,
    Variable,
    Function,
};

std::string_view to_string(CompiledModuleMemberType type);
// [[[end]]]

/* [[[cog
    import unions
    import bytecode
    unions.define_type(bytecode.CompiledModuleMember)
]]] */
/// Represents a member of a compiled module.
class CompiledModuleMember final {
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
        CompiledModuleMemberID name;

        explicit Symbol(const CompiledModuleMemberID& name_)
            : name(name_) {}
    };

    /// Represents an import.
    struct Import final {
        /// References a string constant.
        CompiledModuleMemberID module_name;

        explicit Import(const CompiledModuleMemberID& module_name_)
            : module_name(module_name_) {}
    };

    /// Represents a variable.
    struct Variable final {
        /// References a string constant.
        CompiledModuleMemberID name;

        /// References a constant. Can be invalid (meaning: initially null).
        CompiledModuleMemberID initial_value;

        Variable(const CompiledModuleMemberID& name_,
            const CompiledModuleMemberID& initial_value_)
            : name(name_)
            , initial_value(initial_value_) {}
    };

    /// Represents a function.
    struct Function final {
        /// References the compiled function.
        CompiledFunctionID id;

        explicit Function(const CompiledFunctionID& id_)
            : id(id_) {}
    };

    static CompiledModuleMember make_integer(const i64& value);
    static CompiledModuleMember make_float(const f64& value);
    static CompiledModuleMember make_string(const InternedString& value);
    static CompiledModuleMember make_symbol(const CompiledModuleMemberID& name);
    static CompiledModuleMember
    make_import(const CompiledModuleMemberID& module_name);
    static CompiledModuleMember
    make_variable(const CompiledModuleMemberID& name,
        const CompiledModuleMemberID& initial_value);
    static CompiledModuleMember make_function(const CompiledFunctionID& id);

    CompiledModuleMember(const Integer& integer);
    CompiledModuleMember(const Float& f);
    CompiledModuleMember(const String& string);
    CompiledModuleMember(const Symbol& symbol);
    CompiledModuleMember(const Import& import);
    CompiledModuleMember(const Variable& variable);
    CompiledModuleMember(const Function& function);

    CompiledModuleMemberType type() const noexcept { return type_; }

    void format(FormatStream& stream) const;

    void build_hash(Hasher& h) const;

    const Integer& as_integer() const;
    const Float& as_float() const;
    const String& as_string() const;
    const Symbol& as_symbol() const;
    const Import& as_import() const;
    const Variable& as_variable() const;
    const Function& as_function() const;

    template<typename Visitor>
    TIRO_FORCE_INLINE decltype(auto) visit(Visitor&& vis) {
        return visit_impl(*this, std::forward<Visitor>(vis));
    }

    template<typename Visitor>
    TIRO_FORCE_INLINE decltype(auto) visit(Visitor&& vis) const {
        return visit_impl(*this, std::forward<Visitor>(vis));
    }

private:
    template<typename Self, typename Visitor>
    static TIRO_FORCE_INLINE decltype(auto)
    visit_impl(Self&& self, Visitor&& vis);

private:
    CompiledModuleMemberType type_;
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

bool operator==(
    const CompiledModuleMember& lhs, const CompiledModuleMember& rhs);
bool operator!=(
    const CompiledModuleMember& lhs, const CompiledModuleMember& rhs);
// [[[end]]]

/// Represents a compiled bytecode module.
/// Modules can be loaded into the vm for execution.
class CompiledModule final {
public:
    explicit CompiledModule(StringTable& strings);
    ~CompiledModule();

    CompiledModule(CompiledModule&&) noexcept = default;
    CompiledModule& operator=(CompiledModule&&) noexcept = default;

    StringTable& strings() const { return *strings_; }

    InternedString name() const { return name_; }
    void name(InternedString n) { name_ = n; }

    auto member_ids() const { return members_.keys(); }
    auto function_ids() const { return functions_.keys(); }

    size_t member_count() const { return members_.size(); }
    size_t function_count() const { return functions_.size(); }

    CompiledModuleMemberID make(const CompiledModuleMember& member);
    CompiledFunctionID make(CompiledFunction&& fn);

    NotNull<IndexMapPtr<CompiledModuleMember>>
    operator[](CompiledModuleMemberID id);
    NotNull<IndexMapPtr<const CompiledModuleMember>>
    operator[](CompiledModuleMemberID id) const;

    NotNull<IndexMapPtr<CompiledFunction>> operator[](CompiledFunctionID id);
    NotNull<IndexMapPtr<const CompiledFunction>>
    operator[](CompiledFunctionID id) const;

private:
    NotNull<StringTable*> strings_;
    InternedString name_;
    IndexMap<CompiledModuleMember, IDMapper<CompiledModuleMemberID>> members_;
    IndexMap<CompiledFunction, IDMapper<CompiledFunctionID>> functions_;
};

void dump_module(const CompiledModule& module, FormatStream& stream);

/* [[[cog
    import unions
    import bytecode
    unions.define_inlines(bytecode.CompiledModuleMember)
]]] */
template<typename Self, typename Visitor>
decltype(auto) CompiledModuleMember::visit_impl(Self&& self, Visitor&& vis) {
    switch (self.type()) {
    case CompiledModuleMemberType::Integer:
        return vis.visit_integer(self.integer_);
    case CompiledModuleMemberType::Float:
        return vis.visit_float(self.float_);
    case CompiledModuleMemberType::String:
        return vis.visit_string(self.string_);
    case CompiledModuleMemberType::Symbol:
        return vis.visit_symbol(self.symbol_);
    case CompiledModuleMemberType::Import:
        return vis.visit_import(self.import_);
    case CompiledModuleMemberType::Variable:
        return vis.visit_variable(self.variable_);
    case CompiledModuleMemberType::Function:
        return vis.visit_function(self.function_);
    }
    TIRO_UNREACHABLE("Invalid CompiledModuleMember type.");
}
// [[[end]]]

} // namespace tiro

TIRO_ENABLE_FREE_TO_STRING(tiro::CompiledFunctionType)
TIRO_ENABLE_FREE_TO_STRING(tiro::CompiledModuleMemberType)
TIRO_ENABLE_MEMBER_FORMAT(tiro::CompiledModuleMember)

TIRO_ENABLE_BUILD_HASH(tiro::CompiledModuleMember)

#endif // TIRO_BYTECODE_MODULE_HPP
