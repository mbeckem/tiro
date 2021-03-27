#include "compiler/ir_passes/dead_code_elimination.hpp"

#include "common/entities/entity_storage.hpp"
#include "compiler/ir/function.hpp"
#include "compiler/ir/traversal.hpp"
#include "compiler/ir_passes/visit.hpp"

namespace tiro::ir {

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

/// Returns true iff this value may trigger side effects (such as exceptions).
/// Values with side effects may not be optimized out.
///
/// TODO: The implementation is very conservative regarding unary and binary operators,
/// they can probably be optimized in some situations.
static bool has_side_effects(const Value& value, const Function& func) {
    struct Visitor {
        const Function& func;

        bool visit_read(const Value::Read& r) { return has_side_effects(r.target); }

        bool visit_write(const Value::Write&) { return true; }

        bool visit_alias(const Value::Alias&) { return false; }

        // These instructions are only kept when actually used by an exception handler.
        bool visit_publish_assign(const Value::PublishAssign&) { return false; }

        bool visit_phi(const Value::Phi&) { return false; }

        bool visit_observe_assign(const Value::ObserveAssign&) { return false; }

        bool visit_constant(const Value::Constant&) { return false; }

        bool visit_outer_environment(const Value::OuterEnvironment&) { return false; }

        bool visit_binary_op(const Value::BinaryOp& b) {
            const auto& lhs = func[b.left];
            const auto& rhs = func[b.right];
            return lhs.value().type() != ValueType::Constant
                   || rhs.value().type() != ValueType::Constant;
        }

        bool visit_unary_op(const Value::UnaryOp& u) {
            const auto& operand = func[u.operand];
            return operand.value().type() != ValueType::Constant;
        }

        bool visit_call(const Value::Call&) { return true; }

        bool visit_aggregate(const Value::Aggregate& agg) {
            switch (agg.type()) {
            case AggregateType::Method:
                // Might throw if method does not exist
                return true;
            case AggregateType::IteratorNext:
                return true;
            }

            TIRO_UNREACHABLE("Invalid aggregate type.");
        }

        bool visit_get_aggregate_member(const Value::GetAggregateMember&) { return false; }

        bool visit_method_call(const Value::MethodCall&) { return true; }

        bool visit_make_environment(const Value::MakeEnvironment&) { return false; }

        bool visit_make_closure(const Value::MakeClosure&) { return false; }

        bool visit_make_iterator(const Value::MakeIterator&) { return true; }

        bool visit_record(const Value::Record&) { return false; }

        bool visit_container(const Value::Container&) { return false; }

        bool visit_format(const Value::Format&) { return false; }

        bool visit_error(const Value::Error&) {
            // Do NOT optimize away error values.
            return true;
        }

        bool visit_nop(const Value::Nop&) { return false; }
    };

    return value.visit(Visitor{func});
}

void eliminate_dead_code(Function& func) {
    EntityStorage<bool, InstId> used_insts;
    used_insts.resize(func.inst_count(), false);

    std::vector<InstId> stack;
    const auto visit = [&](InstId inst_id) {
        if (!used_insts[inst_id]) {
            used_insts[inst_id] = true;
            stack.push_back(inst_id);
        }
    };
    auto trace = [&](const auto& item) { visit_insts(func, item, visit); };

    // Find all instructions that must not be eliminated (observable side effects).
    for (auto block_id : PreorderTraversal(func)) {
        const auto& block = func[block_id];

        for (const auto& inst_id : block.insts()) {
            const auto& inst = func[inst_id];
            if (has_side_effects(inst.value(), func))
                visit(inst_id);
        }

        trace(block.terminator());
    }

    // All instructions reachable through needed instructions must be marked as "used" as well.
    while (!stack.empty()) {
        auto inst_id = stack.back();
        stack.pop_back();

        trace(func[inst_id]);
    }

    // Clear everything that has not been marked as "used".
    for (const auto block_id : PreorderTraversal(func)) {
        auto& block = func[block_id];
        block.remove_insts([&](InstId inst_Id) { return !static_cast<bool>(used_insts[inst_Id]); });
    }
}

} // namespace tiro::ir
