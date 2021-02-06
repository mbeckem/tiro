#ifndef TIRO_COMPILER_IR_GEN_SUPPORT_HPP
#define TIRO_COMPILER_IR_GEN_SUPPORT_HPP

#include "common/adt/not_null.hpp"
#include "common/defs.hpp"
#include "common/format.hpp"
#include "common/hash.hpp"
#include "common/id_type.hpp"
#include "compiler/ir/function.hpp"
#include "compiler/ir/fwd.hpp"
#include "compiler/semantics/fwd.hpp"
#include "compiler/semantics/symbol_table.hpp"

#include "absl/container/inlined_vector.h"

#include <string_view>

namespace tiro::ir {

TIRO_DEFINE_ID(RegionId, u32)

/* [[[cog
    from codegen.unions import define
    from codegen.ir_gen import ComputedValue
    define(ComputedValue.tag)
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
/// Represents a reusable value defined by an instruction.
class ComputedValue final {
public:
    /// A known constant.
    using Constant = tiro::ir::Constant;

    /// A cached read targeting a module member.
    /// Only makes sense for constant values.
    using ModuleMemberId = tiro::ir::ModuleMemberId;

    /// The known result of a unary operation.
    struct UnaryOp final {
        /// The unary operator.
        UnaryOpType op;

        /// The operand value.
        InstId operand;

        UnaryOp(const UnaryOpType& op_, const InstId& operand_)
            : op(op_)
            , operand(operand_) {}
    };

    /// The known result of a binary operation.
    struct BinaryOp final {
        /// The binary operator.
        BinaryOpType op;

        /// The left operand.
        InstId left;

        /// The right operand.
        InstId right;

        BinaryOp(const BinaryOpType& op_, const InstId& left_, const InstId& right_)
            : op(op_)
            , left(left_)
            , right(right_) {}
    };

    /// A cached read access to an aggregate's member.
    struct AggregateMemberRead final {
        /// The aggregate instance.
        InstId aggregate;

        /// The accessed member.
        AggregateMember member;

        AggregateMemberRead(const InstId& aggregate_, const AggregateMember& member_)
            : aggregate(aggregate_)
            , member(member_) {}
    };

    static ComputedValue make_constant(const Constant& constant);
    static ComputedValue make_module_member_id(const ModuleMemberId& module_member_id);
    static ComputedValue make_unary_op(const UnaryOpType& op, const InstId& operand);
    static ComputedValue
    make_binary_op(const BinaryOpType& op, const InstId& left, const InstId& right);
    static ComputedValue
    make_aggregate_member_read(const InstId& aggregate, const AggregateMember& member);

    ComputedValue(Constant constant);
    ComputedValue(ModuleMemberId module_member_id);
    ComputedValue(UnaryOp unary_op);
    ComputedValue(BinaryOp binary_op);
    ComputedValue(AggregateMemberRead aggregate_member_read);

    ComputedValueType type() const noexcept { return type_; }

    void format(FormatStream& stream) const;

    void hash(Hasher& h) const;

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
    from codegen.ir_gen import AssignTarget
    define(AssignTarget.tag)
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
    using LValue = tiro::ir::LValue;

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
    from codegen.unions import define
    from codegen.ir_gen import Region
    define(Region.tag)
]]] */
enum class RegionType : u8 {
    Loop,
    Scope,
};

std::string_view to_string(RegionType type);
// [[[end]]]

/* [[[cog
    from codegen.unions import define
    from codegen.ir_gen import Region
    define(Region)
]]] */
/// Represents the data associated with a nested region.
///
/// NOTE: Make sure to only use (de-facto) noexcept movable types in this class.
class Region final {
public:
    /// Represents an active loop.
    struct Loop final {
        /// Target block for the `break` expression.
        BlockId jump_break;

        /// Target block for the `continue` expression.
        BlockId jump_continue;

