#include "tiro/mir/support.hpp"

namespace tiro::compiler::mir_transform {

/* [[[cog
    import mir_transform
    codegen.implement_type(mir_transform.ComputedValueType)
]]] */
std::string_view to_string(ComputedValueType type) {
    switch (type) {
    case ComputedValueType::Constant:
        return "Constant";
    case ComputedValueType::UnaryOp:
        return "UnaryOp";
    case ComputedValueType::BinaryOp:
        return "BinaryOp";
    }
    TIRO_UNREACHABLE("Invalid ComputedValueType.");
}
// [[[end]]]

/* [[[cog
    import mir_transform
    codegen.implement_type(mir_transform.ComputedValue)
]]] */
ComputedValue ComputedValue::make_constant(const Constant& constant) {
    return constant;
}

ComputedValue ComputedValue::make_unary_op(
    const mir::UnaryOpType& op, const mir::LocalID& operand) {
    return UnaryOp{op, operand};
}

ComputedValue ComputedValue::make_binary_op(const mir::BinaryOpType& op,
    const mir::LocalID& left, const mir::LocalID& right) {
    return BinaryOp{op, left, right};
}

ComputedValue::ComputedValue(const Constant& constant)
    : type_(ComputedValueType::Constant)
    , constant_(constant) {}

ComputedValue::ComputedValue(const UnaryOp& unary_op)
    : type_(ComputedValueType::UnaryOp)
    , unary_op_(unary_op) {}

ComputedValue::ComputedValue(const BinaryOp& binary_op)
    : type_(ComputedValueType::BinaryOp)
    , binary_op_(binary_op) {}

const ComputedValue::Constant& ComputedValue::as_constant() const {
    TIRO_ASSERT(type_ == ComputedValueType::Constant,
        "Bad member access on ComputedValue: not a Constant.");
    return constant_;
}

const ComputedValue::UnaryOp& ComputedValue::as_unary_op() const {
    TIRO_ASSERT(type_ == ComputedValueType::UnaryOp,
        "Bad member access on ComputedValue: not a UnaryOp.");
    return unary_op_;
}

const ComputedValue::BinaryOp& ComputedValue::as_binary_op() const {
    TIRO_ASSERT(type_ == ComputedValueType::BinaryOp,
        "Bad member access on ComputedValue: not a BinaryOp.");
    return binary_op_;
}

void ComputedValue::format(FormatStream& stream) const {
    struct Formatter {
        FormatStream& stream;

        void visit_constant([[maybe_unused]] const Constant& constant) {
            stream.format("{}", constant);
        }

        void visit_unary_op([[maybe_unused]] const UnaryOp& unary_op) {
            stream.format(
                "UnaryOp(op: {}, operand: {})", unary_op.op, unary_op.operand);
        }

        void visit_binary_op([[maybe_unused]] const BinaryOp& binary_op) {
            stream.format("BinaryOp(op: {}, left: {}, right: {})", binary_op.op,
                binary_op.left, binary_op.right);
        }
    };
    visit(Formatter{stream});
}

void ComputedValue::build_hash(Hasher& h) const {
    h.append(type());

    struct HashVisitor {
        Hasher& h;

        void visit_constant([[maybe_unused]] const Constant& constant) {
            h.append(constant);
        }

        void visit_unary_op([[maybe_unused]] const UnaryOp& unary_op) {
            h.append(unary_op.op).append(unary_op.operand);
        }

        void visit_binary_op([[maybe_unused]] const BinaryOp& binary_op) {
            h.append(binary_op.op)
                .append(binary_op.left)
                .append(binary_op.right);
        }
    };
    return visit(HashVisitor{h});
}

bool operator==(const ComputedValue& lhs, const ComputedValue& rhs) {
    if (lhs.type() != rhs.type())
        return false;

    struct EqualityVisitor {
        const ComputedValue& rhs;

        bool visit_constant(
            [[maybe_unused]] const ComputedValue::Constant& constant) {
            [[maybe_unused]] const auto& other = rhs.as_constant();
            return constant == other;
        }

        bool visit_unary_op(
            [[maybe_unused]] const ComputedValue::UnaryOp& unary_op) {
            [[maybe_unused]] const auto& other = rhs.as_unary_op();
            return unary_op.op == other.op && unary_op.operand == other.operand;
        }

        bool visit_binary_op(
            [[maybe_unused]] const ComputedValue::BinaryOp& binary_op) {
            [[maybe_unused]] const auto& other = rhs.as_binary_op();
            return binary_op.op == other.op && binary_op.left == other.left
                   && binary_op.right == other.right;
        }
    };
    return lhs.visit(EqualityVisitor{rhs});
}

bool operator!=(const ComputedValue& lhs, const ComputedValue& rhs) {
    return !(lhs == rhs);
}
// [[[end]]]

} // namespace tiro::compiler::mir_transform
