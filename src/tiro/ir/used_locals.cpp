#include "tiro/ir/used_locals.hpp"

#include "tiro/ir/traversal.hpp"
#include "tiro/ir/types.hpp"

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

        bool visit_use_lvalue(const RValue::UseLValue& u) {
            return has_side_effects(u.target);
        }

        bool visit_use_local(const RValue::UseLocal&) { return false; }

        bool visit_phi(const RValue::Phi&) { return false; }

        bool visit_phi0(const RValue::Phi0&) { return false; }

        bool visit_constant(const RValue::Constant&) { return false; }

        bool visit_outer_environment(const RValue::OuterEnvironment&) {
            return false;
        }

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

        bool visit_method_handle(const RValue::MethodHandle&) {
            // Might throw if method does not exist
            return true;
        }

        bool visit_method_call(const RValue::MethodCall&) { return true; }

        bool visit_make_environment(const RValue::MakeEnvironment&) {
            return false;
        }

        bool visit_make_closure(const RValue::MakeClosure&) { return false; }

        bool visit_container(const RValue::Container&) { return false; }

        bool visit_format(const RValue::Format&) { return false; }
    };

    return value.visit(Visitor{func});
}

LocalVisitor::LocalVisitor(const Function& func, FunctionRef<void(LocalID)> cb)
    : func_(func)
    , cb_(cb) {
    TIRO_ASSERT(cb_, "Callback function must be valid.");
}

void LocalVisitor::accept(const Block& block) {
    for (const auto& stmt : block.stmts())
        accept(stmt);
    accept(block.terminator());
}

void LocalVisitor::accept(const Terminator& term) {
    struct Visitor {
        LocalVisitor& parent;

        void visit_none(const Terminator::None&) {}

        void visit_jump(const Terminator::Jump&) {}

        void visit_branch(const Terminator::Branch& b) {
            parent.invoke(b.value);
        }

        void visit_return(const Terminator::Return& r) {
            parent.invoke(r.value);
        }

        void visit_exit(const Terminator::Exit&) {}

        void visit_assert_fail(const Terminator::AssertFail& a) {
            parent.invoke(a.expr);
            parent.invoke(a.message);
        }

        void visit_never(const Terminator::Never&) {}
    };

    term.visit(Visitor{*this});
}

void LocalVisitor::accept(const LValue& lvalue) {
    struct Visitor {
        LocalVisitor& parent;

        void visit_param(const LValue::Param&) {}

        void visit_closure(const LValue::Closure& c) { parent.invoke(c.env); }

        void visit_module(const LValue::Module&) {}

        void visit_field(const LValue::Field& f) { parent.invoke(f.object); }

        void visit_tuple_field(const LValue::TupleField& t) {
            parent.invoke(t.object);
        }

        void visit_index(const LValue::Index& i) {
            parent.invoke(i.object);
            parent.invoke(i.index);
        }
    };
    lvalue.visit(Visitor{*this});
}

void LocalVisitor::accept(const RValue& rvalue) {
    struct Visitor {
        LocalVisitor& parent;

        void visit_use_lvalue(const RValue::UseLValue& u) {
            parent.accept(u.target);
        }

        void visit_use_local(const RValue::UseLocal& u) {
            parent.invoke(u.target);
        }

        void visit_phi(const RValue::Phi& p) {
            parent.accept(*parent.func_[p.value]);
        }

        void visit_phi0(const RValue::Phi0&) {}

        void visit_constant(const RValue::Constant&) {}

        void visit_outer_environment(const RValue::OuterEnvironment&) {}

        void visit_binary_op(const RValue::BinaryOp& b) {
            parent.invoke(b.left);
            parent.invoke(b.right);
        }

        void visit_unary_op(const RValue::UnaryOp& u) {
            parent.invoke(u.operand);
        }

        void visit_call(const RValue::Call& c) {
            parent.invoke(c.func);
            parent.accept(*parent.func_[c.args]);
        }

        void visit_method_handle(const RValue::MethodHandle& m) {
            parent.invoke(m.instance);
        }

        void visit_method_call(const RValue::MethodCall& m) {
            parent.invoke(m.method);
            parent.accept(*parent.func_[m.args]);
        }

        void visit_make_environment(const RValue::MakeEnvironment& m) {
            parent.invoke(m.parent);
        }

        void visit_make_closure(const RValue::MakeClosure& m) {
            parent.invoke(m.env);
            parent.invoke(m.func);
        }

        void visit_container(const RValue::Container& c) {
            parent.accept(*parent.func_[c.args]);
        }

        void visit_format(const RValue::Format& f) {
            parent.accept(*parent.func_[f.args]);
        }
    };
    rvalue.visit(Visitor{*this});
}

void LocalVisitor::accept(const Local& local) {
    accept(local.value());
}

void LocalVisitor::accept(const Phi& phi) {
    for (const auto& op : phi.operands())
        invoke(op);
}

void LocalVisitor::accept(const LocalList& list) {
    for (const auto& op : list)
        invoke(op);
}

void LocalVisitor::accept(const Stmt& stmt) {
    struct Visitor {
        LocalVisitor& parent;

        void visit_assign(const Stmt::Assign& a) {
            parent.accept(a.target);
            parent.invoke(a.value);
        }

        void visit_define(const Stmt::Define& d) {
            parent.invoke(d.local);
            parent.accept(*parent.func_[d.local]);
        }
    };

    stmt.visit(Visitor{*this});
}

void LocalVisitor::invoke(LocalID local) {
    TIRO_ASSERT(local, "Local must be valid.");
    cb_(local);
}

void remove_unused_locals(Function& func) {
    IndexMap<bool, IDMapper<LocalID>> used_locals;
    used_locals.resize(func.local_count(), false);

    std::vector<LocalID> stack;
    const auto visit = [&](LocalID local) {
        if (!used_locals[local]) {
            used_locals[local] = true;
            stack.push_back(local);
        }
    };
    LocalVisitor tracer(func, visit);

    // Find all locals that must not be eliminated (observable side effects).
    for (auto block_id : PreorderTraversal(func)) {
        auto block = func[block_id];

        for (const auto& stmt : block->stmts()) {
            switch (stmt.type()) {
            // Assignments are side effects, the rhs must be preserved.
            case StmtType::Assign:
                tracer.accept(stmt);
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

        tracer.accept(block->terminator());
    }

    // All locals reachable through needed locals must be marked as "used" as well.
    while (!stack.empty()) {
        auto local_id = stack.back();
        stack.pop_back();

        auto local = func[local_id];
        tracer.accept(*local);
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