        Loop(const BlockId& jump_break_, const BlockId& jump_continue_)
            : jump_break(jump_break_)
            , jump_continue(jump_continue_) {}
    };

    /// Represents a block scope.
    struct Scope final {
        /// The original exception handler when this scope was entered.
        BlockId original_handler;

        /// Signals already completed deferred executions to recursive scope exit invocations.
        /// This is important when nested control flow instructions are encountered while
        /// evaluating deferred statements.
        u32 processed;

        /// Deferred expressions that must be evaluated on normal (non-exceptional)
        /// scope exit, e.g. return or break.
        absl::InlinedVector<std::pair<NotNull<AstExpr*>, BlockId>, 3> deferred;

        Scope(const BlockId& original_handler_, const u32& processed_,
            absl::InlinedVector<std::pair<NotNull<AstExpr*>, BlockId>, 3> deferred_)
            : original_handler(original_handler_)
            , processed(processed_)
            , deferred(std::move(deferred_)) {}
    };

    static Region make_loop(const BlockId& jump_break, const BlockId& jump_continue);
    static Region make_scope(const BlockId& original_handler, const u32& processed,
        absl::InlinedVector<std::pair<NotNull<AstExpr*>, BlockId>, 3> deferred);

    Region(Loop loop);
    Region(Scope scope);

    ~Region();

    Region(Region&& other) noexcept;
    Region& operator=(Region&& other) noexcept;

    RegionType type() const noexcept { return type_; }

    const Loop& as_loop() const;
    Loop& as_loop();

    const Scope& as_scope() const;
    Scope& as_scope();

    template<typename Visitor, typename... Args>
    TIRO_FORCE_INLINE decltype(auto) visit(Visitor&& vis, Args&&... args) {
        return visit_impl(*this, std::forward<Visitor>(vis), std::forward<Args>(args)...);
    }

    template<typename Visitor, typename... Args>
    TIRO_FORCE_INLINE decltype(auto) visit(Visitor&& vis, Args&&... args) const {
        return visit_impl(*this, std::forward<Visitor>(vis), std::forward<Args>(args)...);
    }

private:
    void _destroy_value() noexcept;
    void _move_construct_value(Region& other) noexcept;
    void _move_assign_value(Region& other) noexcept;

    template<typename Self, typename Visitor, typename... Args>
    static TIRO_FORCE_INLINE decltype(auto) visit_impl(Self&& self, Visitor&& vis, Args&&... args);

private:
    RegionType type_;
    union {
        Loop loop_;
        Scope scope_;
    };
};
// [[[end]]]

/* [[[cog
    from cog import outl
    from codegen.unions import implement_inlines
    from codegen.ir_gen import ComputedValue, AssignTarget, Region
    for index, type in enumerate([ComputedValue, AssignTarget, Region]):
        if index > 0:
            outl()
        
        implement_inlines(type)
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

template<typename Self, typename Visitor, typename... Args>
decltype(auto) Region::visit_impl(Self&& self, Visitor&& vis, Args&&... args) {
    switch (self.type()) {
    case RegionType::Loop:
        return vis.visit_loop(self.loop_, std::forward<Args>(args)...);
    case RegionType::Scope:
        return vis.visit_scope(self.scope_, std::forward<Args>(args)...);
    }
    TIRO_UNREACHABLE("Invalid Region type.");
}
// [[[end]]]

} // namespace tiro::ir

TIRO_ENABLE_MEMBER_HASH(tiro::ir::ComputedValue)

TIRO_ENABLE_FREE_TO_STRING(tiro::ir::ComputedValueType)
TIRO_ENABLE_FREE_TO_STRING(tiro::ir::AssignTargetType)
TIRO_ENABLE_MEMBER_FORMAT(tiro::ir::ComputedValue)
TIRO_ENABLE_MEMBER_FORMAT(tiro::ir::AssignTarget)

#endif // TIRO_COMPILER_IR_GEN_SUPPORT_HPP
