#include "tiro/bytecode_gen/link.hpp"

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

LinkItem LinkItem::make_definition(
    const ModuleMemberID& ir_id, const BytecodeMember& value) {
    return {Definition{ir_id, value}};
}

LinkItem::LinkItem(Use use)
    : type_(LinkItemType::Use)
    , use_(std::move(use)) {}

LinkItem::LinkItem(Definition definition)
    : type_(LinkItemType::Definition)
    , definition_(std::move(definition)) {}

const LinkItem::Use& LinkItem::as_use() const {
    TIRO_DEBUG_ASSERT(type_ == LinkItemType::Use,
        "Bad member access on LinkItem: not a Use.");
    return use_;
}

const LinkItem::Definition& LinkItem::as_definition() const {
    TIRO_DEBUG_ASSERT(type_ == LinkItemType::Definition,
        "Bad member access on LinkItem: not a Definition.");
    return definition_;
}

void LinkItem::format(FormatStream& stream) const {
    struct FormatVisitor {
        FormatStream& stream;

        void visit_use([[maybe_unused]] const Use& use) {
            stream.format("{}", use);
        }

        void visit_definition([[maybe_unused]] const Definition& definition) {
            stream.format("Definition(ir_id: {}, value: {})", definition.ir_id,
                definition.value);
        }
    };
    visit(FormatVisitor{stream});
}

void LinkItem::build_hash(Hasher& h) const {
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

        bool visit_definition(
            [[maybe_unused]] const LinkItem::Definition& definition) {
            [[maybe_unused]] const auto& other = rhs.as_definition();
            return definition.ir_id == other.ir_id
                   && definition.value == other.value;
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

BytecodeMemberID LinkObject::use_integer(i64 value) {
    return add_member(
        LinkItem::make_definition({}, BytecodeMember::make_integer(value)));
}

BytecodeMemberID LinkObject::use_float(f64 value) {
    return add_member(
        LinkItem::make_definition({}, BytecodeMember::make_float(value)));
}

BytecodeMemberID LinkObject::use_string(InternedString value) {
    TIRO_DEBUG_ASSERT(value, "Invalid string.");
    return add_member(
        LinkItem::make_definition({}, BytecodeMember::make_string(value)));
}

BytecodeMemberID LinkObject::use_symbol(InternedString sym) {
    const auto str = use_string(sym);
    return add_member(
        LinkItem::make_definition({}, BytecodeMember::make_symbol(str)));
}

BytecodeMemberID LinkObject::use_member(ModuleMemberID ir_id) {
    return add_member(LinkItem::make_use(ir_id));
}

void LinkObject::define_import(
    ModuleMemberID ir_id, const BytecodeMember::Import& import) {
    add_member(LinkItem::make_definition(ir_id, import));
}

void LinkObject::define_variable(
    ModuleMemberID ir_id, const BytecodeMember::Variable& var) {
    add_member(LinkItem::make_definition(ir_id, var));
}

void LinkObject::define_function(ModuleMemberID ir_id, LinkFunction&& func) {
    auto func_id = functions_.push_back(std::move(func));
    add_member(
        LinkItem::make_definition(ir_id, BytecodeMember::Function{func_id}));
}

BytecodeMemberID LinkObject::add_member(const LinkItem& member) {
    if (auto pos = data_index_.find(member); pos != data_index_.end()) {
        return pos->second;
    }

    auto id = data_.push_back(member);
    data_index_[member] = id;
    return id;
}

} // namespace tiro
