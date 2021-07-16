#include "compiler/ir_passes/visit.hpp"

#include "compiler/ir/function.hpp"

namespace tiro::ir {

namespace {

/// Visits all insts referenced by the given objects. The provided callback
/// will be invoked for every encountered local id.
class LocalVisitor final {
public:
    explicit LocalVisitor(const Function& func, FunctionRef<void(InstId)> cb);

    LocalVisitor(const LocalVisitor&) = delete;
    LocalVisitor& operator=(const LocalVisitor&) = delete;

    void accept(const Block& block);
    void accept(const Terminator& term);
    void accept(const LValue& lvalue);
    void accept(const Aggregate& agg);
    void accept(const Value& value);
    void accept(const Inst& local);
    void accept(const Phi& phi);
    void accept(const LocalList& list);
    void accept(const Record& record);

private:
    void invoke(InstId local);

    void visit_list(LocalListId id);

private:
    const Function& func_;
    FunctionRef<void(InstId)> cb_;
};

} // namespace

LocalVisitor::LocalVisitor(const Function& func, FunctionRef<void(InstId)> cb)
    : func_(func)
    , cb_(cb) {
    TIRO_DEBUG_ASSERT(cb_, "Callback function must be valid.");
}

void LocalVisitor::accept(const Block& block) {
    for (const auto& inst : block.insts()) {
        invoke(inst);
        accept(func_[inst]);
    }
    accept(block.terminator());
}

void LocalVisitor::accept(const Terminator& term) {
    struct Visitor {
        LocalVisitor& self;

        void visit_none(const Terminator::None&) {}

        void visit_never(const Terminator::Never&) {}

        void visit_entry(const Terminator::Entry&) {}

        void visit_exit(const Terminator::Exit&) {}

        void visit_jump(const Terminator::Jump&) {}

        void visit_branch(const Terminator::Branch& b) { self.invoke(b.value); }

        void visit_return(const Terminator::Return& r) { self.invoke(r.value); }

        void visit_rethrow(const Terminator::Rethrow&) {}

        void visit_assert_fail(const Terminator::AssertFail& a) {
            self.invoke(a.expr);
            self.invoke(a.message);
        }
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

void LocalVisitor::accept(const Aggregate& agg) {
    struct Visitor {
        LocalVisitor& self;

        void visit_method(const Aggregate::Method& method) { self.invoke(method.instance); }

        void visit_iterator_next(const Aggregate::IteratorNext& iter) {
            self.invoke(iter.iterator);
        }
    };
    agg.visit(Visitor{*this});
}

void LocalVisitor::accept(const Value& value) {
    struct Visitor {
        LocalVisitor& self;

        void visit_read(const Value::Read& r) { self.accept(r.target); }

        void visit_write(const Value::Write& w) {
            self.accept(w.target);
            self.invoke(w.value);
        }

        void visit_alias(const Value::Alias& a) { self.invoke(a.target); }

        void visit_publish_assign(const Value::PublishAssign& p) { self.invoke(p.value); }

        void visit_phi(const Value::Phi& p) { self.accept(p); }

        void visit_observe_assign(const Value::ObserveAssign& o) { self.visit_list(o.operands); }

        void visit_constant(const Value::Constant&) {}

        void visit_outer_environment(const Value::OuterEnvironment&) {}

        void visit_binary_op(const Value::BinaryOp& b) {
            self.invoke(b.left);
            self.invoke(b.right);
        }

        void visit_unary_op(const Value::UnaryOp& u) { self.invoke(u.operand); }

        void visit_call(const Value::Call& c) {
            self.invoke(c.func);
            self.accept(self.func_[c.args]);
        }

        void visit_aggregate(const Value::Aggregate& a) { self.accept(a); }

        void visit_get_aggregate_member(const Value::GetAggregateMember& m) {
            self.invoke(m.aggregate);
        }

        void visit_method_call(const Value::MethodCall& m) {
            self.invoke(m.method);
            self.accept(self.func_[m.args]);
        }

        void visit_make_environment(const Value::MakeEnvironment& m) { self.invoke(m.parent); }

        void visit_make_closure(const Value::MakeClosure& m) { self.invoke(m.env); }

        void visit_make_iterator(const Value::MakeIterator& i) { self.invoke(i.container); }

        void visit_record(const Value::Record& r) { self.accept(self.func_[r.value]); }

        void visit_container(const Value::Container& c) { self.visit_list(c.args); }

        void visit_format(const Value::Format& f) { self.visit_list(f.args); }

        void visit_error(const Value::Error&) {}

        void visit_nop(const Value::Nop&) {}
    };
    value.visit(Visitor{*this});
}

void LocalVisitor::accept(const Inst& local) {
    accept(local.value());
}

void LocalVisitor::accept(const Phi& phi) {
    visit_list(phi.operands());
}

void LocalVisitor::accept(const LocalList& list) {
    for (const auto& op : list)
        invoke(op);
}

void LocalVisitor::accept(const Record& record) {
    for (const auto& [name, value] : record) {
        static_assert(std::is_same_v<decltype(name), const InternedString>);
        invoke(value);
    }
}

void LocalVisitor::invoke(InstId local) {
    TIRO_DEBUG_ASSERT(local, "Inst must be valid.");
    cb_(local);
}

void LocalVisitor::visit_list(LocalListId id) {
    if (id)
        accept(func_[id]);
}

void visit_insts(const Function& func, const Block& block, FunctionRef<void(InstId)> cb) {
    return LocalVisitor(func, cb).accept(block);
}

void visit_insts(const Function& func, const Terminator& term, FunctionRef<void(InstId)> cb) {
    return LocalVisitor(func, cb).accept(term);
}

void visit_insts(const Function& func, const LValue& lvalue, FunctionRef<void(InstId)> cb) {
    return LocalVisitor(func, cb).accept(lvalue);
}

void visit_insts(const Function& func, const Value& value, FunctionRef<void(InstId)> cb) {
    return LocalVisitor(func, cb).accept(value);
}

void visit_insts(const Function& func, const Inst& local, FunctionRef<void(InstId)> cb) {
    return LocalVisitor(func, cb).accept(local);
}

void visit_insts(const Function& func, const Phi& phi, FunctionRef<void(InstId)> cb) {
    return LocalVisitor(func, cb).accept(phi);
}

void visit_insts(const Function& func, const LocalList& list, FunctionRef<void(InstId)> cb) {
    return LocalVisitor(func, cb).accept(list);
}

void visit_inst_operands(const Function& func, InstId inst, FunctionRef<void(InstId)> cb) {
    const auto& inst_data = func[inst];
    visit_insts(func, inst_data.value(), cb);
}

} // namespace tiro::ir
