#ifndef TIRO_MIR_GEN_SUPPORT_HPP
#define TIRO_MIR_GEN_SUPPORT_HPP

#include "tiro/core/defs.hpp"
#include "tiro/core/format.hpp"
#include "tiro/core/hash.hpp"
#include "tiro/core/not_null.hpp"
#include "tiro/mir/fwd.hpp"
#include "tiro/mir/types.hpp"
#include "tiro/semantics/symbol_table.hpp" // TODO fwd

#include <string_view>

namespace tiro {

/* [[[cog
    import unions
    import mir_gen
    unions.define_type(mir_gen.ComputedValueType)
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
    import mir_gen
    unions.define_type(mir_gen.ComputedValue)
]]] */
/// Represents a reusable local variable for a certain operation.
class ComputedValue final {
public:
    /// A known constant.
    using Constant = tiro::Constant;

    /// The known result of a unary operation.
    struct UnaryOp final {
        /// The unary operator.
        UnaryOpType op;

        /// The operand value.
        LocalID operand;

        UnaryOp(const UnaryOpType& op_, const LocalID& operand_)
            : op(op_)
            , operand(operand_) {}
    };

    /// The known result of a binary operation.
    struct BinaryOp final {
        /// The binary operator.
        BinaryOpType op;

        /// The left operand.
        LocalID left;

        /// The right operand.
        LocalID right;

        BinaryOp(const BinaryOpType& op_, const LocalID& left_,
            const LocalID& right_)
            : op(op_)
            , left(left_)
            , right(right_) {}
    };

    static ComputedValue make_constant(const Constant& constant);
    static ComputedValue
    make_unary_op(const UnaryOpType& op, const LocalID& operand);
    static ComputedValue make_binary_op(
        const BinaryOpType& op, const LocalID& left, const LocalID& right);

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
    TIRO_FORCE_INLINE decltype(auto) visit(Visitor&& vis) {
        return visit_impl(*this, std::forward<Visitor>(vis));
    }

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
    import unions
    import mir_gen
    unions.define_type(mir_gen.AssignTargetType)
]]] */
enum class AssignTargetType : u8 {
    LValue,
    Symbol,
};

std::string_view to_string(AssignTargetType type);
// [[[end]]]

/* [[[cog
    import unions
    import mir_gen
    unions.define_type(mir_gen.AssignTarget)
]]] */
/// Represents the left hand side of an assignment during compilation.
class AssignTarget final {
public:
    /// An ir lvalue
    using LValue = tiro::LValue;

    /// Represents a symbol.
    using Symbol = NotNull<tiro::Symbol*>;

    static AssignTarget make_lvalue(const LValue& lvalue);
    static AssignTarget make_symbol(const Symbol& symbol);

    AssignTarget(const LValue& lvalue);
    AssignTarget(const Symbol& symbol);

    AssignTargetType type() const noexcept { return type_; }

    const LValue& as_lvalue() const;
    const Symbol& as_symbol() const;

    template<typename Visitor>
    TIRO_FORCE_INLINE decltype(auto) visit(Visitor&& vis) {
        return visit_impl(*this, std::forward<Visitor>(vis));
    }

    template<typename Visitor>
    TIRO_FORCE_INLINE decltype(auto) visit(Visitor&& vis) const {
        return visit_impl(*this, std::forward<Visitor>(vis));
    }

private:
    template<typename Self, typename Visitor>
    static TIRO_FORCE_INLINE decltype(auto)
    visit_impl(Self&& self, Visitor&& vis);

private:
    AssignTargetType type_;
    union {
        LValue lvalue_;
        Symbol symbol_;
    };
};
// [[[end]]]

/* [[[cog
    import cog
    import unions
    import mir_gen
    unions.define_inlines(mir_gen.ComputedValue)
    cog.outl()
    unions.define_inlines(mir_gen.AssignTarget)
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

template<typename Self, typename Visitor>
decltype(auto) AssignTarget::visit_impl(Self&& self, Visitor&& vis) {
    switch (self.type()) {
    case AssignTargetType::LValue:
        return vis.visit_lvalue(self.lvalue_);
    case AssignTargetType::Symbol:
        return vis.visit_symbol(self.symbol_);
    }
    TIRO_UNREACHABLE("Invalid AssignTarget type.");
}
// [[[end]]]

} // namespace tiro

TIRO_ENABLE_BUILD_HASH(tiro::ComputedValue)

TIRO_ENABLE_FREE_TO_STRING(tiro::ComputedValueType)
TIRO_ENABLE_FREE_TO_STRING(tiro::AssignTargetType)
TIRO_ENABLE_MEMBER_FORMAT(tiro::ComputedValue)
TIRO_ENABLE_MEMBER_FORMAT(tiro::AssignTarget)

#endif // TIRO_MIR_GEN_SUPPORT_HPP
