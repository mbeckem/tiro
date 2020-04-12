#ifndef TIRO_BYTECODE_GEN_GEN_FUNC_HPP
#define TIRO_BYTECODE_GEN_GEN_FUNC_HPP

#include "tiro/bytecode/instruction.hpp"
#include "tiro/bytecode/module.hpp"
#include "tiro/core/format.hpp"
#include "tiro/core/hash.hpp"
#include "tiro/core/index_map.hpp"
#include "tiro/core/not_null.hpp"
#include "tiro/ir/types.hpp"

#include <unordered_map>
#include <vector>

namespace tiro {

/* [[[cog
    import unions
    import bytecode_gen
    unions.define_type(bytecode_gen.LinkItemType)
]]] */
/// Represents the type of an external item referenced by the bytecode.
enum class LinkItemType : u8 {
    Use,
    Definition,
};

std::string_view to_string(LinkItemType type);
// [[[end]]]

/* [[[cog
    import unions
    import bytecode_gen
    unions.define_type(bytecode_gen.LinkItem)
]]] */
/// Represents an external item referenced by the bytecode.
/// These references must be patched when the module is being linked.
class LinkItem final {
public:
    /// References a ir module member, possibly defined in another object.
    using Use = ModuleMemberID;

    /// A definition made in the current object.
    struct Definition final {
        /// ID of this definition in the IR. May be invalid (for anonymous constants etc.).
        ModuleMemberID ir_id;
        BytecodeMember value;

        Definition(const ModuleMemberID& ir_id_, const BytecodeMember& value_)
            : ir_id(ir_id_)
            , value(value_) {}
    };

    static LinkItem make_use(const Use& use);
    static LinkItem
    make_definition(const ModuleMemberID& ir_id, const BytecodeMember& value);

    LinkItem(const Use& use);
    LinkItem(const Definition& definition);

    LinkItemType type() const noexcept { return type_; }

    void format(FormatStream& stream) const;

    void build_hash(Hasher& h) const;

    const Use& as_use() const;
    const Definition& as_definition() const;

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
    LinkItemType type_;
    union {
        Use use_;
        Definition definition_;
    };
};

bool operator==(const LinkItem& lhs, const LinkItem& rhs);
bool operator!=(const LinkItem& lhs, const LinkItem& rhs);
// [[[end]]]

struct LinkFunction {
    /// Incomplete function representation. Contains unpatched bytecode wrt module items.
    BytecodeFunction func;

    /// Places where the items are referenced (byte offset -> item id).
    std::vector<std::tuple<u32, BytecodeMemberID>> refs_;
};

class LinkObject final {
public:
    LinkObject();
    ~LinkObject();

    LinkObject(LinkObject&&) noexcept = default;
    LinkObject& operator=(LinkObject&&) noexcept = default;

    BytecodeMemberID use_integer(i64 value);
    BytecodeMemberID use_float(f64 value);
    BytecodeMemberID use_string(InternedString str);
    BytecodeMemberID use_symbol(InternedString sym);
    BytecodeMemberID use_member(ModuleMemberID ir_id);

    void
    define_import(ModuleMemberID ir_id, const BytecodeMember::Import& import);
    void
    define_variable(ModuleMemberID ir_id, const BytecodeMember::Variable& var);
    void define_function(ModuleMemberID ir_id, LinkFunction&& func);

    auto item_ids() const { return data_.keys(); }
    auto function_ids() const { return functions_.keys(); }

    NotNull<IndexMapPtr<LinkItem>> operator[](BytecodeMemberID id) {
        return TIRO_NN(data_.ptr_to(id));
    }

    NotNull<IndexMapPtr<const LinkItem>> operator[](BytecodeMemberID id) const {
        return TIRO_NN(data_.ptr_to(id));
    }

    NotNull<IndexMapPtr<LinkFunction>> operator[](BytecodeFunctionID id) {
        return TIRO_NN(functions_.ptr_to(id));
    }

    NotNull<IndexMapPtr<const LinkFunction>>
    operator[](BytecodeFunctionID id) const {
        return TIRO_NN(functions_.ptr_to(id));
    }

private:
    BytecodeMemberID add_member(const LinkItem& member);

private:
    /// External items used by the bytecode of the compiled functions.
    IndexMap<LinkItem, IDMapper<BytecodeMemberID>> data_;

    /// Deduplicates members (especially constants).
    // TODO: Container
    std::unordered_map<LinkItem, BytecodeMemberID, UseHasher> data_index_;

    /// Compiled functions. Bytecode must be patched when the module is linked (indices
    /// to module constants point into data_).
    IndexMap<LinkFunction, IDMapper<BytecodeFunctionID>> functions_;
};

/* [[[cog
    import unions
    import bytecode_gen
    unions.define_inlines(bytecode_gen.LinkItem)
]]] */
template<typename Self, typename Visitor, typename... Args>
decltype(auto)
LinkItem::visit_impl(Self&& self, Visitor&& vis, Args&&... args) {
    switch (self.type()) {
    case LinkItemType::Use:
        return vis.visit_use(self.use_, std::forward<Args>(args)...);
    case LinkItemType::Definition:
        return vis.visit_definition(
            self.definition_, std::forward<Args>(args)...);
    }
    TIRO_UNREACHABLE("Invalid LinkItem type.");
}
// [[[end]]]

/// Compiles the given members of the module into a link object.
/// Objects must be linked together to produce the completed bytecode module.
LinkObject compile_object(Module& module, Span<const ModuleMemberID> members);

} // namespace tiro

TIRO_ENABLE_BUILD_HASH(tiro::LinkItem)

TIRO_ENABLE_FREE_TO_STRING(tiro::LinkItemType)
TIRO_ENABLE_MEMBER_FORMAT(tiro::LinkItem)

#endif // TIRO_BYTECODE_GEN_GEN_FUNC_HPP
