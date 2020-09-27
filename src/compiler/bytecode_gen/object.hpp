#ifndef TIRO_COMPILER_BYTECODE_GEN_OBJECT_HPP
#define TIRO_COMPILER_BYTECODE_GEN_OBJECT_HPP

#include "common/defs.hpp"
#include "common/index_map.hpp"
#include "common/not_null.hpp"
#include "compiler/bytecode/module.hpp"
#include "compiler/ir/function.hpp"

#include "absl/container/flat_hash_map.h"
#include "absl/container/flat_hash_set.h"
#include "absl/container/node_hash_map.h"

#include <string_view>
#include <tuple>
#include <vector>

namespace tiro {

/* [[[cog
    from codegen.unions import define
    from codegen.bytecode_gen import LinkItemType
    define(LinkItemType)
]]] */
/// Represents the type of an external item referenced by the bytecode.
enum class LinkItemType : u8 {
    Use,
    Definition,
};

std::string_view to_string(LinkItemType type);
// [[[end]]]

/* [[[cog
    from codegen.unions import define
    from codegen.bytecode_gen import LinkItem
    define(LinkItem)
]]] */
/// Represents an external item referenced by the bytecode.
/// These references must be patched when the module is being linked.
class LinkItem final {
public:
    /// References a ir module member, possibly defined in another object.
    using Use = ModuleMemberId;

    /// A definition made in the current object.
    struct Definition final {
        /// Id of this definition in the IR. May be invalid (for anonymous constants etc.).
        ModuleMemberId ir_id;
        BytecodeMember value;

        Definition(const ModuleMemberId& ir_id_, const BytecodeMember& value_)
            : ir_id(ir_id_)
            , value(value_) {}
    };

    static LinkItem make_use(const Use& use);
    static LinkItem make_definition(const ModuleMemberId& ir_id, const BytecodeMember& value);

    LinkItem(Use use);
    LinkItem(Definition definition);

    LinkItemType type() const noexcept { return type_; }

    void format(FormatStream& stream) const;

    void hash(Hasher& h) const;

    const Use& as_use() const;
    const Definition& as_definition() const;

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
    std::vector<std::tuple<u32, BytecodeMemberId>> refs_;
};

class LinkObject final {
public:
    LinkObject();
    ~LinkObject();

    LinkObject(LinkObject&&) noexcept = default;
    LinkObject& operator=(LinkObject&&) noexcept = default;

    BytecodeMemberId use_integer(i64 value);
    BytecodeMemberId use_float(f64 value);
    BytecodeMemberId use_string(InternedString str);
    BytecodeMemberId use_symbol(InternedString sym);
    BytecodeMemberId use_member(ModuleMemberId ir_id);
    BytecodeMemberId use_record(Span<const BytecodeMemberId> keys);

    BytecodeMemberId define_import(ModuleMemberId ir_id, const BytecodeMember::Import& import);
    BytecodeMemberId define_variable(ModuleMemberId ir_id, const BytecodeMember::Variable& var);
    BytecodeMemberId define_function(ModuleMemberId ir_id, LinkFunction&& func);

    void define_export(InternedString name, BytecodeMemberId member_id);

    auto item_ids() const { return items_.keys(); }
    auto function_ids() const { return functions_.keys(); }
    auto record_ids() const { return records_.keys(); }

    // Range of (symbol_id, value_id) pairs. Every pair defines a named export.
    auto exports() const { return IterRange(exports_.begin(), exports_.end()); }

    NotNull<IndexMapPtr<LinkItem>> operator[](BytecodeMemberId id) {
        return TIRO_NN(items_.ptr_to(id));
    }

    NotNull<IndexMapPtr<const LinkItem>> operator[](BytecodeMemberId id) const {
        return TIRO_NN(items_.ptr_to(id));
    }

    NotNull<IndexMapPtr<LinkFunction>> operator[](BytecodeFunctionId id) {
        return TIRO_NN(functions_.ptr_to(id));
    }

    NotNull<IndexMapPtr<const LinkFunction>> operator[](BytecodeFunctionId id) const {
        return TIRO_NN(functions_.ptr_to(id));
    }

