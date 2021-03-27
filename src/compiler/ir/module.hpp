#ifndef TIRO_COMPILER_IR_MODULE_HPP
#define TIRO_COMPILER_IR_MODULE_HPP

#include "common/defs.hpp"
#include "common/entities/entity_storage.hpp"
#include "common/entities/entity_storage_accessors.hpp"
#include "common/text/string_table.hpp"
#include "compiler/ir/entities.hpp"
#include "compiler/ir/fwd.hpp"

namespace tiro::ir {

/// Represents a module that has been lowered to IR.
class Module final {
public:
    explicit Module(InternedString name, StringTable& strings);
    ~Module();

    Module(Module&&) noexcept = default;
    Module& operator=(Module&&) noexcept = default;

    Module(const Module&) = delete;
    Module& operator=(const Module&) = delete;

    StringTable& strings() const { return *strings_; }

    InternedString name() const { return name_; }

    // The initializer function. May be invalid if none is needed.
    ModuleMemberId init() const { return init_; }
    void init(ModuleMemberId init) { init_ = init; }

    ModuleMemberId make(const ModuleMember& member);
    FunctionId make(Function&& function);

    TIRO_ENTITY_STORAGE_ACCESSORS(ModuleMember, ModuleMemberId, members_)
    TIRO_ENTITY_STORAGE_ACCESSORS(Function, FunctionId, functions_)

    auto member_ids() const { return members_.keys(); }
    auto function_ids() const { return functions_.keys(); }

    auto members() const { return range_view(members_); }
    auto functions() const { return range_view(functions_); }

    size_t member_count() const { return members_.size(); }
    size_t function_count() const { return functions_.size(); }

private:
    NotNull<StringTable*> strings_;

    InternedString name_;
    ModuleMemberId init_;
    EntityStorage<ModuleMember, ModuleMemberId> members_;
    EntityStorage<Function, FunctionId> functions_;
};

void dump_module(const Module& module, FormatStream& stream);

/* [[[cog
    from codegen.unions import define
    from codegen.ir import ModuleMemberData
    define(ModuleMemberData.tag)
]]] */
enum class ModuleMemberType : u8 {
    Import,
    Variable,
    Function,
};

std::string_view to_string(ModuleMemberType type);
// [[[end]]]

/* [[[cog
    from codegen.unions import define
    from codegen.ir import ModuleMemberData
    define(ModuleMemberData)
]]] */
class ModuleMemberData final {
public:
    /// Represents an import of another module.
    struct Import final {
        /// The name of the imported module.
        InternedString name;

        explicit Import(const InternedString& name_)
            : name(name_) {}
    };

    /// Represents a variable at module scope.
    struct Variable final {
        /// The name of the variable.
        InternedString name;

        explicit Variable(const InternedString& name_)
            : name(name_) {}
    };

    /// Represents a function of this module, in IR form.
    struct Function final {
        /// The Id of the function within this module.
        FunctionId id;

        explicit Function(const FunctionId& id_)
            : id(id_) {}
    };

    static ModuleMemberData make_import(const InternedString& name);
    static ModuleMemberData make_variable(const InternedString& name);
    static ModuleMemberData make_function(const FunctionId& id);

    ModuleMemberData(Import import);
    ModuleMemberData(Variable variable);
    ModuleMemberData(Function function);

    ModuleMemberType type() const noexcept { return type_; }

    void format(FormatStream& stream) const;

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
    ModuleMemberType type_;
    union {
        Import import_;
        Variable variable_;
        Function function_;
    };
};
// [[[end]]]

/// Represents a member of a module."
class ModuleMember final {
public:
    ModuleMember(const ModuleMemberData& data);

    /// True if the module member is being exported from its module.
    void exported(bool is_exported) { exported_ = is_exported; }
    bool exported() const { return exported_; }

    /// Returns the type of this module member.
    ModuleMemberType type() const { return data_.type(); }

    /// Getter and setter for the concrete module member data.
    void data(const ModuleMemberData& data) { data_ = data; }
    const ModuleMemberData& data() const { return data_; }

private:
    ModuleMemberData data_;
    bool exported_ = false; // TODO: Flags?
};

/* [[[cog
    from codegen.unions import implement_inlines
    from codegen.ir import ModuleMemberData
    implement_inlines(ModuleMemberData)
]]] */
template<typename Self, typename Visitor, typename... Args>
decltype(auto) ModuleMemberData::visit_impl(Self&& self, Visitor&& vis, Args&&... args) {
    switch (self.type()) {
    case ModuleMemberType::Import:
        return vis.visit_import(self.import_, std::forward<Args>(args)...);
    case ModuleMemberType::Variable:
        return vis.visit_variable(self.variable_, std::forward<Args>(args)...);
    case ModuleMemberType::Function:
        return vis.visit_function(self.function_, std::forward<Args>(args)...);
    }
    TIRO_UNREACHABLE("Invalid ModuleMemberData type.");
}
/// [[[end]]]

namespace dump_helpers {

struct DumpModuleMember {
    const Module& parent;
    const ModuleMember& member;
};

void format(const DumpModuleMember& d, FormatStream& stream);

} // namespace dump_helpers

} // namespace tiro::ir

TIRO_ENABLE_MEMBER_HASH(tiro::ir::ModuleMember)

TIRO_ENABLE_FREE_TO_STRING(tiro::ir::ModuleMemberType)
TIRO_ENABLE_MEMBER_FORMAT(tiro::ir::ModuleMember)

TIRO_ENABLE_FREE_FORMAT(tiro::ir::dump_helpers::DumpModuleMember);

#endif // TIRO_COMPILER_IR_MODULE_HPP
