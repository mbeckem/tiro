#include "tiro/mir/used_locals.hpp"

#include "tiro/mir/types.hpp"

namespace tiro::compiler::mir_transform {

/// Returns true if the lvalue access (read or write) may produce
/// side effects (such as exceptions when accessing an array with an out of bounds index).
/// These lvalues may not be optimized out.
static bool has_side_effects(const mir::LValue& value) {
    switch (value.type()) {
    case mir::LValueType::Param:
    case mir::LValueType::Closure:
    case mir::LValueType::Module:
        return false;

    // Because we dont have type information, we cannot be sure
    // that a field actually exists. We should revisit this logic
    // once we have something resembling compile time type information.
    case mir::LValueType::Field:
    case mir::LValueType::TupleField:
    case mir::LValueType::Index:
        return true;
    }

    TIRO_UNREACHABLE("Invalid lvalue type.");
}

/// Returns true iff this rvalue may trigger side effects (such as exceptions).
/// RValues with side effects may not be optimized out.
///
/// TODO: The implementation is very conservative regarding unary and binary operators,
/// they can probably be optimized in some situations.
static bool
has_side_effects(const mir::RValue& value, const mir::Function& func) {
    struct Visitor {
        const mir::Function& func;

        bool visit_use_lvalue(const mir::RValue::UseLValue& u) {
            return has_side_effects(u.target);
        }

        bool visit_use_local(const mir::RValue::UseLocal&) { return false; }

        bool visit_phi(const mir::RValue::Phi&) { return false; }

        bool visit_phi0(const mir::RValue::Phi0&) { return false; }

        bool visit_constant(const mir::RValue::Constant&) { return false; }

        bool visit_outer_environment(const mir::RValue::OuterEnvironment&) {
            return false;
        }

        bool visit_binary_op(const mir::RValue::BinaryOp& b) {
            const auto lhs = func[b.left];
            const auto rhs = func[b.right];
            return lhs->value().type() != mir::RValueType::Constant
                   || rhs->value().type() != mir::RValueType::Constant;
        }

        bool visit_unary_op(const mir::RValue::UnaryOp& u) {
            const auto operand = func[u.operand];
            return operand->value().type() != mir::RValueType::Constant;
        }

        bool visit_call(const mir::RValue::Call&) { return true; }

        bool visit_method_handle(const mir::RValue::MethodHandle&) {
            // Might throw if method does not exist
            return true;
        }

        bool visit_method_call(const mir::RValue::MethodCall&) { return true; }

        bool visit_make_environment(const mir::RValue::MakeEnvironment&) {
            return false;
        }

        bool visit_make_closure(const mir::RValue::MakeClosure&) {
            return false;
        }

        bool visit_container(const mir::RValue::Container&) { return false; }

        bool visit_format(const mir::RValue::Format&) { return false; }
    };

    return value.visit(Visitor{func});
}

LocalVisitor::LocalVisitor(
    const mir::Function& func, FunctionRef<void(mir::LocalID)> cb)
    : func_(func)
    , cb_(cb) {
    TIRO_ASSERT(cb_, "Callback function must be valid.");
}

void LocalVisitor::accept(const mir::Block& block) {
    for (const auto& stmt : block.stmts())
        accept(stmt);
    accept(block.terminator());
}

void LocalVisitor::accept(const mir::Terminator& term) {
    struct Visitor {
        LocalVisitor& parent;

        void visit_none(const mir::Terminator::None&) {}

        void visit_jump(const mir::Terminator::Jump&) {}

        void visit_branch(const mir::Terminator::Branch& b) {
            parent.invoke(b.value);
        }

        void visit_return(const mir::Terminator::Return& r) {
            parent.invoke(r.value);
        }

        void visit_exit(const mir::Terminator::Exit&) {}

        void visit_assert_fail(const mir::Terminator::AssertFail& a) {
            parent.invoke(a.expr);
            parent.invoke(a.message);
        }

        void visit_never(const mir::Terminator::Never&) {}
    };

    term.visit(Visitor{*this});
}

void LocalVisitor::accept(const mir::LValue& lvalue) {
    struct Visitor {
        LocalVisitor& parent;

        void visit_param(const mir::LValue::Param&) {}

        void visit_closure(const mir::LValue::Closure& c) {
            parent.invoke(c.env);
        }

        void visit_module(const mir::LValue::Module&) {}

        void visit_field(const mir::LValue::Field& f) {
            parent.invoke(f.object);
        }

        void visit_tuple_field(const mir::LValue::TupleField& t) {
            parent.invoke(t.object);
        }

        void visit_index(const mir::LValue::Index& i) {
            parent.invoke(i.object);
            parent.invoke(i.index);
        }
    };
    lvalue.visit(Visitor{*this});
}

void LocalVisitor::accept(const mir::RValue& rvalue) {
    struct Visitor {
        LocalVisitor& parent;

        void visit_use_lvalue(const mir::RValue::UseLValue& u) {
            parent.accept(u.target);
        }

        void visit_use_local(const mir::RValue::UseLocal& u) {
            parent.invoke(u.target);
        }

        void visit_phi(const mir::RValue::Phi& p) {
            parent.accept(*parent.func_[p.value]);
        }

        void visit_phi0(const mir::RValue::Phi0&) {}

        void visit_constant(const mir::RValue::Constant&) {}

        void visit_outer_environment(const mir::RValue::OuterEnvironment&) {}

        void visit_binary_op(const mir::RValue::BinaryOp& b) {
            parent.invoke(b.left);
            parent.invoke(b.right);
        }

        void visit_unary_op(const mir::RValue::UnaryOp& u) {
            parent.invoke(u.operand);
        }

        void visit_call(const mir::RValue::Call& c) {
            parent.invoke(c.func);
            parent.accept(*parent.func_[c.args]);
        }

        void visit_method_handle(const mir::RValue::MethodHandle& m) {
            parent.invoke(m.instance);
        }

        void visit_method_call(const mir::RValue::MethodCall& m) {
            parent.invoke(m.method);
            parent.accept(*parent.func_[m.args]);
        }

        void visit_make_environment(const mir::RValue::MakeEnvironment& m) {
            parent.invoke(m.parent);
        }

        void visit_make_closure(const mir::RValue::MakeClosure& m) {
            parent.invoke(m.env);
            parent.invoke(m.func);
        }

        void visit_container(const mir::RValue::Container& c) {
            parent.accept(*parent.func_[c.args]);
        }

        void visit_format(const mir::RValue::Format& f) {
            parent.accept(*parent.func_[f.args]);
        }
    };
    rvalue.visit(Visitor{*this});
}

void LocalVisitor::accept(const mir::Local& local) {
    accept(local.value());
}

void LocalVisitor::accept(const mir::Phi& phi) {
    for (const auto& op : phi.operands())
        invoke(op);
}

void LocalVisitor::accept(const mir::LocalList& list) {
    for (const auto& op : list)
        invoke(op);
}

void LocalVisitor::accept(const mir::Stmt& stmt) {
    struct Visitor {
        LocalVisitor& parent;

        void visit_assign(const mir::Stmt::Assign& a) {
            parent.accept(a.target);
            parent.invoke(a.value);
        }

        void visit_define(const mir::Stmt::Define& d) {
            // The newly defined local is NOT treated as a use!
            // Only the right hand side of the definition is considered.
            parent.accept(*parent.func_[d.local]);
        }
    };

    stmt.visit(Visitor{*this});
}

void LocalVisitor::invoke(mir::LocalID local) {
    TIRO_ASSERT(local, "Local must be valid.");
    cb_(local);
}

// TODO: only consider blocks that are actually reachable in the cfg.
// TODO: expand to "remove dead variables" pass (the current implementation will keep
// locals alive that are only used by other dead locals).
void remove_unused_locals(mir::Function& func) {
    // TODO efficient containers
    std::unordered_set<mir::LocalID, UseHasher> used_locals;
    {
        const auto callback = [&](const mir::LocalID& local) {
            used_locals.emplace(local);
        };
        LocalVisitor visitor(func, callback);

        for (const auto id : func.block_ids()) {
            visitor.accept(*func[id]);
        }
    }

    for (const auto block_id : func.block_ids()) {
        auto& block = *func[block_id];
        block.remove_stmts([&](const mir::Stmt& stmt) {
            if (stmt.type() != mir::StmtType::Define)
                return false;

            auto id = stmt.as_define().local;
            auto local = func[id];

            // Side effects must be preserved.
            if (has_side_effects(local->value(), func))
                return false;

            // Unused locals without side effects can be removed.
            if (auto pos = used_locals.find(id); pos != used_locals.end())
                return false;

            return true;
        });
    }
}

} // namespace tiro::compiler::mir_transform
