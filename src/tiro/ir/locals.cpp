#include "tiro/ir/locals.hpp"

#include "tiro/ir/function.hpp"

namespace tiro {

namespace {

/// Visits all locals referenced by the given objects. The provided callback
/// will be invoked for every encountered local id.
// TODO: Needs unit tests to check that all locals are visited.
class LocalVisitor final {
public:
    explicit LocalVisitor(const Function& func, FunctionRef<void(LocalId)> cb);

    LocalVisitor(const LocalVisitor&) = delete;
    LocalVisitor& operator=(const LocalVisitor&) = delete;

    void accept(const Block& block);
    void accept(const Terminator& term);
    void accept(const LValue& lvalue);
    void accept(const RValue& rvalue);
    void accept(const Local& local);
    void accept(const Phi& phi);
    void accept(const LocalList& list);
    void accept(const Stmt& stmt);

private:
    void invoke(LocalId local);

private:
    const Function& func_;
    FunctionRef<void(LocalId)> cb_;
};

} // namespace

LocalVisitor::LocalVisitor(const Function& func, FunctionRef<void(LocalId)> cb)
    : func_(func)
    , cb_(cb) {
    TIRO_DEBUG_ASSERT(cb_, "Callback function must be valid.");
}

void LocalVisitor::accept(const Block& block) {
    for (const auto& stmt : block.stmts())
        accept(stmt);
    accept(block.terminator());
}

void LocalVisitor::accept(const Terminator& term) {
    struct Visitor {
        LocalVisitor& self;

        void visit_none(const Terminator::None&) {}

        void visit_jump(const Terminator::Jump&) {}

        void visit_branch(const Terminator::Branch& b) { self.invoke(b.value); }

        void visit_return(const Terminator::Return& r) { self.invoke(r.value); }

        void visit_exit(const Terminator::Exit&) {}

        void visit_assert_fail(const Terminator::AssertFail& a) {
            self.invoke(a.expr);
            self.invoke(a.message);
        }

        void visit_never(const Terminator::Never&) {}
    };

    term.visit(Visitor{*this});
}

void LocalVisitor::accept(const LValue& lvalue) {
    struct Visitor {
        LocalVisitor& self;

        void visit_param(const LValue::Param&) {}

        void visit_closure(const LValue::Closure& c) { self.invoke(c.env); }

        void visit_module(const LValue::Module&) {}

        void visit_field(const LValue::Field& f) { self.invoke(f.object); }

        void visit_tuple_field(const LValue::TupleField& t) { self.invoke(t.object); }

        void visit_index(const LValue::Index& i) {
            self.invoke(i.object);
            self.invoke(i.index);
        }
    };
    lvalue.visit(Visitor{*this});
}

void LocalVisitor::accept(const RValue& rvalue) {
    struct Visitor {
        LocalVisitor& self;

        void visit_use_lvalue(const RValue::UseLValue& u) { self.accept(u.target); }

        void visit_use_local(const RValue::UseLocal& u) { self.invoke(u.target); }

        void visit_phi(const RValue::Phi& p) { self.accept(*self.func_[p.value]); }

        void visit_phi0(const RValue::Phi0&) {}

        void visit_constant(const RValue::Constant&) {}

        void visit_outer_environment(const RValue::OuterEnvironment&) {}

        void visit_binary_op(const RValue::BinaryOp& b) {
            self.invoke(b.left);
            self.invoke(b.right);
        }

        void visit_unary_op(const RValue::UnaryOp& u) { self.invoke(u.operand); }

        void visit_call(const RValue::Call& c) {
            self.invoke(c.func);
            self.accept(*self.func_[c.args]);
        }

        void visit_method_handle(const RValue::MethodHandle& m) { self.invoke(m.instance); }

        void visit_method_call(const RValue::MethodCall& m) {
            self.invoke(m.method);
            self.accept(*self.func_[m.args]);
        }

        void visit_make_environment(const RValue::MakeEnvironment& m) { self.invoke(m.parent); }

        void visit_make_closure(const RValue::MakeClosure& m) {
            self.invoke(m.env);
            self.invoke(m.func);
        }

        void visit_container(const RValue::Container& c) { self.accept(*self.func_[c.args]); }

        void visit_format(const RValue::Format& f) { self.accept(*self.func_[f.args]); }
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
        LocalVisitor& self;

        void visit_assign(const Stmt::Assign& a) {
            self.accept(a.target);
            self.invoke(a.value);
        }

        void visit_define(const Stmt::Define& d) {
            self.invoke(d.local);
            self.accept(*self.func_[d.local]);
        }
    };

    stmt.visit(Visitor{*this});
}

void LocalVisitor::invoke(LocalId local) {
    TIRO_DEBUG_ASSERT(local, "Local must be valid.");
    cb_(local);
}

void visit_locals(const Function& func, const Block& block, FunctionRef<void(LocalId)> cb) {
    return LocalVisitor(func, cb).accept(block);
}

void visit_locals(const Function& func, const Terminator& term, FunctionRef<void(LocalId)> cb) {
    return LocalVisitor(func, cb).accept(term);
}

void visit_locals(const Function& func, const LValue& lvalue, FunctionRef<void(LocalId)> cb) {
    return LocalVisitor(func, cb).accept(lvalue);
}

void visit_locals(const Function& func, const RValue& rvalue, FunctionRef<void(LocalId)> cb) {
    return LocalVisitor(func, cb).accept(rvalue);
}

void visit_locals(const Function& func, const Local& local, FunctionRef<void(LocalId)> cb) {
    return LocalVisitor(func, cb).accept(local);
}

void visit_locals(const Function& func, const Phi& phi, FunctionRef<void(LocalId)> cb) {
    return LocalVisitor(func, cb).accept(phi);
}

void visit_locals(const Function& func, const LocalList& list, FunctionRef<void(LocalId)> cb) {
    return LocalVisitor(func, cb).accept(list);
}

void visit_locals(const Function& func, const Stmt& stmt, FunctionRef<void(LocalId)> cb) {
    return LocalVisitor(func, cb).accept(stmt);
}

void visit_definitions(
    [[maybe_unused]] const Function& func, const Stmt& stmt, FunctionRef<void(LocalId)> cb) {
    if (stmt.type() != StmtType::Define)
        return;

    cb(stmt.as_define().local);
}

void visit_uses(const Function& func, const Stmt& stmt, FunctionRef<void(LocalId)> cb) {
    if (stmt.type() == StmtType::Define) {
        auto local = func[stmt.as_define().local];
        visit_locals(func, local->value(), cb);
    } else {
        visit_locals(func, stmt, cb);
    }
}

} // namespace tiro
