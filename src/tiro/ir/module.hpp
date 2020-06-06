#ifndef TIRO_IR_MODULE_HPP
#define TIRO_IR_MODULE_HPP

#include "tiro/core/defs.hpp"
#include "tiro/core/id_type.hpp"
#include "tiro/core/index_map.hpp"
#include "tiro/core/not_null.hpp"
#include "tiro/core/string_table.hpp"
#include "tiro/ir/fwd.hpp"
#include "tiro/ir/id.hpp"

namespace tiro {

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

    NotNull<VecPtr<ModuleMember>> operator[](ModuleMemberId id);
    NotNull<VecPtr<Function>> operator[](FunctionId id);

    NotNull<VecPtr<const ModuleMember>> operator[](ModuleMemberId id) const;
    NotNull<VecPtr<const Function>> operator[](FunctionId id) const;

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
    IndexMap<ModuleMember, IdMapper<ModuleMemberId>> members_;
    IndexMap<Function, IdMapper<FunctionId>> functions_;
};

void dump_module(const Module& module, FormatStream& stream);

/* [[[cog
    from codegen.unions import define
    from codegen.ir import ModuleMember
    define(ModuleMember.tag)
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
    from codegen.ir import ModuleMember
    define(ModuleMember)
]]] */
/// Represents a member of a module.
class ModuleMember final {
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

    static ModuleMember make_import(const InternedString& name);
    static ModuleMember make_variable(const InternedString& name);
    static ModuleMember make_function(const FunctionId& id);

    ModuleMember(Import import);
    ModuleMember(Variable variable);
    ModuleMember(Function function);

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

/* [[[cog
    from codegen.unions import implement_inlines
    from codegen.ir import ModuleMember
    implement_inlines(ModuleMember)
]]] */
template<typename Self, typename Visitor, typename... Args>
decltype(auto) ModuleMember::visit_impl(Self&& self, Visitor&& vis, Args&&... args) {
    switch (self.type()) {
    case ModuleMemberType::Import:
        return vis.visit_import(self.import_, std::forward<Args>(args)...);
    case ModuleMemberType::Variable:
        return vis.visit_variable(self.variable_, std::forward<Args>(args)...);
    case ModuleMemberType::Function:
        return vis.visit_function(self.function_, std::forward<Args>(args)...);
    }
    TIRO_UNREACHABLE("Invalid ModuleMember type.");
}
/// [[[end]]]

namespace dump_helpers {

struct DumpModuleMember {
    const Module& parent;
    const ModuleMember& member;
};

void format(const DumpModuleMember& d, FormatStream& stream);

} // namespace dump_helpers

} // namespace tiro

TIRO_ENABLE_BUILD_HASH(tiro::ModuleMember)

TIRO_ENABLE_FREE_TO_STRING(tiro::ModuleMemberType)
TIRO_ENABLE_MEMBER_FORMAT(tiro::ModuleMember)

TIRO_ENABLE_FREE_FORMAT(tiro::dump_helpers::DumpModuleMember);

#endif // TIRO_IR_MODULE_HPP
