#ifndef TIRO_BYTECODE_MODULE_HPP
#define TIRO_BYTECODE_MODULE_HPP

#include "bytecode/entities.hpp"
#include "bytecode/function.hpp"
#include "bytecode/fwd.hpp"
#include "bytecode/instruction.hpp"
#include "common/adt/index_map.hpp"
#include "common/adt/not_null.hpp"
#include "common/defs.hpp"
#include "common/format.hpp"
#include "common/hash.hpp"
#include "common/text/string_table.hpp"

#include <vector>

namespace tiro {

/// Represents a record template. Record templates are used to construct records
/// with a statically determined set of keys.
/// The keys referenced by a bytecode record template must be symbol constants.
class BytecodeRecordTemplate final {
public:
    BytecodeRecordTemplate();
    ~BytecodeRecordTemplate();

    BytecodeRecordTemplate(BytecodeRecordTemplate&&) noexcept;
    BytecodeRecordTemplate& operator=(BytecodeRecordTemplate&&) noexcept;

    std::vector<BytecodeMemberId>& keys() { return keys_; }
    const std::vector<BytecodeMemberId>& keys() const { return keys_; }

private:
    std::vector<BytecodeMemberId> keys_;
};

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
    RecordTemplate,
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

    /// Represents a record template.
    struct RecordTemplate final {
        /// References the compiled record template.
        BytecodeRecordTemplateId id;

        explicit RecordTemplate(const BytecodeRecordTemplateId& id_)
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
    static BytecodeMember make_record_template(const BytecodeRecordTemplateId& id);

    BytecodeMember(Integer integer);
    BytecodeMember(Float f);
    BytecodeMember(String string);
    BytecodeMember(Symbol symbol);
    BytecodeMember(Import import);
    BytecodeMember(Variable variable);
    BytecodeMember(Function function);
    BytecodeMember(RecordTemplate record_template);

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
    const RecordTemplate& as_record_template() const;

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
        RecordTemplate record_template_;
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

    /// Iterate over the record template ids in this module.
    auto record_ids() const { return records_.keys(); }

    size_t member_count() const { return members_.size(); }
    size_t function_count() const { return functions_.size(); }
    size_t record_count() const { return records_.size(); }

    BytecodeMemberId make(const BytecodeMember& member);
    BytecodeFunctionId make(BytecodeFunction&& fn);
    BytecodeRecordTemplateId make(BytecodeRecordTemplate&& tmpl);

    NotNull<IndexMapPtr<BytecodeMember>> operator[](BytecodeMemberId id);
    NotNull<IndexMapPtr<const BytecodeMember>> operator[](BytecodeMemberId id) const;

    NotNull<IndexMapPtr<BytecodeFunction>> operator[](BytecodeFunctionId id);
    NotNull<IndexMapPtr<const BytecodeFunction>> operator[](BytecodeFunctionId id) const;

    NotNull<IndexMapPtr<BytecodeRecordTemplate>> operator[](BytecodeRecordTemplateId id);
    NotNull<IndexMapPtr<const BytecodeRecordTemplate>>
    operator[](BytecodeRecordTemplateId id) const;

private:
    StringTable strings_;
    InternedString name_;
    BytecodeMemberId init_;
    std::vector<std::tuple<BytecodeMemberId, BytecodeMemberId>> exports_; // symbol -> value
    IndexMap<BytecodeMember, IdMapper<BytecodeMemberId>> members_;
    IndexMap<BytecodeFunction, IdMapper<BytecodeFunctionId>> functions_;
    IndexMap<BytecodeRecordTemplate, IdMapper<BytecodeRecordTemplateId>> records_;
};

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
    case BytecodeMemberType::RecordTemplate:
        return vis.visit_record_template(self.record_template_, std::forward<Args>(args)...);
    }
    TIRO_UNREACHABLE("Invalid BytecodeMember type.");
}
// [[[end]]]

} // namespace tiro

TIRO_ENABLE_FREE_TO_STRING(tiro::BytecodeMemberType)
TIRO_ENABLE_MEMBER_FORMAT(tiro::BytecodeMember)

TIRO_ENABLE_MEMBER_HASH(tiro::BytecodeMember)

#endif // TIRO_BYTECODE_MODULE_HPP
