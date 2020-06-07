#include "tiro/ir/dead_code_elimination.hpp"

#include "tiro/ir/function.hpp"
#include "tiro/ir/locals.hpp"
#include "tiro/ir/traversal.hpp"

namespace tiro {

/// Returns true if the lvalue access (read or write) may produce
/// side effects (such as exceptions when accessing an array with an out of bounds index).
/// These lvalues may not be optimized out.
static bool has_side_effects(const LValue& value) {
    switch (value.type()) {
    case LValueType::Param:
    case LValueType::Closure:
    case LValueType::Module:
        return false;

    // Because we dont have type information, we cannot be sure
    // that a field actually exists. We should revisit this logic
    // once we have something resembling compile time type information.
    case LValueType::Field:
    case LValueType::TupleField:
    case LValueType::Index:
        return true;
    }

    TIRO_UNREACHABLE("Invalid lvalue type.");
}

/// Returns true iff this rvalue may trigger side effects (such as exceptions).
/// RValues with side effects may not be optimized out.
///
/// TODO: The implementation is very conservative regarding unary and binary operators,
/// they can probably be optimized in some situations.
static bool has_side_effects(const RValue& value, const Function& func) {
    struct Visitor {
        const Function& func;

        bool visit_use_lvalue(const RValue::UseLValue& u) { return has_side_effects(u.target); }

        bool visit_use_local(const RValue::UseLocal&) { return false; }

        bool visit_phi(const RValue::Phi&) { return false; }

        bool visit_phi0(const RValue::Phi0&) { return false; }

        bool visit_constant(const RValue::Constant&) { return false; }

        bool visit_outer_environment(const RValue::OuterEnvironment&) { return false; }

        bool visit_binary_op(const RValue::BinaryOp& b) {
            const auto lhs = func[b.left];
            const auto rhs = func[b.right];
            return lhs->value().type() != RValueType::Constant
                   || rhs->value().type() != RValueType::Constant;
        }

        bool visit_unary_op(const RValue::UnaryOp& u) {
            const auto operand = func[u.operand];
            return operand->value().type() != RValueType::Constant;
        }

        bool visit_call(const RValue::Call&) { return true; }

        bool visit_aggregate(const RValue::Aggregate& agg) {
            switch (agg.type()) {
            case AggregateType::Method:
                // Might throw if method does not exist
                return true;
            }

            TIRO_UNREACHABLE("Invalid aggregate type.");
        }

        bool visit_get_aggregate_member(const RValue::GetAggregateMember&) { return false; }

        bool visit_method_call(const RValue::MethodCall&) { return true; }

        bool visit_make_environment(const RValue::MakeEnvironment&) { return false; }

        bool visit_make_closure(const RValue::MakeClosure&) { return false; }

        bool visit_container(const RValue::Container&) { return false; }

        bool visit_format(const RValue::Format&) { return false; }
    };

    return value.visit(Visitor{func});
}

void eliminate_dead_code(Function& func) {
    IndexMap<bool, IdMapper<LocalId>> used_locals;
    used_locals.resize(func.local_count(), false);

    std::vector<LocalId> stack;
    const auto visit = [&](LocalId local) {
        if (!used_locals[local]) {
            used_locals[local] = true;
            stack.push_back(local);
        }
    };
    auto trace = [&](const auto& item) { visit_locals(func, item, visit); };

    // Find all locals that must not be eliminated (observable side effects).
    for (auto block_id : PreorderTraversal(func)) {
        auto block = func[block_id];

        for (const auto& stmt : block->stmts()) {
            switch (stmt.type()) {
            // Assignments are side effects, the rhs must be preserved.
            case StmtType::Assign:
                trace(stmt);
                break;

            case StmtType::Define: {
                auto local_id = stmt.as_define().local;
                auto local = func[local_id];

                if (has_side_effects(local->value(), func))
                    visit(local_id);
                break;
            }
            }
        }

        trace(block->terminator());
    }

    // All locals reachable through needed locals must be marked as "used" as well.
    while (!stack.empty()) {
        auto local_id = stack.back();
        stack.pop_back();

        auto local = func[local_id];
        trace(*local);
    }

    // Clear everything that has not been marked as "used".
    for (const auto block_id : PreorderTraversal(func)) {
        auto block = func[block_id];

        block->remove_stmts([&](const Stmt& stmt) {
            if (stmt.type() != StmtType::Define)
                return false;

            auto local_id = stmt.as_define().local;
            return !static_cast<bool>(used_locals[local_id]);
        });
    }
}

} // namespace tiro