    NotNull<IndexMapPtr<BytecodeRecordTemplate>> operator[](BytecodeRecordTemplateId id) {
        return TIRO_NN(records_.ptr_to(id));
    }

    NotNull<IndexMapPtr<const BytecodeRecordTemplate>>
    operator[](BytecodeRecordTemplateId id) const {
        return TIRO_NN(records_.ptr_to(id));
    }

private:
    using RecordKey = absl::flat_hash_set<BytecodeMemberId, UseHasher>;

    struct RecordHash {
        using is_transparent = void;

        size_t operator()(const RecordKey& key) const noexcept { return hash_range(key); }

        size_t operator()(Span<const BytecodeMemberId> keys) const noexcept {
            return hash_range(keys);
        }

    private:
        template<typename Range>
        size_t hash_range(const Range& range) const noexcept {
            // Addition for simple, order independent hash values.
            UseHasher hasher;

            size_t hash = 0;
            for (const auto& key : range) {
                hash += hasher(key);
            }
            return hash;
        }
    };

    struct RecordEqual {
        using is_transparent = void;

        bool operator()(Span<const BytecodeMemberId> lhs, const RecordKey& rhs) const noexcept {
            return equal_keys(rhs, lhs);
        }

        bool operator()(const RecordKey& lhs, Span<const BytecodeMemberId> rhs) const noexcept {
            return equal_keys(lhs, rhs);
        }

        bool operator()(const RecordKey& lhs, const RecordKey& rhs) const noexcept {
            return lhs == rhs;
        }

    private:
        bool equal_keys(const RecordKey& lhs, Span<const BytecodeMemberId> rhs) const noexcept {
            if (lhs.size() != rhs.size())
                return false;

            for (const auto& id : rhs) {
                if (!lhs.contains(id))
                    return false;
            }
            return true;
        }
    };

    BytecodeMemberId add_member(const LinkItem& member);

private:
    /// Module-level items used by the bytecode of the compiled functions.
    IndexMap<LinkItem, IdMapper<BytecodeMemberId>> items_;

    /// Deduplicates items. Does not do deep equality checks (for example, all functions
    /// and record templates are unequal).
    absl::flat_hash_map<LinkItem, BytecodeMemberId, UseHasher> item_index_;

    /// Compiled record templates (collection of symbol keys used for record construction).
    /// These are anonymous and immutable and will be shared when the same composition of keys is requested again.
    IndexMap<BytecodeRecordTemplate, IdMapper<BytecodeRecordTemplateId>> records_;

    /// Deduplicates record templates. Maps sets of symbols to a record template id that can be used
    /// to construct a record with those symbols as keys. Spans can be used for querying.
    absl::node_hash_map<RecordKey, BytecodeRecordTemplateId, RecordHash, RecordEqual> record_index_;

    /// Compiled functions. Bytecode must be patched when the module is linked (indices
    /// to module constants point into data_).
    IndexMap<LinkFunction, IdMapper<BytecodeFunctionId>> functions_;

    // Pairs of (symbol_id, value_id).
    std::vector<std::tuple<BytecodeMemberId, BytecodeMemberId>> exports_;
};

/* [[[cog
    from codegen.unions import implement_inlines
    from codegen.bytecode_gen import LinkItem
    implement_inlines(LinkItem)
]]] */
template<typename Self, typename Visitor, typename... Args>
decltype(auto) LinkItem::visit_impl(Self&& self, Visitor&& vis, Args&&... args) {
    switch (self.type()) {
    case LinkItemType::Use:
        return vis.visit_use(self.use_, std::forward<Args>(args)...);
    case LinkItemType::Definition:
        return vis.visit_definition(self.definition_, std::forward<Args>(args)...);
    }
    TIRO_UNREACHABLE("Invalid LinkItem type.");
}
// [[[end]]]

} // namespace tiro

TIRO_ENABLE_MEMBER_HASH(tiro::LinkItem)

TIRO_ENABLE_FREE_TO_STRING(tiro::LinkItemType)
TIRO_ENABLE_MEMBER_FORMAT(tiro::LinkItem)

#endif // TIRO_COMPILER_BYTECODE_GEN_OBJECT_HPP
