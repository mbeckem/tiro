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

TIRO_DEFINE_ID(BytecodeFunctionID, u32)

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
    BytecodeMemberID name() const { return name_; }
    void name(BytecodeMemberID value) { name_ = value; }

    BytecodeFunctionType type() const { return type_; }
    void type(BytecodeFunctionType t) { type_ = t; }

    u32 params() const { return params_; }
    void params(u32 count) { params_ = count; }

    u32 locals() const { return locals_; }
    void locals(u32 count) { locals_ = count; }

    std::vector<byte>& code() { return code_; }
    Span<const byte> code() const { return code_; }

private:
    BytecodeMemberID name_;
    BytecodeFunctionType type_ = BytecodeFunctionType::Normal;
    u32 params_ = 0;
    u32 locals_ = 0;
    std::vector<byte> code_;
};

void dump_function(const BytecodeFunction& func, FormatStream& stream);

/* [[[cog
    from codegen.unions import define_type
    from codegen.bytecode import BytecodeMemberType
    define_type(BytecodeMemberType)
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
    from codegen.unions import define_type
    from codegen.bytecode import BytecodeMember
    define_type(BytecodeMember)
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
        BytecodeMemberID name;

        explicit Symbol(const BytecodeMemberID& name_)
            : name(name_) {}
    };

    /// Represents an import.
    struct Import final {
        /// References a string constant.
        BytecodeMemberID module_name;

        explicit Import(const BytecodeMemberID& module_name_)
            : module_name(module_name_) {}
    };

    /// Represents a variable.
    struct Variable final {
        /// References a string constant.
        BytecodeMemberID name;

        /// References a constant. Can be invalid (meaning: initially null).
        BytecodeMemberID initial_value;

        Variable(const BytecodeMemberID& name_,
            const BytecodeMemberID& initial_value_)
            : name(name_)
            , initial_value(initial_value_) {}
    };

    /// Represents a function.
    struct Function final {
        /// References the compiled function.
        BytecodeFunctionID id;

        explicit Function(const BytecodeFunctionID& id_)
            : id(id_) {}
    };

    static BytecodeMember make_integer(const i64& value);
    static BytecodeMember make_float(const f64& value);
    static BytecodeMember make_string(const InternedString& value);
    static BytecodeMember make_symbol(const BytecodeMemberID& name);
    static BytecodeMember make_import(const BytecodeMemberID& module_name);
    static BytecodeMember make_variable(
        const BytecodeMemberID& name, const BytecodeMemberID& initial_value);
    static BytecodeMember make_function(const BytecodeFunctionID& id);

    BytecodeMember(const Integer& integer);
    BytecodeMember(const Float& f);
    BytecodeMember(const String& string);
    BytecodeMember(const Symbol& symbol);
    BytecodeMember(const Import& import);
    BytecodeMember(const Variable& variable);
    BytecodeMember(const Function& function);

    BytecodeMemberType type() const noexcept { return type_; }

    void format(FormatStream& stream) const;

    void build_hash(Hasher& h) const;

    const Integer& as_integer() const;
    const Float& as_float() const;
    const String& as_string() const;
    const Symbol& as_symbol() const;
    const Import& as_import() const;
    const Variable& as_variable() const;
    const Function& as_function() const;

    template<typename Visitor, typename... Args>
    TIRO_FORCE_INLINE decltype(auto) visit(Visitor&& vis, Args&&... args) {
        return visit_impl(
            *this, std::forward<Visitor>(vis), std::forward<Args>(args)...);
    }

    template<typename Visitor, typename... Args>
    TIRO_FORCE_INLINE decltype(auto)
    visit(Visitor&& vis, Args&&... args) const {
        return visit_impl(
            *this, std::forward<Visitor>(vis), std::forward<Args>(args)...);
    }

private:
    template<typename Self, typename Visitor, typename... Args>
    static TIRO_FORCE_INLINE decltype(auto)
    visit_impl(Self&& self, Visitor&& vis, Args&&... args);

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
    BytecodeModule& operator=(BytecodeModule&&) noexcept = default;

    StringTable& strings() { return strings_; }
    const StringTable& strings() const { return strings_; }

    InternedString name() const { return name_; }
    void name(InternedString n) { name_ = n; }

    /// Member id of the initialization function (invalid if there is none).
    BytecodeMemberID init() const { return init_; }
    void init(BytecodeMemberID init) { init_ = init; }

    auto member_ids() const { return members_.keys(); }
    auto function_ids() const { return functions_.keys(); }

    size_t member_count() const { return members_.size(); }
    size_t function_count() const { return functions_.size(); }

    BytecodeMemberID make(const BytecodeMember& member);
    BytecodeFunctionID make(BytecodeFunction&& fn);

    NotNull<IndexMapPtr<BytecodeMember>> operator[](BytecodeMemberID id);
    NotNull<IndexMapPtr<const BytecodeMember>>
    operator[](BytecodeMemberID id) const;

    NotNull<IndexMapPtr<BytecodeFunction>> operator[](BytecodeFunctionID id);
    NotNull<IndexMapPtr<const BytecodeFunction>>
    operator[](BytecodeFunctionID id) const;

private:
    StringTable strings_;
    InternedString name_;
    BytecodeMemberID init_;
    IndexMap<BytecodeMember, IDMapper<BytecodeMemberID>> members_;
    IndexMap<BytecodeFunction, IDMapper<BytecodeFunctionID>> functions_;
};

void dump_module(const BytecodeModule& module, FormatStream& stream);

/* [[[cog
    from codegen.unions import define_inlines
    from codegen.bytecode import BytecodeMember
    define_inlines(BytecodeMember)
]]] */
template<typename Self, typename Visitor, typename... Args>
decltype(auto)
BytecodeMember::visit_impl(Self&& self, Visitor&& vis, Args&&... args) {
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

TIRO_ENABLE_BUILD_HASH(tiro::BytecodeMember)

#endif // TIRO_BYTECODE_MODULE_HPP
