#ifndef TIRO_MIR_SUPPORT_HPP
#define TIRO_MIR_SUPPORT_HPP

#include "tiro/core/defs.hpp"
#include "tiro/core/format_stream.hpp"
#include "tiro/core/hash.hpp"
#include "tiro/mir/fwd.hpp"
#include "tiro/mir/types.hpp"

#include <string_view>

namespace tiro::compiler::mir_transform {

/* [[[cog
    import unions
    import mir_transform
    unions.define_type(mir_transform.ComputedValueType)
]]] */
enum class ComputedValueType : u8 {
    Constant,
    UnaryOp,
    BinaryOp,
};

std::string_view to_string(ComputedValueType type);
// [[[end]]]

/* [[[cog
    import unions
    import mir_transform
    unions.define_type(mir_transform.ComputedValue)
]]] */
/// Represents a reusable local variable for a certain operation.
class ComputedValue final {
public:
    /// A known constant.
    using Constant = mir::Constant;

    /// The known result of a unary operation.
    struct UnaryOp final {
        /// The unary operator.
        mir::UnaryOpType op;

        /// The operand value.
        mir::LocalID operand;

        UnaryOp(const mir::UnaryOpType& op_, const mir::LocalID& operand_)
            : op(op_)
            , operand(operand_) {}
    };

    /// The known result of a binary operation.
    struct BinaryOp final {
        /// The binary operator.
        mir::BinaryOpType op;

        /// The left operand.
        mir::LocalID left;

        /// The right operand.
        mir::LocalID right;

        BinaryOp(const mir::BinaryOpType& op_, const mir::LocalID& left_,
            const mir::LocalID& right_)
            : op(op_)
            , left(left_)
            , right(right_) {}
    };

    static ComputedValue make_constant(const Constant& constant);
    static ComputedValue
    make_unary_op(const mir::UnaryOpType& op, const mir::LocalID& operand);
    static ComputedValue make_binary_op(const mir::BinaryOpType& op,
        const mir::LocalID& left, const mir::LocalID& right);

    ComputedValue(const Constant& constant);
    ComputedValue(const UnaryOp& unary_op);
    ComputedValue(const BinaryOp& binary_op);

    ComputedValueType type() const noexcept { return type_; }

    void format(FormatStream& stream) const;

    void build_hash(Hasher& h) const;

    const Constant& as_constant() const;
    const UnaryOp& as_unary_op() const;
    const BinaryOp& as_binary_op() const;

    template<typename Visitor>
    TIRO_FORCE_INLINE decltype(auto) visit(Visitor&& vis) const {
        return visit_impl(*this, std::forward<Visitor>(vis));
    }

private:
    template<typename Self, typename Visitor>
    static TIRO_FORCE_INLINE decltype(auto)
    visit_impl(Self&& self, Visitor&& vis);

private:
    ComputedValueType type_;
    union {
        Constant constant_;
        UnaryOp unary_op_;
        BinaryOp binary_op_;
    };
};

bool operator==(const ComputedValue& lhs, const ComputedValue& rhs);
bool operator!=(const ComputedValue& lhs, const ComputedValue& rhs);
// [[[end]]]

/* [[[cog
    import cog
    import unions
    import mir_transform
    unions.define_inlines(mir_transform.ComputedValue)
]]] */
template<typename Self, typename Visitor>
decltype(auto) ComputedValue::visit_impl(Self&& self, Visitor&& vis) {
    switch (self.type()) {
    case ComputedValueType::Constant:
        return vis.visit_constant(self.constant_);
    case ComputedValueType::UnaryOp:
        return vis.visit_unary_op(self.unary_op_);
    case ComputedValueType::BinaryOp:
        return vis.visit_binary_op(self.binary_op_);
    }
    TIRO_UNREACHABLE("Invalid ComputedValue type.");
}
// [[[end]]]

} // namespace tiro::compiler::mir_transform

TIRO_ENABLE_BUILD_HASH(tiro::compiler::mir_transform::ComputedValue)

TIRO_ENABLE_FREE_TO_STRING(tiro::compiler::mir_transform::ComputedValueType)
TIRO_ENABLE_MEMBER_FORMAT(tiro::compiler::mir_transform::ComputedValue)

#endif // TIRO_MIR_SUPPORT_HPP
