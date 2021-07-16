#include "compiler/bytecode_gen/func.hpp"

#include "common/entities/entity_storage.hpp"
#include "compiler/bytecode_gen/alloc_registers.hpp"
#include "compiler/bytecode_gen/bytecode_builder.hpp"
#include "compiler/bytecode_gen/locations.hpp"
#include "compiler/ir/function.hpp"
#include "compiler/ir/module.hpp"
#include "compiler/ir/traversal.hpp"
#include "compiler/ir_passes/critical_edges.hpp"

#include "absl/container/inlined_vector.h"

namespace tiro {

namespace {

class FunctionCompiler final {
public:
    explicit FunctionCompiler(const ir::Module& module, const ir::Function& func,
        LinkFunction& result, LinkObject& object)
        : module_(module)
        , func_(func)
        , result_(result)
        , object_(object)
        , builder_(result_.func, func.block_count()) {
        seen_.resize(func_.block_count(), false);
    }

    void run();

    const ir::Module& module() { return module_; }
    BytecodeBuilder& builder() { return builder_; }
    const ir::Function& func() { return func_; }
    LinkFunction& result() { return result_; }
    LinkObject& object() { return object_; }

private:
    bool visit(ir::BlockId block);

    void compile_value(const ir::Value& source, ir::InstId target);
    void compile_lvalue_read(const ir::LValue& source, ir::InstId target);
    void compile_lvalue_write(ir::InstId source, const ir::LValue& target);
    void compile_constant(const ir::Constant& constant, ir::InstId target);
    void compile_terminator(ir::BlockId block_id, const ir::Terminator& term);
    void compile_phi_operands(ir::BlockId predecessor, const ir::Terminator& terminator);

    void emit_copy(const BytecodeLocation& source, const BytecodeLocation& target);

    // Returns the location of that instruction. Follows aliases.
    BytecodeLocation location(ir::InstId id) const;

    // Like location(id), but checks that the location maps to a single register.
    BytecodeRegister value(ir::InstId id) const;

    // Returns the location of the given member in the aggregate instruction.
    BytecodeLocation member_location(ir::InstId aggregate_id, ir::AggregateMember member) const;

    // Like member_location(id, member), but checks that the location maps to a single register.
    BytecodeRegister member_value(ir::InstId aggregate_id, ir::AggregateMember member) const;

    // Returns true if `id` is guaranteed to be null.
    bool is_constant_null(ir::InstId id);

    ir::ModuleMemberId resolve_module_ref(ir::InstId inst_id);

private:
    const ir::Module& module_;
    const ir::Function& func_;
    LinkFunction& result_;
    LinkObject& object_;
    BytecodeBuilder builder_;

