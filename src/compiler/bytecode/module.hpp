#ifndef TIRO_COMPILER_BYTECODE_MODULE_HPP
#define TIRO_COMPILER_BYTECODE_MODULE_HPP

#include "common/defs.hpp"
#include "common/format.hpp"
#include "common/hash.hpp"
#include "common/id_type.hpp"
#include "common/index_map.hpp"
#include "common/not_null.hpp"
#include "common/span.hpp"
#include "common/string_table.hpp"
#include "compiler/bytecode/fwd.hpp"
#include "compiler/bytecode/instruction.hpp"

#include <vector>

namespace tiro {

TIRO_DEFINE_ID(BytecodeFunctionId, u32)

enum class BytecodeFunctionType : u8 {
    Normal,  // Normal function
    Closure, // Function requires closure environment
};

std::string_view to_string(BytecodeFunctionType type);

class BytecodeFunction final {
public:
    BytecodeFunction();
    ~BytecodeFunction();

    BytecodeFunction(BytecodeFunction&&) noexcept = default;
    BytecodeFunction& operator=(BytecodeFunction&&) noexcept = default;

    // Name can be invalid for anonymous entries.
    BytecodeMemberId name() const { return name_; }
    void name(BytecodeMemberId value) { name_ = value; }

    BytecodeFunctionType type() const { return type_; }
    void type(BytecodeFunctionType t) { type_ = t; }

    u32 params() const { return params_; }
    void params(u32 count) { params_ = count; }

    u32 locals() const { return locals_; }
    void locals(u32 count) { locals_ = count; }

    std::vector<byte>& code() { return code_; }
    Span<const byte> code() const { return code_; }

private:
    BytecodeMemberId name_;
    BytecodeFunctionType type_ = BytecodeFunctionType::Normal;
    u32 params_ = 0;
    u32 locals_ = 0;
    std::vector<byte> code_;
};

void dump_function(const BytecodeFunction& func, FormatStream& stream);

/* [[[cog
    from codegen.unions import define
    from codegen.bytecode import BytecodeMemberType
    define(BytecodeMemberType)
]]] */
/// Represents the type of a module member.
enum class BytecodeMemberType : u8 {
    Integer,
    Float,
    String,
    Symbol,
    Import,
    Variable,
    Function,
};

std::string_view to_string(BytecodeMemberType type);
// [[[end]]]

/* [[[cog
    from codegen.unions import define
    from codegen.bytecode import BytecodeMember
    define(BytecodeMember)
]]] */
/// Represents a member of a compiled module.
class BytecodeMember final {
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
        BytecodeMemberId name;

        explicit Symbol(const BytecodeMemberId& name_)
            : name(name_) {}
    };

    /// Represents an import.
    struct Import final {
        /// References a string constant.
        BytecodeMemberId module_name;

        explicit Import(const BytecodeMemberId& module_name_)
            : module_name(module_name_) {}
    };

    /// Represents a variable.
    struct Variable final {
        /// References a string constant.
        BytecodeMemberId name;

        /// References a constant. Can be invalid (meaning: initially null).
        BytecodeMemberId initial_value;

        Variable(const BytecodeMemberId& name_, const BytecodeMemberId& initial_value_)
            : name(name_)
            , initial_value(initial_value_) {}
    };

    /// Represents a function.
    struct Function final {
        /// References the compiled function.
        BytecodeFunctionId id;

        explicit Function(const BytecodeFunctionId& id_)
            : id(id_) {}
    };

    static BytecodeMember make_integer(const i64& value);
    static BytecodeMember make_float(const f64& value);
    static BytecodeMember make_string(const InternedString& value);
    static BytecodeMember make_symbol(const BytecodeMemberId& name);
    static BytecodeMember make_import(const BytecodeMemberId& module_name);
    static BytecodeMember
    make_variable(const BytecodeMemberId& name, const BytecodeMemberId& initial_value);
    static BytecodeMember make_function(const BytecodeFunctionId& id);

    BytecodeMember(Integer integer);
    BytecodeMember(Float f);
    BytecodeMember(String string);
    BytecodeMember(Symbol symbol);
    BytecodeMember(Import import);
    BytecodeMember(Variable variable);
    BytecodeMember(Function function);

    BytecodeMemberType type() const noexcept { return type_; }

    void format(FormatStream& stream) const;

    void hash(Hasher& h) const;

    const Integer& as_integer() const;
    const Float& as_float() const;
    const String& as_string() const;
    const Symbol& as_symbol() const;
    const Import& as_import() const;
    const Variable& as_variable() const;
    const Function& as_function() const;

    template<typename Visitor, typename... Args>
    TIRO_FORCE_INLINE decltype(auto) visit(Visitor&& vis, Args&&... args) {
        return visit_impl(*this, std::forward<Visitor>(vis), std::forward<Args>(args)...);
    }

