#include "compiler/ir_gen/support.hpp"

namespace tiro {

/* [[[cog
    from codegen.unions import implement
    from codegen.ir_gen import ComputedValueType
    implement(ComputedValueType)
]]] */
std::string_view to_string(ComputedValueType type) {
    switch (type) {
    case ComputedValueType::Constant:
        return "Constant";
    case ComputedValueType::ModuleMemberId:
        return "ModuleMemberId";
    case ComputedValueType::UnaryOp:
        return "UnaryOp";
    case ComputedValueType::BinaryOp:
        return "BinaryOp";
    case ComputedValueType::AggregateMemberRead:
        return "AggregateMemberRead";
    }
    TIRO_UNREACHABLE("Invalid ComputedValueType.");
}
// [[[end]]]

/* [[[cog
    from codegen.unions import implement
    from codegen.ir_gen import ComputedValue
    implement(ComputedValue)
]]] */
ComputedValue ComputedValue::make_constant(const Constant& constant) {
    return constant;
}

ComputedValue ComputedValue::make_module_member_id(const ModuleMemberId& module_member_id) {
    return module_member_id;
}

ComputedValue ComputedValue::make_unary_op(const UnaryOpType& op, const LocalId& operand) {
    return {UnaryOp{op, operand}};
}

ComputedValue
ComputedValue::make_binary_op(const BinaryOpType& op, const LocalId& left, const LocalId& right) {
    return {BinaryOp{op, left, right}};
}

ComputedValue
ComputedValue::make_aggregate_member_read(const LocalId& aggregate, const AggregateMember& member) {
    return {AggregateMemberRead{aggregate, member}};
}

ComputedValue::ComputedValue(Constant constant)
    : type_(ComputedValueType::Constant)
    , constant_(std::move(constant)) {}

ComputedValue::ComputedValue(ModuleMemberId module_member_id)
    : type_(ComputedValueType::ModuleMemberId)
    , module_member_id_(std::move(module_member_id)) {}

ComputedValue::ComputedValue(UnaryOp unary_op)
    : type_(ComputedValueType::UnaryOp)
    , unary_op_(std::move(unary_op)) {}

ComputedValue::ComputedValue(BinaryOp binary_op)
    : type_(ComputedValueType::BinaryOp)
    , binary_op_(std::move(binary_op)) {}

ComputedValue::ComputedValue(AggregateMemberRead aggregate_member_read)
    : type_(ComputedValueType::AggregateMemberRead)
    , aggregate_member_read_(std::move(aggregate_member_read)) {}

const ComputedValue::Constant& ComputedValue::as_constant() const {
    TIRO_DEBUG_ASSERT(type_ == ComputedValueType::Constant,
        "Bad member access on ComputedValue: not a Constant.");
    return constant_;
}

const ComputedValue::ModuleMemberId& ComputedValue::as_module_member_id() const {
    TIRO_DEBUG_ASSERT(type_ == ComputedValueType::ModuleMemberId,
        "Bad member access on ComputedValue: not a ModuleMemberId.");
    return module_member_id_;
}

const ComputedValue::UnaryOp& ComputedValue::as_unary_op() const {
    TIRO_DEBUG_ASSERT(
        type_ == ComputedValueType::UnaryOp, "Bad member access on ComputedValue: not a UnaryOp.");
    return unary_op_;
}

const ComputedValue::BinaryOp& ComputedValue::as_binary_op() const {
    TIRO_DEBUG_ASSERT(type_ == ComputedValueType::BinaryOp,
        "Bad member access on ComputedValue: not a BinaryOp.");
    return binary_op_;
}

const ComputedValue::AggregateMemberRead& ComputedValue::as_aggregate_member_read() const {
    TIRO_DEBUG_ASSERT(type_ == ComputedValueType::AggregateMemberRead,
        "Bad member access on ComputedValue: not a AggregateMemberRead.");
    return aggregate_member_read_;
}

void ComputedValue::format(FormatStream& stream) const {
    struct FormatVisitor {
        FormatStream& stream;

        void visit_constant([[maybe_unused]] const Constant& constant) {
            stream.format("{}", constant);
        }

        void visit_module_member_id([[maybe_unused]] const ModuleMemberId& module_member_id) {
            stream.format("{}", module_member_id);
        }

        void visit_unary_op([[maybe_unused]] const UnaryOp& unary_op) {
            stream.format("UnaryOp(op: {}, operand: {})", unary_op.op, unary_op.operand);
        }

        void visit_binary_op([[maybe_unused]] const BinaryOp& binary_op) {
            stream.format("BinaryOp(op: {}, left: {}, right: {})", binary_op.op, binary_op.left,
                binary_op.right);
        }

        void visit_aggregate_member_read(
            [[maybe_unused]] const AggregateMemberRead& aggregate_member_read) {
            stream.format("AggregateMemberRead(aggregate: {}, member: {})",
                aggregate_member_read.aggregate, aggregate_member_read.member);
        }
    };
    visit(FormatVisitor{stream});
}

void ComputedValue::build_hash(Hasher& h) const {
    h.append(type());

    struct HashVisitor {
        Hasher& h;

        void visit_constant([[maybe_unused]] const Constant& constant) { h.append(constant); }

        void visit_module_member_id([[maybe_unused]] const ModuleMemberId& module_member_id) {
            h.append(module_member_id);
        }

        void visit_unary_op([[maybe_unused]] const UnaryOp& unary_op) {
            h.append(unary_op.op).append(unary_op.operand);
        }

        void visit_binary_op([[maybe_unused]] const BinaryOp& binary_op) {
            h.append(binary_op.op).append(binary_op.left).append(binary_op.right);
        }

        void visit_aggregate_member_read(
            [[maybe_unused]] const AggregateMemberRead& aggregate_member_read) {
            h.append(aggregate_member_read.aggregate).append(aggregate_member_read.member);
        }
    };
    return visit(HashVisitor{h});
}

bool operator==(const ComputedValue& lhs, const ComputedValue& rhs) {
    if (lhs.type() != rhs.type())
        return false;

    struct EqualityVisitor {
        const ComputedValue& rhs;

        bool visit_constant([[maybe_unused]] const ComputedValue::Constant& constant) {
            [[maybe_unused]] const auto& other = rhs.as_constant();
            return constant == other;
        }

        bool visit_module_member_id(
            [[maybe_unused]] const ComputedValue::ModuleMemberId& module_member_id) {
            [[maybe_unused]] const auto& other = rhs.as_module_member_id();
            return module_member_id == other;
        }

        bool visit_unary_op([[maybe_unused]] const ComputedValue::UnaryOp& unary_op) {
            [[maybe_unused]] const auto& other = rhs.as_unary_op();
            return unary_op.op == other.op && unary_op.operand == other.operand;
        }

        bool visit_binary_op([[maybe_unused]] const ComputedValue::BinaryOp& binary_op) {
            [[maybe_unused]] const auto& other = rhs.as_binary_op();
            return binary_op.op == other.op && binary_op.left == other.left
                   && binary_op.right == other.right;
        }

        bool visit_aggregate_member_read(
            [[maybe_unused]] const ComputedValue::AggregateMemberRead& aggregate_member_read) {
            [[maybe_unused]] const auto& other = rhs.as_aggregate_member_read();
            return aggregate_member_read.aggregate == other.aggregate
                   && aggregate_member_read.member == other.member;
        }
    };
    return lhs.visit(EqualityVisitor{rhs});
}

bool operator!=(const ComputedValue& lhs, const ComputedValue& rhs) {
    return !(lhs == rhs);
}
// [[[end]]]

/* [[[cog
    from codegen.unions import implement
    from codegen.ir_gen import AssignTargetType
    implement(AssignTargetType)
]]] */
std::string_view to_string(AssignTargetType type) {
    switch (type) {
    case AssignTargetType::LValue:
        return "LValue";
    case AssignTargetType::Symbol:
        return "Symbol";
    }
    TIRO_UNREACHABLE("Invalid AssignTargetType.");
}
// [[[end]]]

/* [[[cog
    from codegen.unions import implement
    from codegen.ir_gen import AssignTarget
    implement(AssignTarget)
]]] */
AssignTarget AssignTarget::make_lvalue(const LValue& lvalue) {
    return lvalue;
}

AssignTarget AssignTarget::make_symbol(const Symbol& symbol) {
    return symbol;
}

AssignTarget::AssignTarget(LValue lvalue)
    : type_(AssignTargetType::LValue)
    , lvalue_(std::move(lvalue)) {}

AssignTarget::AssignTarget(Symbol symbol)
    : type_(AssignTargetType::Symbol)
    , symbol_(std::move(symbol)) {}

const AssignTarget::LValue& AssignTarget::as_lvalue() const {
    TIRO_DEBUG_ASSERT(
        type_ == AssignTargetType::LValue, "Bad member access on AssignTarget: not a LValue.");
    return lvalue_;
}

const AssignTarget::Symbol& AssignTarget::as_symbol() const {
    TIRO_DEBUG_ASSERT(
        type_ == AssignTargetType::Symbol, "Bad member access on AssignTarget: not a Symbol.");
    return symbol_;
}

// [[[end]]]

} // namespace tiro