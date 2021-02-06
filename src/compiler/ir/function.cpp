#include "compiler/ir/function.hpp"

#include "common/text/string_utils.hpp"
#include "compiler/ir/traversal.hpp"
#include "compiler/utils.hpp"

#include <cmath>
#include <type_traits>

namespace tiro::ir {

template<typename ID, typename Vec>
static bool check_id(const ID& id, const Vec& vec) {
    return id && id.value() < vec.size();
}

std::string_view to_string(FunctionType type) {
    switch (type) {
    case FunctionType::Normal:
        return "Normal";
    case FunctionType::Closure:
        return "Closure";
    }
    TIRO_UNREACHABLE("Invalid function type.");
}

Function::Function(InternedString name, FunctionType type, StringTable& strings)
    : strings_(TIRO_NN(&strings))
    , name_(name)
    , type_(type) {
    entry_ = make(Block(strings.insert("entry")));
    body_ = make(Block(strings.insert("body")));
    exit_ = make(Block(strings.insert("exit")));

    auto entry = (*this)[entry_];
    entry->terminator(Terminator::make_entry(body_, {}));

    auto body = (*this)[body_];
    body->append_predecessor(entry_);

    auto exit = (*this)[exit_];
    exit->terminator(Terminator::make_exit());
}

Function::~Function() {}

BlockId Function::make(Block&& block) {
    return blocks_.push_back(std::move(block));
}

ParamId Function::make(const Param& param) {
    return params_.push_back(param);
}

InstId Function::make(Inst&& inst) {
    return insts_.push_back(std::move(inst));
}

LocalListId Function::make(LocalList&& local_list) {
    return local_lists_.push_back(std::move(local_list));
}

RecordId Function::make(Record&& record) {
    return records_.push_back(std::move(record));
}

BlockId Function::entry() const {
    return entry_;
}

BlockId Function::body() const {
    return body_;
}

BlockId Function::exit() const {
    return exit_;
}

NotNull<IndexMapPtr<Block>> Function::operator[](BlockId id) {
    TIRO_DEBUG_ASSERT(check_id(id, blocks_), "Invalid block id.");
    return TIRO_NN(blocks_.ptr_to(id));
}

NotNull<IndexMapPtr<Param>> Function::operator[](ParamId id) {
    TIRO_DEBUG_ASSERT(check_id(id, params_), "Invalid param id.");
    return TIRO_NN(params_.ptr_to(id));
}

NotNull<IndexMapPtr<Inst>> Function::operator[](InstId id) {
    TIRO_DEBUG_ASSERT(check_id(id, insts_), "Invalid instruction id.");
    return TIRO_NN(insts_.ptr_to(id));
}

NotNull<IndexMapPtr<LocalList>> Function::operator[](LocalListId id) {
    TIRO_DEBUG_ASSERT(check_id(id, local_lists_), "Invalid local list id.");
    return TIRO_NN(local_lists_.ptr_to(id));
}

NotNull<IndexMapPtr<Record>> Function::operator[](RecordId id) {
    TIRO_DEBUG_ASSERT(check_id(id, local_lists_), "Invalid record id.");
    return TIRO_NN(records_.ptr_to(id));
}

NotNull<IndexMapPtr<const Block>> Function::operator[](BlockId id) const {
    TIRO_DEBUG_ASSERT(check_id(id, blocks_), "Invalid block id.");
    return TIRO_NN(blocks_.ptr_to(id));
}

NotNull<IndexMapPtr<const Param>> Function::operator[](ParamId id) const {
    TIRO_DEBUG_ASSERT(check_id(id, params_), "Invalid param id.");
    return TIRO_NN(params_.ptr_to(id));
}

NotNull<IndexMapPtr<const Inst>> Function::operator[](InstId id) const {
    TIRO_DEBUG_ASSERT(check_id(id, insts_), "Invalid instruction id.");
    return TIRO_NN(insts_.ptr_to(id));
}

NotNull<IndexMapPtr<const LocalList>> Function::operator[](LocalListId id) const {
    TIRO_DEBUG_ASSERT(check_id(id, local_lists_), "Invalid local list id.");
    return TIRO_NN(local_lists_.ptr_to(id));
}

NotNull<IndexMapPtr<const Record>> Function::operator[](RecordId id) const {
    TIRO_DEBUG_ASSERT(check_id(id, records_), "Invalid record id.");
    return TIRO_NN(records_.ptr_to(id));
}

void dump_function(const Function& func, FormatStream& stream) {
    using namespace dump_helpers;

    StringTable& strings = func.strings();

    stream.format(
        "Function\n"
        "  Name: {}\n"
        "  Type: {}\n",
        strings.dump(func.name()), func.type());

    // Walk the control flow graph
    stream.format("\n");
    for (auto block_id : ReversePostorderTraversal(func)) {
        if (block_id != func.entry())
            stream.format("\n");

        auto block = func[block_id];

        stream.format("{} (sealed: {}, filled: {})\n", dump(func, block_id), block->sealed(),
            block->filled());

        if (block->predecessor_count() > 0) {
            stream.format("  <- ");
            {
                size_t index = 0;
                for (auto pred : block->predecessors()) {
                    if (index++ != 0)
                        stream.format(", ");
                    stream.format("{}", dump(func, pred));
                }
            }
            stream.format("\n");
        }

        if (block->handler()) {
            stream.format("  handler: {}\n", dump(func, block->handler()));
        }

        const size_t stmt_count = block->inst_count();
        const size_t max_index_length = fmt::formatted_size(
            "{}", stmt_count == 0 ? 0 : stmt_count - 1);

        size_t index = 0;
        for (const auto& inst : block->insts()) {
            stream.format("  {index:>{width}}: {value}", fmt::arg("index", index),
                fmt::arg("width", max_index_length),
                fmt::arg("value", dump(func, Definition{inst})));

            stream.format("\n");
            ++index;
        }
        stream.format("  {}\n", dump(func, block->terminator()));
    }
}

LocalList::LocalList() {}

LocalList::LocalList(std::initializer_list<InstId> locals)
    : locals_(locals) {}

LocalList::LocalList(Storage&& locals)
    : locals_(std::move(locals)) {}

LocalList::~LocalList() {}

namespace dump_helpers {

void format(const Dump<BlockId>& d, FormatStream& stream) {
    auto block_id = d.value;
    if (!block_id) {
        stream.format("<INVALID>");
        return;
    }

    auto& func = d.parent;
    auto block = func[block_id];

    if (block->label()) {
        stream.format("${}-{}", func.strings().value(block->label()), block_id.value());
    } else {
        stream.format("${}", block_id.value());
    }
}

void format(const Dump<Definition>& d, FormatStream& stream) {
    auto inst_id = d.value.inst;
    if (!inst_id) {
        stream.format("<INVALID>");
        return;
    }

    auto& func = d.parent;
    auto inst = func[inst_id];
    stream.format("{} = {}", dump(func, inst_id), dump(func, inst->value()));
}

void format(const Dump<const Terminator&>& d, FormatStream& stream) {
    struct Visitor {
        const Function& func;
        FormatStream& stream;

        void visit_none([[maybe_unused]] const Terminator::None& none) { stream.format("-> none"); }

        void visit_never(const Terminator::Never& never) {
            stream.format("-> never {}", dump(func, never.target));
        }

        void visit_jump(const Terminator::Jump& jump) {
            stream.format("-> jump {}", dump(func, jump.target));
        }

        void visit_branch(const Terminator::Branch& branch) {
            stream.format("-> branch {} {} target: {} fallthrough: {}", branch.type,
                dump(func, branch.value), dump(func, branch.target),
                dump(func, branch.fallthrough));
        }

        void visit_return(const Terminator::Return& ret) {
            stream.format("-> return {} target: {}", dump(func, ret.value), dump(func, ret.target));
        }

        void visit_rethrow(const Terminator::Rethrow& rethrow) {
            stream.format("-> rethrow target: {}", dump(func, rethrow.target));
        }

        void visit_assert_fail(const Terminator::AssertFail& fail) {
            stream.format("-> assert fail expr: {} message: {} target: {}", dump(func, fail.expr),
                dump(func, fail.message), dump(func, fail.target));
        }

        void visit_entry(const Terminator::Entry& entry) {
            stream.format("-> body: {}, handlers: ", dump(func, entry.body));

            bool first = true;
            for (const auto& handler : entry.handlers) {
                if (!first)
                    stream.format(", ");

                first = false;
                stream.format("{}", dump(func, handler));
            }
        }

        void visit_exit([[maybe_unused]] const Terminator::Exit& exit) { stream.format("-> exit"); }
    };
    Visitor visitor{d.parent, stream};
    d.value.visit(visitor);
}

void format(const Dump<LValue>& d, FormatStream& stream) {
    struct Visitor {
        const Function& func;
        FormatStream& stream;

        void visit_param(const LValue::Param& param) {
            stream.format("<param {}>", param.target.value());
        }

        void visit_closure(const LValue::Closure& closure) {
            stream.format("<closure {} level: {} index: {}>", dump(func, closure.env),
                closure.levels, closure.index);
        }

        void visit_module(const LValue::Module& module) {
            stream.format("<module {}>", module.member.value());
        }

        void visit_field(const LValue::Field& field) {
            stream.format("{}.{}", dump(func, field.object), func.strings().dump(field.name));
        }

        void visit_tuple_field(const LValue::TupleField& field) {
            stream.format("{}.{}", dump(func, field.object), field.index);
        }

        void visit_index(const LValue::Index& index) {
            stream.format("{}[{}]", dump(func, index.object), dump(func, index.index));
        }
    };
    Visitor visitor{d.parent, stream};
    d.value.visit(visitor);
}

void format(const Dump<Phi>& d, FormatStream& stream) {
    auto& func = d.parent;
    auto& phi = d.value;

    stream.format("<phi");

    auto list_id = phi.operands();
    if (list_id) {
        auto list = func[list_id];
        for (const auto& op : *list) {
            stream.format(" {}", dump(func, op));
        }
    }

    stream.format(">");
}

void format(const Dump<Constant>& d, FormatStream& stream) {
    struct Visitor {
        const Function& func;
        FormatStream& stream;

        void visit_integer(const Constant::Integer& i) { stream.format("{}", i.value); }

        void visit_float(const Constant::Float& f) { stream.format("{:f}", f.value); }

        void visit_string(const Constant::String& str) {
            if (!str.value) {
                stream.format("\"\"");
                return;
            }

            auto escaped = escape_string(func.strings().value(str.value));
            stream.format("\"{}\"", escaped);
        }

        void visit_symbol(const Constant::Symbol& sym) {
            stream.format("#{}", func.strings().dump(sym.value));
        }

        void visit_null([[maybe_unused]] const Constant::Null& null) { stream.format("null"); }

        void visit_true([[maybe_unused]] const Constant::True& t) { stream.format("true"); }

        void visit_false([[maybe_unused]] const Constant::False& f) { stream.format("false"); }
    };
    Visitor visitor{d.parent, stream};
    d.value.visit(visitor);
}

void format(const Dump<Aggregate>& d, FormatStream& stream) {
    struct Visitor {
        const Function& func;
        FormatStream& stream;

        void visit_method(const Aggregate::Method& m) {
            stream.format(
                "<method {}.{}>", dump(func, m.instance), func.strings().dump(m.function));
        }

        void visit_iterator_next(const Aggregate::IteratorNext& i) {
            stream.format("<iterator-next {}>", dump(func, i.iterator));
        }
    };
    Visitor visitor{d.parent, stream};
    d.value.visit(visitor);
}

void format(const Dump<const Value&>& d, FormatStream& stream) {
    struct Visitor {
        const Function& func;
        FormatStream& stream;

        void visit_read(const Value::Read& r) { return format(dump(func, r.target), stream); }

        void visit_write(const Value::Write& write) {
            stream.format("<write {} {}>", dump(func, write.target), dump(func, write.value));
        }

        void visit_alias(const Value::Alias& a) { return format(dump(func, a.target), stream); }

        void visit_phi(const Value::Phi& phi) { return format(dump(func, phi.operands()), stream); }

        void visit_observe_assign(const Value::ObserveAssign& obs) {
            stream.format("<observe-assign {} {}>", obs.symbol, dump(func, obs.operands));
        }

        void visit_publish_assign(const Value::PublishAssign& p) {
            stream.format("<publish-assign {} {}>", p.symbol, dump(func, p.value));
        }

        void visit_constant(const Value::Constant& constant) {
            return format(dump(func, constant), stream);
        }

        void visit_outer_environment([[maybe_unused]] const Value::OuterEnvironment& outer) {
            stream.format("<outer-env>");
        }

        void visit_binary_op(const Value::BinaryOp& binop) {
            stream.format("{} {} {}", dump(func, binop.left), binop.op, dump(func, binop.right));
        }

        void visit_unary_op(const Value::UnaryOp& unop) {
            stream.format("{} {} ", unop.op, dump(func, unop.operand));
        }

        void visit_call(const Value::Call& call) {
            stream.format("{}({})", dump(func, call.func), dump(func, call.args));
        }

        void visit_aggregate(const Value::Aggregate& agg) {
            return format(dump(func, agg), stream);
        }

        void visit_get_aggregate_member(const Value::GetAggregateMember& get) {
            stream.format("<get-aggregate-member {} {}>", dump(func, get.aggregate), get.member);
        }

        void visit_method_call(const Value::MethodCall& call) {
            stream.format("{}({})", dump(func, call.method), dump(func, call.args));
        }

        void visit_make_environment(const Value::MakeEnvironment& env) {
            stream.format("<make-env {} {}>", dump(func, env.parent), env.size);
        }

        void visit_make_closure(const Value::MakeClosure& closure) {
            stream.format("<make-closure env: {} func: {}>", dump(func, closure.env),
                dump(func, closure.func));
        }

        void visit_make_iterator(const Value::MakeIterator& iter) {
            stream.format("<make-iterator container: {}>", dump(func, iter.container));
        }

        void visit_record(const Value::Record& record) {
            return format(dump(func, record.value), stream);
        }

        void visit_container(const Value::Container& cont) {
            stream.format("{}({})", cont.container, dump(func, cont.args));
        }

        void visit_format(const Value::Format& format) {
            stream.format("<format {}>", dump(func, format.args));
        }

        void visit_error([[maybe_unused]] const Value::Error& error) { stream.format("<error>"); }

        void visit_nop(const Value::Nop&) { stream.format("<nop>"); }
    };

    Visitor visitor{d.parent, stream};
    d.value.visit(visitor);
}

void format(const Dump<InstId>& d, FormatStream& stream) {
    auto inst_id = d.value;
    if (!inst_id) {
        stream.format("<INVALID>");
        return;
    }

    auto& func = d.parent;
    auto& strings = func.strings();
    auto inst = func[inst_id];
    if (inst->name()) {
        stream.format("%{1}_{0}", inst_id.value(), strings.value(inst->name()));
    } else {
        stream.format("%{}", inst_id.value());
    }
}

void format(const Dump<LocalListId>& d, FormatStream& stream) {
    auto list_id = d.value;
    if (!list_id) {
        stream.format("<INVALID>");
        return;
    }

    auto& func = d.parent;
    auto list = func[list_id];

    size_t index = 0;
    for (auto inst : *list) {
        if (index++ > 0)
            stream.format(", ");
        format(dump(func, inst), stream);
    }
}

void format(const Dump<RecordId>& d, FormatStream& stream) {
    auto record_id = d.value;
    if (!record_id) {
        stream.format("<INVALID>");
        return;
    }

    auto& func = d.parent;
    auto record = func[record_id];

    stream.format("<record");
    size_t index = 0;
    for (const auto& [name, value] : *record) {
        if (index++ > 0)
            stream.format(",");
        stream.format(" {}: {}", func.strings().dump(name), dump(func, value));
    }
    stream.format(">");
}

} // namespace dump_helpers
} // namespace tiro::ir