    template<typename Visitor, typename... Args>
    TIRO_FORCE_INLINE decltype(auto) visit(Visitor&& vis, Args&&... args) const {
        return visit_impl(*this, std::forward<Visitor>(vis), std::forward<Args>(args)...);
    }

private:
    template<typename Self, typename Visitor, typename... Args>
    static TIRO_FORCE_INLINE decltype(auto) visit_impl(Self&& self, Visitor&& vis, Args&&... args);

private:
    BytecodeMemberType type_;
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

bool operator==(const BytecodeMember& lhs, const BytecodeMember& rhs);
bool operator!=(const BytecodeMember& lhs, const BytecodeMember& rhs);
// [[[end]]]

/// Represents a compiled bytecode module.
/// Modules can be loaded into the vm for execution.
class BytecodeModule final {
public:
    BytecodeModule();
    ~BytecodeModule();

    BytecodeModule(BytecodeModule&&) noexcept = default;

    StringTable& strings() { return strings_; }
    const StringTable& strings() const { return strings_; }

    InternedString name() const { return name_; }
    void name(InternedString n) { name_ = n; }

    /// Member id of the initialization function (invalid if there is none).
    BytecodeMemberId init() const { return init_; }
    void init(BytecodeMemberId init) { init_ = init; }

    /// Add an entry to the export set of this module. A value can be exported
    /// by giving it a (unique) name. The left hand side must always point to a symbol,
    /// the right hand side may be any (constant) value.
    void add_export(BytecodeMemberId symbol_id, BytecodeMemberId value_id);

    /// Iterate over the exported (symbol, value)-pairs.
    auto exports() const { return IterRange(exports_.begin(), exports_.end()); }

    /// Iterate over the member ids in this module.
    auto member_ids() const { return members_.keys(); }

    /// Iterate over the function ids in this module.
    auto function_ids() const { return functions_.keys(); }

    size_t member_count() const { return members_.size(); }
    size_t function_count() const { return functions_.size(); }

    BytecodeMemberId make(const BytecodeMember& member);
    BytecodeFunctionId make(BytecodeFunction&& fn);

    NotNull<IndexMapPtr<BytecodeMember>> operator[](BytecodeMemberId id);
    NotNull<IndexMapPtr<const BytecodeMember>> operator[](BytecodeMemberId id) const;

    NotNull<IndexMapPtr<BytecodeFunction>> operator[](BytecodeFunctionId id);
    NotNull<IndexMapPtr<const BytecodeFunction>> operator[](BytecodeFunctionId id) const;

private:
    StringTable strings_;
    InternedString name_;
    BytecodeMemberId init_;
    std::vector<std::tuple<BytecodeMemberId, BytecodeMemberId>> exports_; // symbol -> value
    IndexMap<BytecodeMember, IdMapper<BytecodeMemberId>> members_;
    IndexMap<BytecodeFunction, IdMapper<BytecodeFunctionId>> functions_;
};

void dump_module(const BytecodeModule& module, FormatStream& stream);

/* [[[cog
    from codegen.unions import implement_inlines
    from codegen.bytecode import BytecodeMember
    implement_inlines(BytecodeMember)
]]] */
template<typename Self, typename Visitor, typename... Args>
decltype(auto) BytecodeMember::visit_impl(Self&& self, Visitor&& vis, Args&&... args) {
    switch (self.type()) {
    case BytecodeMemberType::Integer:
        return vis.visit_integer(self.integer_, std::forward<Args>(args)...);
    case BytecodeMemberType::Float:
        return vis.visit_float(self.float_, std::forward<Args>(args)...);
    case BytecodeMemberType::String:
        return vis.visit_string(self.string_, std::forward<Args>(args)...);
    case BytecodeMemberType::Symbol:
        return vis.visit_symbol(self.symbol_, std::forward<Args>(args)...);
    case BytecodeMemberType::Import:
        return vis.visit_import(self.import_, std::forward<Args>(args)...);
    case BytecodeMemberType::Variable:
        return vis.visit_variable(self.variable_, std::forward<Args>(args)...);
    case BytecodeMemberType::Function:
        return vis.visit_function(self.function_, std::forward<Args>(args)...);
    }
    TIRO_UNREACHABLE("Invalid BytecodeMember type.");
}
// [[[end]]]

} // namespace tiro

TIRO_ENABLE_FREE_TO_STRING(tiro::BytecodeFunctionType)
TIRO_ENABLE_FREE_TO_STRING(tiro::BytecodeMemberType)
TIRO_ENABLE_MEMBER_FORMAT(tiro::BytecodeMember)

TIRO_ENABLE_MEMBER_HASH(tiro::BytecodeMember)

#endif // TIRO_COMPILER_BYTECODE_MODULE_HPP
