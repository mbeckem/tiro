#include "compiler/bytecode_gen/object.hpp"

namespace tiro {

/* [[[cog
    from codegen.unions import implement
    from codegen.bytecode_gen import LinkItemType
    implement(LinkItemType)
]]] */
std::string_view to_string(LinkItemType type) {
    switch (type) {
    case LinkItemType::Use:
        return "Use";
    case LinkItemType::Definition:
        return "Definition";
    }
    TIRO_UNREACHABLE("Invalid LinkItemType.");
}
// [[[end]]]

/* [[[cog
    from codegen.unions import implement
    from codegen.bytecode_gen import LinkItem
    implement(LinkItem)
]]] */
LinkItem LinkItem::make_use(const Use& use) {
    return use;
}

LinkItem LinkItem::make_definition(const ir::ModuleMemberId& ir_id, const BytecodeMember& value) {
    return {Definition{ir_id, value}};
}

LinkItem::LinkItem(Use use)
    : type_(LinkItemType::Use)
    , use_(std::move(use)) {}

LinkItem::LinkItem(Definition definition)
    : type_(LinkItemType::Definition)
    , definition_(std::move(definition)) {}

const LinkItem::Use& LinkItem::as_use() const {
    TIRO_DEBUG_ASSERT(type_ == LinkItemType::Use, "Bad member access on LinkItem: not a Use.");
    return use_;
}

const LinkItem::Definition& LinkItem::as_definition() const {
    TIRO_DEBUG_ASSERT(
        type_ == LinkItemType::Definition, "Bad member access on LinkItem: not a Definition.");
    return definition_;
}

void LinkItem::format(FormatStream& stream) const {
    struct FormatVisitor {
        FormatStream& stream;

        void visit_use([[maybe_unused]] const Use& use) { stream.format("{}", use); }

        void visit_definition([[maybe_unused]] const Definition& definition) {
            stream.format("Definition(ir_id: {}, value: {})", definition.ir_id, definition.value);
        }
    };
    visit(FormatVisitor{stream});
}

void LinkItem::hash(Hasher& h) const {
    h.append(type());

    struct HashVisitor {
        Hasher& h;

        void visit_use([[maybe_unused]] const Use& use) { h.append(use); }

        void visit_definition([[maybe_unused]] const Definition& definition) {
            h.append(definition.ir_id).append(definition.value);
        }
    };
    return visit(HashVisitor{h});
}

bool operator==(const LinkItem& lhs, const LinkItem& rhs) {
    if (lhs.type() != rhs.type())
        return false;

    struct EqualityVisitor {
        const LinkItem& rhs;

        bool visit_use([[maybe_unused]] const LinkItem::Use& use) {
            [[maybe_unused]] const auto& other = rhs.as_use();
            return use == other;
        }

        bool visit_definition([[maybe_unused]] const LinkItem::Definition& definition) {
            [[maybe_unused]] const auto& other = rhs.as_definition();
            return definition.ir_id == other.ir_id && definition.value == other.value;
        }
    };
    return lhs.visit(EqualityVisitor{rhs});
}

bool operator!=(const LinkItem& lhs, const LinkItem& rhs) {
    return !(lhs == rhs);
}
// [[[end]]]

LinkObject::LinkObject() {}

LinkObject::~LinkObject() {}

BytecodeMemberId LinkObject::use_integer(i64 value) {
    return add_member(LinkItem::make_definition({}, BytecodeMember::make_integer(value)));
}

BytecodeMemberId LinkObject::use_float(f64 value) {
    return add_member(LinkItem::make_definition({}, BytecodeMember::make_float(value)));
}

BytecodeMemberId LinkObject::use_string(InternedString value) {
    TIRO_DEBUG_ASSERT(value, "Invalid string.");
    return add_member(LinkItem::make_definition({}, BytecodeMember::make_string(value)));
}

BytecodeMemberId LinkObject::use_symbol(InternedString sym) {
    const auto str = use_string(sym);
    return add_member(LinkItem::make_definition({}, BytecodeMember::make_symbol(str)));
}

BytecodeMemberId LinkObject::use_definition(ir::ModuleMemberId ir_id) {
    return add_member(LinkItem::make_use(ir_id));
}

BytecodeMemberId LinkObject::use_record(Span<const BytecodeMemberId> keys) {
    const auto record_id = [&] {
        if (auto pos = record_index_.find(keys); pos != record_index_.end())
            return pos->second;

        BytecodeRecordSchemaId id;
        {
            BytecodeRecordSchema tmpl;
            tmpl.keys().assign(keys.begin(), keys.end());
            id = records_.push_back(std::move(tmpl));
        }
        record_index_.emplace(RecordKey(keys.begin(), keys.end()), id);
        return id;
    }();

    return add_member(LinkItem::make_definition({}, BytecodeMember::make_record_schema(record_id)));
}

BytecodeMemberId LinkObject::use_import(InternedString name) {
    const auto str = use_string(name);
    return add_member(LinkItem::make_definition({}, BytecodeMember::make_import(str)));
}

BytecodeMemberId
LinkObject::define_variable(ir::ModuleMemberId ir_id, const BytecodeMember::Variable& var) {
    return add_member(LinkItem::make_definition(ir_id, var));
}

BytecodeMemberId LinkObject::define_function(ir::ModuleMemberId ir_id, LinkFunction&& func) {
    auto func_id = functions_.push_back(std::move(func));
    return add_member(LinkItem::make_definition(ir_id, BytecodeMember::Function{func_id}));
}

void LinkObject::define_export(InternedString name, BytecodeMemberId member_id) {
    auto symbol_id = use_symbol(name);
    exports_.emplace_back(symbol_id, member_id);
}

BytecodeMemberId LinkObject::add_member(const LinkItem& member) {
    if (auto pos = item_index_.find(member); pos != item_index_.end()) {
        return pos->second;
    }

    auto id = items_.push_back(member);
    item_index_[member] = id;
    return id;
}

} // namespace tiro