    BytecodeLocations locs_;
    std::vector<ir::BlockId> stack_;
    EntityStorage<bool, ir::BlockId> seen_;
};

} // namespace

void FunctionCompiler::run() {
    locs_ = allocate_locations(func_);

    visit(func_.entry());
    while (!stack_.empty()) {
        const auto block_id = stack_.back();
        stack_.pop_back();

        const auto& block = func_[block_id];
        builder_.define_label(block_id);
        builder_.start_handler(block.handler());

        for (const auto& inst_id : block.insts()) {
            compile_value(func_[inst_id].value(), inst_id);
        }

        compile_phi_operands(block_id, block.terminator());
        compile_terminator(block_id, block.terminator());
    }
    builder_.finish();

    if (func_.name())
        result_.func.name(object().use_string(func_.name()));

    result_.func.type(func_.type() == ir::FunctionType::Closure ? BytecodeFunctionType::Closure
                                                                : BytecodeFunctionType::Normal);
    result_.func.params(func_.param_count());
    result_.func.locals(locs_.total_registers());
    result_.refs_ = builder_.take_module_refs();
}

bool FunctionCompiler::visit(ir::BlockId block) {
    if (seen_[block])
        return false;

    seen_[block] = true;
    stack_.push_back(block);
    return true;
}

void FunctionCompiler::compile_value(const ir::Value& source, ir::InstId target) {
    struct Visitor {
        FunctionCompiler& self;
        ir::InstId target;

        void visit_read(const ir::Value::Read& r) { self.compile_lvalue_read(r.target, target); }

        void visit_write(const ir::Value::Write& w) {
            self.compile_lvalue_write(w.value, w.target);
        }

        void visit_alias(const ir::Value::Alias& a) {
            self.emit_copy(self.location(a.target), self.location(target));
        }

        void visit_phi(const ir::Value::Phi&) {
            // Don't do anything. Arguments are provided by the predecessors.
        }

        void visit_observe_assign(const ir::Value::ObserveAssign& o) {
            // All publish_assign instructions write to the preallocated location, and we read from it here.
            // This is probably a bit wasteful, but it is the simplest approach to implement i can come up with right now.
            auto loc = self.locs_.get_preallocated_location(o.symbol);
            self.emit_copy(loc, self.location(target));
        }

        void visit_publish_assign(const ir::Value::PublishAssign& p) {
            self.emit_copy(self.location(p.value), self.location(target));
        }

        void visit_constant(const ir::Constant& constant) {
            self.compile_constant(constant, target);
        }

        void visit_outer_environment(const ir::Value::OuterEnvironment&) {
            self.builder().emit(BytecodeInstr::make_load_closure(self.value(target)));
        }

        void visit_binary_op(const ir::Value::BinaryOp& bin) {
            auto lhs_value = self.value(bin.left);
            auto rhs_value = self.value(bin.right);
            auto target_value = self.value(target);

            auto instruction = [&]() {
                switch (bin.op) {
#define TIRO_CASE(op, ins)     \
    case ir::BinaryOpType::op: \
        return BytecodeInstr::make_##ins(lhs_value, rhs_value, target_value);

                    TIRO_CASE(Plus, add)
                    TIRO_CASE(Minus, sub)
                    TIRO_CASE(Multiply, mul)
                    TIRO_CASE(Divide, div)
                    TIRO_CASE(Modulus, mod)
                    TIRO_CASE(Power, pow)
                    TIRO_CASE(LeftShift, lsh)
                    TIRO_CASE(RightShift, rsh)
                    TIRO_CASE(BitwiseAnd, band)
                    TIRO_CASE(BitwiseOr, bor)
                    TIRO_CASE(BitwiseXor, bxor)
                    TIRO_CASE(Less, lt)
                    TIRO_CASE(LessEquals, lte)
                    TIRO_CASE(Greater, gt)
                    TIRO_CASE(GreaterEquals, gte)
                    TIRO_CASE(Equals, eq)
                    TIRO_CASE(NotEquals, neq)

#undef TIRO_CASE
                }
                TIRO_UNREACHABLE("Invalid binary operation type.");
            }();
            self.builder().emit(instruction);
        }

        void visit_unary_op(const ir::Value::UnaryOp& un) {
            auto operand_value = self.value(un.operand);
            auto target_value = self.value(target);

            auto instruction = [&]() {
                switch (un.op) {
#define TIRO_CASE(op, ins)    \
    case ir::UnaryOpType::op: \
        return BytecodeInstr::make_##ins(operand_value, target_value);

                    TIRO_CASE(Plus, uadd)
                    TIRO_CASE(Minus, uneg)
                    TIRO_CASE(BitwiseNot, bnot)
                    TIRO_CASE(LogicalNot, lnot)

#undef TIRO_CASE
                }
                TIRO_UNREACHABLE("Invalid unary operation type.");
            }();
            self.builder().emit(instruction);
        }

        void visit_call(const ir::Value::Call& c) {
            auto source_value = self.value(c.func);
            auto target_value = self.value(target);
            auto argc = push_args(c.args);
            self.builder().emit(BytecodeInstr::make_call(source_value, argc));
            self.builder().emit(BytecodeInstr::make_pop_to(target_value));
        }

        void visit_aggregate(const ir::Value::Aggregate& a) {
            switch (a.type()) {
            case ir::AggregateType::Method: {
                const auto& method = a.as_method();

                auto instance_value = self.value(method.instance);
                auto name_value = self.object().use_symbol(method.function);

                auto out_instance = self.member_value(target, ir::AggregateMember::MethodInstance);
                auto out_method = self.member_value(target, ir::AggregateMember::MethodFunction);

                self.builder().emit(BytecodeInstr::make_load_method(
                    instance_value, name_value, out_instance, out_method));
                return;
            }
            case ir::AggregateType::IteratorNext: {
                const auto& next = a.as_iterator_next();

                auto iterator_value = self.value(next.iterator);

                auto out_valid = self.member_value(target, ir::AggregateMember::IteratorNextValid);
                auto out_value = self.member_value(target, ir::AggregateMember::IteratorNextValue);
                self.builder().emit(
                    BytecodeInstr::make_iterator_next(iterator_value, out_valid, out_value));
                return;
            }
            }

            TIRO_UNREACHABLE("Invalid aggregate type.");
        }

        void visit_get_aggregate_member(const ir::Value::GetAggregateMember&) {
            // Aggregate accesses map to register aliases, they are not compiled.
            return;
        }

        void visit_method_call(const ir::Value::MethodCall& c) {
            auto instance_value = self.member_value(c.method, ir::AggregateMember::MethodInstance);
            auto method_value = self.member_value(c.method, ir::AggregateMember::MethodFunction);

            auto target_value = self.value(target);
            self.builder().emit(BytecodeInstr::make_push(instance_value));

            auto argc = push_args(c.args);
            self.builder().emit(BytecodeInstr::make_call_method(method_value, argc));
            self.builder().emit(BytecodeInstr::make_pop_to(target_value));
        }

        void visit_make_environment(const ir::Value::MakeEnvironment& e) {
            auto parent_value = self.value(e.parent);
            auto target_value = self.value(target);
            self.builder().emit(BytecodeInstr::make_env(parent_value, e.size, target_value));
        }

        void visit_make_closure(const ir::Value::MakeClosure& c) {
            auto tmpl_value = self.object().use_member(c.func);
            auto env_value = self.value(c.env);
            auto target_value = self.value(target);
            self.builder().emit(BytecodeInstr::make_closure(tmpl_value, env_value, target_value));
        }

        void visit_make_iterator(const ir::Value::MakeIterator& i) {
            auto container_value = self.value(i.container);
            auto target_value = self.value(target);
            self.builder().emit(BytecodeInstr::make_iterator(container_value, target_value));
        }

        void visit_record(const ir::Value::Record& r) {
            auto target_value = self.value(target);
            const auto& record = self.func()[r.value];

            // Assemble the set of symbol keys.
            absl::InlinedVector<BytecodeMemberId, 8> keys;
            keys.reserve(record.size());
            for (const auto& pair : record)
                keys.push_back(self.object().use_symbol(pair.first));

            // Fetch (or create) a record template for the current composition of keys.
            auto tmpl = self.object().use_record({keys.data(), keys.size()});
            self.builder().emit(BytecodeInstr::make_record(tmpl, target_value));

            // Write the actual values into the record. Null constants can be skipped because all record values
            // are initialized to null.
            for (const auto& [key_name, ir_value] : record) {
                auto key = self.object().use_symbol(key_name);
                if (!self.is_constant_null(ir_value)) {
                    auto value = self.value(ir_value);
                    self.builder().emit(BytecodeInstr::make_store_member(value, target_value, key));
                }
            }
        }

        void visit_container(const ir::Value::Container& c) {
            auto target_value = self.value(target);
            auto argc = push_args(c.args);
            switch (c.container) {
            case ir::ContainerType::Array: {
                self.builder().emit(BytecodeInstr::make_array(argc, target_value));
                return;
            }
            case ir::ContainerType::Tuple: {
                self.builder().emit(BytecodeInstr::make_tuple(argc, target_value));
                return;
            }
            case ir::ContainerType::Set: {
                self.builder().emit(BytecodeInstr::make_set(argc, target_value));
                return;
            }
            case ir::ContainerType::Map: {
                self.builder().emit(BytecodeInstr::make_map(argc, target_value));
                return;
            }
            }
            TIRO_UNREACHABLE("Invalid container type.");
        }

        void visit_format(const ir::Value::Format& f) {
            auto target_value = self.value(target);
            const auto& args = self.func()[f.args];

            self.builder().emit(BytecodeInstr::make_formatter(target_value));
            for (const auto& ir_arg : args) {
                auto arg_value = self.value(ir_arg);
                self.builder().emit(BytecodeInstr::make_append_format(arg_value, target_value));
            }

            self.builder().emit(BytecodeInstr::make_format_result(target_value, target_value));
        }

        void visit_error([[maybe_unused]] const ir::Value::Error& e) {
            TIRO_ERROR("The internal representation contains errors.");
        }

        void visit_nop(const ir::Value::Nop&) {}

        u32 push_args(ir::LocalListId list_id) {
            const auto& args = self.func()[list_id];
            const u32 argc = args.size();
            for (const auto& ir_arg : args) {
                auto arg_value = self.value(ir_arg);
                self.builder().emit(BytecodeInstr::make_push(arg_value));
            }
            return argc;
        }
    };

    source.visit(Visitor{*this, target});
}

void FunctionCompiler::compile_lvalue_read(const ir::LValue& source, ir::InstId target) {
    struct Visitor {
        FunctionCompiler& self;
        ir::InstId target;

        void visit_param(const ir::LValue::Param& p) {
            auto source_param = BytecodeParam(p.target.value());
            auto target_value = self.value(target);
            self.builder().emit(BytecodeInstr::make_load_param(source_param, target_value));
        }

        void visit_closure(const ir::LValue::Closure& c) {
            auto env_value = self.value(c.env);
            auto target_value = self.value(target);
            self.builder().emit(
                BytecodeInstr::make_load_env(env_value, c.levels, c.index, target_value));
        }

        void visit_module(const ir::LValue::Module& m) {
            auto source = self.object().use_member(m.member);
            auto target_value = self.value(target);
            self.builder().emit(BytecodeInstr::make_load_module(source, target_value));
        }

        void visit_field(const ir::LValue::Field& f) {
            auto object_value = self.value(f.object);
            auto name = self.object().use_symbol(f.name);
            auto target_value = self.value(target);
            self.builder().emit(BytecodeInstr::make_load_member(object_value, name, target_value));
        }

        void visit_tuple_field(const ir::LValue::TupleField& t) {
            auto tuple_value = self.value(t.object);
            auto target_value = self.value(target);
            self.builder().emit(
                BytecodeInstr::make_load_tuple_member(tuple_value, t.index, target_value));
        }

        void visit_index(const ir::LValue::Index& i) {
            auto array_value = self.value(i.object);
            auto index_value = self.value(i.index);
            auto target_value = self.value(target);
            self.builder().emit(
                BytecodeInstr::make_load_index(array_value, index_value, target_value));
        }
    };
    source.visit(Visitor{*this, target});
}

void FunctionCompiler::compile_lvalue_write(ir::InstId source, const ir::LValue& target) {
    struct Visitor {
        FunctionCompiler& self;
        BytecodeRegister source_value;

        void visit_param(const ir::LValue::Param& p) {
            auto target_param = BytecodeParam(p.target.value());
            self.builder().emit(BytecodeInstr::make_store_param(source_value, target_param));
        }

        void visit_closure(const ir::LValue::Closure& c) {
            auto env_value = self.value(c.env);
            self.builder().emit(
                BytecodeInstr::make_store_env(source_value, env_value, c.levels, c.index));
        }

        void visit_module(const ir::LValue::Module& m) {
            auto target = self.object().use_member(m.member);
            self.builder().emit(BytecodeInstr::make_store_module(source_value, target));
        }

        void visit_field(const ir::LValue::Field& f) {
            auto object_value = self.value(f.object);
            auto name = self.object().use_symbol(f.name);
            self.builder().emit(BytecodeInstr::make_store_member(source_value, object_value, name));
        }

        void visit_tuple_field(const ir::LValue::TupleField& t) {
            auto tuple_value = self.value(t.object);
            self.builder().emit(
                BytecodeInstr::make_store_tuple_member(source_value, tuple_value, t.index));
        }

        void visit_index(const ir::LValue::Index& i) {
            auto array_value = self.value(i.object);
            auto index_value = self.value(i.index);
            self.builder().emit(
                BytecodeInstr::make_store_index(source_value, array_value, index_value));
        }
    };
    target.visit(Visitor{*this, value(source)});
}

void FunctionCompiler::compile_constant(const ir::Constant& c, ir::InstId target) {
    struct Visitor {
        FunctionCompiler& self;
        BytecodeRegister target_value;

        // Improvement: it might be useful to only pack small integers (e.g. up to 32 bit)
        // into the instruction stream and to store large integers as module level constants.
        void visit_integer(const ir::Constant::Integer& i) {
            self.builder().emit(BytecodeInstr::make_load_int(i.value, target_value));
        }

        void visit_float(const ir::Constant::Float& f) {
            self.builder().emit(BytecodeInstr::make_load_float(f.value, target_value));
        }

        void visit_string(const ir::Constant::String& s) {
            auto id = self.object().use_string(s.value);
            self.builder().emit(BytecodeInstr::make_load_module(id, target_value));
        }

        void visit_symbol(const ir::Constant::Symbol& s) {
            auto id = self.object().use_symbol(s.value);
            self.builder().emit(BytecodeInstr::make_load_module(id, target_value));
        }

        void visit_null(const ir::Constant::Null&) {
            self.builder().emit(BytecodeInstr::make_load_null(target_value));
        }

        void visit_true(const ir::Constant::True&) {
            self.builder().emit(BytecodeInstr::make_load_true(target_value));
        }

        void visit_false(const ir::Constant::False&) {
            self.builder().emit(BytecodeInstr::make_load_false(target_value));
        }
    };
    c.visit(Visitor{*this, value(target)});
}

void FunctionCompiler::compile_terminator(ir::BlockId block_id, const ir::Terminator& term) {
    struct Visitor {
        FunctionCompiler& self;
        ir::BlockId block_id;

        void visit_none(const ir::Terminator::None&) { TIRO_ERROR("Block without a terminator."); }

        void visit_never(const ir::Terminator::Never&) {}

        void visit_entry(const ir::Terminator::Entry& e) {
            TIRO_CHECK(block_id == self.func().entry(),
                "Only the entry block may have an entry terminator.");

            bool inserted;
            std::for_each(e.handlers.rbegin(), e.handlers.rend(), [&](ir::BlockId handler) {
                inserted = self.visit(handler);
                TIRO_CHECK(inserted, "A handler block was already visited.");
            });

            inserted = self.visit(e.body);
            TIRO_CHECK(inserted, "The body block was already visited.");
        }

        void visit_exit(const ir::Terminator::Exit&) {
            TIRO_CHECK(
                block_id == self.func().exit(), "Only the exit block may have an exit terminator.");
        }

        void visit_jump(const ir::Terminator::Jump& j) {
            if (!self.visit(j.target)) {
                const auto offset = self.builder().use_label(j.target);
                self.builder().emit(BytecodeInstr::make_jmp(offset));
            }
        }

        void visit_branch(const ir::Terminator::Branch& b) {
            const auto value = self.value(b.value);
            {
                self.visit(b.target);
                const auto offset = self.builder().use_label(b.target);
                const auto ins = [&]() {
                    switch (b.type) {
                    case ir::BranchType::IfTrue:
                        return BytecodeInstr::make_jmp_true(value, offset);
                    case ir::BranchType::IfFalse:
                        return BytecodeInstr::make_jmp_false(value, offset);
                    case ir::BranchType::IfNull:
                        return BytecodeInstr::make_jmp_null(value, offset);
                    case ir::BranchType::IfNotNull:
                        return BytecodeInstr::make_jmp_not_null(value, offset);
                    }
                    TIRO_UNREACHABLE("Invalid branch type.");
                }();

                self.builder().emit(ins);
            }

            if (!self.visit(b.fallthrough)) {
                const auto offset = self.builder().use_label(b.fallthrough);
                self.builder().emit(BytecodeInstr::make_jmp(offset));
            }
        }

        void visit_return(const ir::Terminator::Return& r) {
            auto value = self.value(r.value);
            self.builder().emit(BytecodeInstr::make_return(value));
        }

        void visit_rethrow(const ir::Terminator::Rethrow&) {
            self.builder().emit(BytecodeInstr::make_rethrow());
        }

        void visit_assert_fail(const ir::Terminator::AssertFail& a) {
            auto expr_value = self.value(a.expr);
            auto message_value = self.value(a.message);
            self.builder().emit(BytecodeInstr::make_assert_fail(expr_value, message_value));
        }
    };
    term.visit(Visitor{*this, block_id});
}

void FunctionCompiler::compile_phi_operands(ir::BlockId pred, const ir::Terminator& term) {
    // Only normal jumps can transport phi operands. Critical edges are removed before codegen.
    if (term.type() != ir::TerminatorType::Jump) {
#if TIRO_DEBUG
        visit_targets(term, [&](ir::BlockId succ_id) {
            const size_t phi_count = func_[succ_id].phi_count(func_);
            TIRO_DEBUG_ASSERT(phi_count == 0, "Successor with phi functions via non-jump edge.");
        });
#endif
        return;
    }

    if (locs_.has_phi_copies(pred)) {
        for (const auto& copy : locs_.get_phi_copies(pred)) {
            emit_copy(copy.src, copy.dest);
        }
    }
}

void FunctionCompiler::emit_copy(const BytecodeLocation& source, const BytecodeLocation& target) {
    TIRO_DEBUG_ASSERT(
        source.size() == target.size(), "Cannot copy between locations of different size.");

    for (u32 i = 0, n = source.size(); i < n; ++i) {
        // TODO: Do we need parallel copy sequentialization here as well?
        // Overwriting a value could make a later copy invalid.
        auto src_reg = source[i];
        auto target_reg = target[i];
        if (src_reg != target_reg) {
            builder().emit(BytecodeInstr::make_copy(src_reg, target_reg));
        }
    }
}

BytecodeLocation FunctionCompiler::location(ir::InstId id) const {
    return storage_location(id, locs_, func_);
}

BytecodeRegister FunctionCompiler::value(ir::InstId id) const {
    auto loc = location(id);
    TIRO_CHECK(loc.size() == 1,
        "Expected the instruction {} to be mapped to a single physical "
        "register.",
        id);
    return loc[0];
}

BytecodeLocation
FunctionCompiler::member_location(ir::InstId aggregate_id, ir::AggregateMember member) const {
    return get_aggregate_member(aggregate_id, member, locs_, func_);
}

BytecodeRegister
FunctionCompiler::member_value(ir::InstId aggregate_id, ir::AggregateMember member) const {
    auto loc = member_location(aggregate_id, member);
    TIRO_CHECK(loc.size() == 1,
        "Expected the member {}.{} to be mapped to a single physical register.", aggregate_id,
        member);
    return loc[0];
}

bool FunctionCompiler::is_constant_null(ir::InstId id) {
    auto current = id;
    while (1) {
        TIRO_DEBUG_ASSERT(current, "Invalid instruction id.");

        const auto& inst = func_[current];
        switch (inst.value().type()) {
        case ir::ValueType::Alias:
            current = inst.value().as_alias().target;
            break;
        case ir::ValueType::Constant:
            return inst.value().as_constant().type() == ir::ConstantType::Null;
        default:
            return false;
        }
    }
}

[[maybe_unused]] ir::ModuleMemberId FunctionCompiler::resolve_module_ref(ir::InstId inst_id) {
    auto current_id = inst_id;
    while (1) {
        const auto& inst = func_[current_id];
        const auto& value = inst.value();

        switch (value.type()) {
        case ir::ValueType::Alias:
            current_id = value.as_alias().target;
            break;
        case ir::ValueType::Read: {
            const auto& lvalue = value.as_read().target;
            if (lvalue.type() == ir::LValueType::Module)
                return lvalue.as_module().member;

            TIRO_ERROR("{} did not resolve to a module member reference.", inst_id);
            break;
        }
        default:
            TIRO_ERROR("{} did not resolve to a module member reference.", inst_id);
        }
    }
}

static InternedString
exported_member_name(const ir::ModuleMember& member, const ir::Module& module) {
    struct NameVisitor {
        const ir::Module& module;

        InternedString visit_import(const ir::ModuleMemberData::Import& i) { return i.name; }

        InternedString visit_variable(const ir::ModuleMemberData::Variable& v) { return v.name; }

        InternedString visit_function(const ir::ModuleMemberData::Function& f) {
            const auto& function = module[f.id];
            TIRO_DEBUG_ASSERT(function.type() == ir::FunctionType::Normal,
                "Only normal functions can be exported.");
            return function.name();
        }
    };

    auto name = member.data().visit(NameVisitor{module});
    TIRO_DEBUG_ASSERT(name, "Anonymous module members cannot be exported.");
    return name;
}

static LinkFunction
compile_function(const ir::Module& module, ir::Function& func, LinkObject& object) {
    split_critical_edges(func);

    LinkFunction lf;
    FunctionCompiler compiler(module, func, lf, object);
    compiler.run();
    return lf;
}

static BytecodeMemberId
compile_member(ir::ModuleMemberId member_id, ir::Module& module, LinkObject& object) {
    struct MemberVisitor {
        ir::Module& module;
        LinkObject& object;
        ir::ModuleMemberId member_id;

        BytecodeMemberId visit_import(const ir::ModuleMemberData::Import& i) {
            auto name = object.use_string(i.name);
            return object.define_import(member_id, BytecodeMember::Import(name));
        }

        BytecodeMemberId visit_variable(const ir::ModuleMemberData::Variable& v) {
            // Initial value not implemented yet (always null).
            auto name = object.use_string(v.name);
            return object.define_variable(member_id, BytecodeMember::Variable(name, {}));
        }

        BytecodeMemberId visit_function(const ir::ModuleMemberData::Function& f) {
            auto& func = module[f.id];
            return object.define_function(member_id, compile_function(module, func, object));
        }
    };

    auto member = module.ptr_to(member_id);
    auto compiled_member_id = member->data().visit(MemberVisitor{module, object, member_id});
    if (member->exported()) {
        auto name = exported_member_name(*member, module);
        object.define_export(name, compiled_member_id);
    }
    return compiled_member_id;
}

LinkObject compile_object(ir::Module& module, Span<const ir::ModuleMemberId> members) {
    LinkObject object;
    for (const auto id : members)
        compile_member(id, module, object);
    return object;
}

} // namespace tiro
