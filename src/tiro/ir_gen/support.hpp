#ifndef TIRO_IR_GEN_SUPPORT_HPP
#define TIRO_IR_GEN_SUPPORT_HPP

#include "tiro/core/defs.hpp"
#include "tiro/core/format.hpp"
#include "tiro/core/hash.hpp"
#include "tiro/core/not_null.hpp"
#include "tiro/ir/function.hpp"
#include "tiro/ir/fwd.hpp"
#include "tiro/semantics/symbol_table.hpp" // TODO fwd

#include <string_view>

namespace tiro {

/* [[[cog
    from codegen.unions import define
    from codegen.ir_gen import ComputedValueType
    define(ComputedValueType)
]]] */
enum class ComputedValueType : u8 {
    Constant,
    ModuleMemberId,
    UnaryOp,
    BinaryOp,
    AggregateMemberRead,
};

std::string_view to_string(ComputedValueType type);
// [[[end]]]

/* [[[cog
    from codegen.unions import define
    from codegen.ir_gen import ComputedValue
    define(ComputedValue)
]]] */
/// Represents a reusable local variable for a certain operation.
class ComputedValue final {
public:
    /// A known constant.
    using Constant = tiro::Constant;

    /// A cached read targeting a module member.
    /// Only makes sense for constant values.
    using ModuleMemberId = tiro::ModuleMemberId;

    /// The known result of a unary operation.
    struct UnaryOp final {
        /// The unary operator.
        UnaryOpType op;

        /// The operand value.
        LocalId operand;

        UnaryOp(const UnaryOpType& op_, const LocalId& operand_)
            : op(op_)
            , operand(operand_) {}
    };

    /// The known result of a binary operation.
    struct BinaryOp final {
        /// The binary operator.
        BinaryOpType op;

        /// The left operand.
        LocalId left;

        /// The right operand.
        LocalId right;

        BinaryOp(const BinaryOpType& op_, const LocalId& left_, const LocalId& right_)
            : op(op_)
            , left(left_)
            , right(right_) {}
    };

    /// A cached read access to an aggregate's member.
    struct AggregateMemberRead final {
        /// The aggregate instance.
        LocalId aggregate;

        /// The accessed member.
        AggregateMember member;

        AggregateMemberRead(const LocalId& aggregate_, const AggregateMember& member_)
            : aggregate(aggregate_)
            , member(member_) {}
    };

    static ComputedValue make_constant(const Constant& constant);
    static ComputedValue make_module_member_id(const ModuleMemberId& module_member_id);
    static ComputedValue make_unary_op(const UnaryOpType& op, const LocalId& operand);
    static ComputedValue
    make_binary_op(const BinaryOpType& op, const LocalId& left, const LocalId& right);
    static ComputedValue
    make_aggregate_member_read(const LocalId& aggregate, const AggregateMember& member);

    ComputedValue(Constant constant);
    ComputedValue(ModuleMemberId module_member_id);
    ComputedValue(UnaryOp unary_op);
    ComputedValue(BinaryOp binary_op);
    ComputedValue(AggregateMemberRead aggregate_member_read);

    ComputedValueType type() const noexcept { return type_; }

    void format(FormatStream& stream) const;

    void build_hash(Hasher& h) const;

    const Constant& as_constant() const;
    const ModuleMemberId& as_module_member_id() const;
    const UnaryOp& as_unary_op() const;
    const BinaryOp& as_binary_op() const;
    const AggregateMemberRead& as_aggregate_member_read() const;

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
    ComputedValueType type_;
    union {
        Constant constant_;
        ModuleMemberId module_member_id_;
        UnaryOp unary_op_;
        BinaryOp binary_op_;
        AggregateMemberRead aggregate_member_read_;
    };
};

bool operator==(const ComputedValue& lhs, const ComputedValue& rhs);
bool operator!=(const ComputedValue& lhs, const ComputedValue& rhs);
// [[[end]]]

/* [[[cog
    from codegen.unions import define
    from codegen.ir_gen import AssignTargetType
    define(AssignTargetType)
]]] */
enum class AssignTargetType : u8 {
    LValue,
    Symbol,
};

std::string_view to_string(AssignTargetType type);
// [[[end]]]

/* [[[cog
    from codegen.unions import define
    from codegen.ir_gen import AssignTarget
    define(AssignTarget)
]]] */
/// Represents the left hand side of an assignment during compilation.
class AssignTarget final {
public:
    /// An ir lvalue
    using LValue = tiro::LValue;

    /// Represents a symbol.
    using Symbol = tiro::SymbolId;

    static AssignTarget make_lvalue(const LValue& lvalue);
    static AssignTarget make_symbol(const Symbol& symbol);

    AssignTarget(LValue lvalue);
    AssignTarget(Symbol symbol);

    AssignTargetType type() const noexcept { return type_; }

    const LValue& as_lvalue() const;
    const Symbol& as_symbol() const;

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
    AssignTargetType type_;
    union {
        LValue lvalue_;
        Symbol symbol_;
    };
};
// [[[end]]]

/* [[[cog
    import cog
    from codegen.unions import implement_inlines
    from codegen.ir_gen import ComputedValue, AssignTarget
    implement_inlines(ComputedValue)
    cog.outl()
    implement_inlines(AssignTarget)
]]] */
template<typename Self, typename Visitor, typename... Args>
decltype(auto) ComputedValue::visit_impl(Self&& self, Visitor&& vis, Args&&... args) {
    switch (self.type()) {
    case ComputedValueType::Constant:
        return vis.visit_constant(self.constant_, std::forward<Args>(args)...);
    case ComputedValueType::ModuleMemberId:
        return vis.visit_module_member_id(self.module_member_id_, std::forward<Args>(args)...);
    case ComputedValueType::UnaryOp:
        return vis.visit_unary_op(self.unary_op_, std::forward<Args>(args)...);
    case ComputedValueType::BinaryOp:
        return vis.visit_binary_op(self.binary_op_, std::forward<Args>(args)...);
    case ComputedValueType::AggregateMemberRead:
        return vis.visit_aggregate_member_read(
            self.aggregate_member_read_, std::forward<Args>(args)...);
    }
    TIRO_UNREACHABLE("Invalid ComputedValue type.");
}

template<typename Self, typename Visitor, typename... Args>
decltype(auto) AssignTarget::visit_impl(Self&& self, Visitor&& vis, Args&&... args) {
    switch (self.type()) {
    case AssignTargetType::LValue:
        return vis.visit_lvalue(self.lvalue_, std::forward<Args>(args)...);
    case AssignTargetType::Symbol:
        return vis.visit_symbol(self.symbol_, std::forward<Args>(args)...);
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

#endif // TIRO_IR_GEN_SUPPORT_HPP
