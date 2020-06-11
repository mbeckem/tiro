#include "tiro/bytecode_gen/func.hpp"

#include "tiro/bytecode_gen/alloc_registers.hpp"
#include "tiro/bytecode_gen/bytecode_builder.hpp"
#include "tiro/bytecode_gen/locations.hpp"
#include "tiro/ir/critical_edges.hpp"
#include "tiro/ir/function.hpp"
#include "tiro/ir/module.hpp"
#include "tiro/ir/traversal.hpp"

#include <unordered_set>

namespace tiro {

namespace {

class FunctionCompiler final {
public:
    explicit FunctionCompiler(
        const Module& module, const Function& func, LinkFunction& result, LinkObject& object)
        : module_(module)
        , func_(func)
        , result_(result)
        , object_(object)
        , builder_(result_.func.code(), func.block_count()) {
        seen_.resize(func_.block_count(), false);
    }

    void run();

    const Module& module() { return module_; }
    BytecodeBuilder& builder() { return builder_; }
    const Function& func() { return func_; }
    LinkFunction& result() { return result_; }
    LinkObject& object() { return object_; }

private:
    bool visit(BlockId block);

    void compile_rvalue(const RValue& source, LocalId target);
    void compile_lvalue_read(const LValue& source, LocalId target);
    void compile_lvalue_write(LocalId source, const LValue& target);
    void compile_constant(const Constant& constant, LocalId target);
    void compile_terminator(const Terminator& term);
    void compile_phi_operands(BlockId predecessor, const Terminator& terminator);

    void emit_copy(const BytecodeLocation& source, const BytecodeLocation& target);

    // Returns the location of that local. Follows aliases.
    BytecodeLocation location(LocalId id) const;

    // Like location(id), but checks that the location maps to a single register.
    BytecodeRegister value(LocalId id) const;

    // Returns the location of the given member in the aggregate local.
    BytecodeLocation member_location(LocalId aggregate_id, AggregateMember member) const;

    // Like member_location(id, member), but checks that the location maps to a single register.
    BytecodeRegister member_value(LocalId aggregate_id, AggregateMember member) const;

    ModuleMemberId resolve_module_ref(LocalId local);

private:
    const Module& module_;
    const Function& func_;
    LinkFunction& result_;
    LinkObject& object_;
    BytecodeBuilder builder_;

    BytecodeLocations locs_;
    std::vector<BlockId> stack_;
    IndexMap<bool, IdMapper<BlockId>> seen_;
};

} // namespace

void FunctionCompiler::run() {
    locs_ = allocate_locations(func_);

    visit(func_.entry());
    while (!stack_.empty()) {
        const auto block_id = stack_.back();
        stack_.pop_back();

        const auto block = func_[block_id];
        builder_.define_label(block_id);

        for (const auto& stmt : block->stmts()) {
            switch (stmt.type()) {
            case StmtType::Assign: {
                const auto& assign = stmt.as_assign();
                compile_lvalue_write(assign.value, assign.target);
                break;
            }
            case StmtType::Define:
                const auto target = stmt.as_define().local;
                compile_rvalue(func_[target]->value(), target);
                break;
            }
        }

        compile_phi_operands(block_id, block->terminator());
        compile_terminator(block->terminator());
    }
    builder_.finish();

    if (func_.name())
        result_.func.name(object().use_string(func_.name()));

    result_.func.type(func_.type() == FunctionType::Closure ? BytecodeFunctionType::Closure
                                                            : BytecodeFunctionType::Normal);
    result_.func.params(func_.param_count());
    result_.func.locals(locs_.total_registers());
    result_.refs_ = builder_.take_module_refs();
}

bool FunctionCompiler::visit(BlockId block) {
    if (seen_[block])
        return false;

    seen_[block] = true;
    stack_.push_back(block);
    return true;
}

void FunctionCompiler::compile_rvalue(const RValue& source, LocalId target) {
    struct Visitor {
        FunctionCompiler& self;
        LocalId target;

        void visit_use_lvalue(const RValue::UseLValue& u) {
            self.compile_lvalue_read(u.target, target);
        }

        void visit_use_local(const RValue::UseLocal& u) {
            self.emit_copy(self.location(u.target), self.location(target));
        }

        void visit_phi(const RValue::Phi&) {
            // Don't do anything. Arguments are provided by the predecessors.
        }

        void visit_phi0(const RValue::Phi0&) {}

        void visit_constant(const Constant& constant) { self.compile_constant(constant, target); }

        void visit_outer_environment(const RValue::OuterEnvironment&) {
            self.builder().emit(BytecodeInstr::make_load_closure(self.value(target)));
        }

        void visit_binary_op(const RValue::BinaryOp& bin) {
            auto lhs_value = self.value(bin.left);
            auto rhs_value = self.value(bin.right);
            auto target_value = self.value(target);

            auto instruction = [&]() {
                switch (bin.op) {
#define TIRO_CASE(op, ins) \
    case BinaryOpType::op: \
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

        void visit_unary_op(const RValue::UnaryOp& un) {
            auto operand_value = self.value(un.operand);
            auto target_value = self.value(target);

            auto instruction = [&]() {
                switch (un.op) {
#define TIRO_CASE(op, ins) \
    case UnaryOpType::op:  \
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

        // TODO: a call static variant when the call target is known to be a
        // module member?
        void visit_call(const RValue::Call& c) {
            auto source_value = self.value(c.func);
            auto target_value = self.value(target);
            auto argc = push_args(c.args);
            self.builder().emit(BytecodeInstr::make_call(source_value, argc));
            self.builder().emit(BytecodeInstr::make_pop_to(target_value));
        }

        void visit_aggregate(const RValue::Aggregate& a) {
            switch (a.type()) {
            case AggregateType::Method: {
                const auto& method = a.as_method();

                auto instance_value = self.value(method.instance);
                auto name_value = self.object().use_symbol(method.function);

                auto out_instance = self.member_value(target, AggregateMember::MethodInstance);
                auto out_method = self.member_value(target, AggregateMember::MethodFunction);

                self.builder().emit(BytecodeInstr::make_load_method(
                    instance_value, name_value, out_instance, out_method));
                return;
            }
            }

            TIRO_UNREACHABLE("Invalid aggregate type.");
        }

        void visit_get_aggregate_member(const RValue::GetAggregateMember&) {
            // Aggregate accesses map to register aliases, they are not compiled.
            return;
        }

        void visit_method_call(const RValue::MethodCall& c) {
            auto instance_value = self.member_value(c.method, AggregateMember::MethodInstance);
            auto method_value = self.member_value(c.method, AggregateMember::MethodFunction);

            auto target_value = self.value(target);
            self.builder().emit(BytecodeInstr::make_push(instance_value));

            auto argc = push_args(c.args);
            self.builder().emit(BytecodeInstr::make_call_method(method_value, argc));
            self.builder().emit(BytecodeInstr::make_pop_to(target_value));
        }

        void visit_make_environment(const RValue::MakeEnvironment& e) {
            auto parent_value = self.value(e.parent);
            auto target_value = self.value(target);
            self.builder().emit(BytecodeInstr::make_env(parent_value, e.size, target_value));
        }

        void visit_make_closure(const RValue::MakeClosure& c) {
            auto tmpl_value = self.value(c.func);
            auto env_value = self.value(c.env);
            auto target_value = self.value(target);
            self.builder().emit(BytecodeInstr::make_closure(tmpl_value, env_value, target_value));
        }

        void visit_container(const RValue::Container& c) {
            auto target_value = self.value(target);
            auto argc = push_args(c.args);
            auto instruction = [&]() {
                switch (c.container) {
                case ContainerType::Array:
                    return BytecodeInstr::make_array(argc, target_value);
                case ContainerType::Tuple:
                    return BytecodeInstr::make_tuple(argc, target_value);
                case ContainerType::Set:
                    return BytecodeInstr::make_set(argc, target_value);
                case ContainerType::Map:
                    return BytecodeInstr::make_map(argc, target_value);
                }
                TIRO_UNREACHABLE("Invalid container type.");
            }();
            self.builder().emit(instruction);
        }

        void visit_format(const RValue::Format& f) {
            auto target_value = self.value(target);
            auto args = self.func()[f.args];

            self.builder().emit(BytecodeInstr::make_formatter(target_value));
            for (const auto& ir_arg : *args) {
                auto arg_value = self.value(ir_arg);
                self.builder().emit(BytecodeInstr::make_append_format(arg_value, target_value));
            }

            self.builder().emit(BytecodeInstr::make_format_result(target_value, target_value));
        }

        void visit_error([[maybe_unused]] const RValue::Error& e) {
            TIRO_ERROR("The internal representation contains errors.");
        }

        u32 push_args(LocalListId list_id) {
            auto args = self.func()[list_id];
            const u32 argc = args->size();
            for (const auto& ir_arg : *args) {
                auto arg_value = self.value(ir_arg);
                self.builder().emit(BytecodeInstr::make_push(arg_value));
            }
            return argc;
        }
    };
    source.visit(Visitor{*this, target});
}

void FunctionCompiler::compile_lvalue_read(const LValue& source, LocalId target) {
    struct Visitor {
        FunctionCompiler& self;
        LocalId target;

        void visit_param(const LValue::Param& p) {
            auto source_param = BytecodeParam(p.target.value());
            auto target_value = self.value(target);
            self.builder().emit(BytecodeInstr::make_load_param(source_param, target_value));
        }

        void visit_closure(const LValue::Closure& c) {
            auto env_value = self.value(c.env);
            auto target_value = self.value(target);
            self.builder().emit(
                BytecodeInstr::make_load_env(env_value, c.levels, c.index, target_value));
        }

        void visit_module(const LValue::Module& m) {
            auto source = self.object().use_member(m.member);
            auto target_value = self.value(target);
            self.builder().emit(BytecodeInstr::make_load_module(source, target_value));
        }

        void visit_field(const LValue::Field& f) {
            auto object_value = self.value(f.object);
            auto name = self.object().use_symbol(f.name);
            auto target_value = self.value(target);
            self.builder().emit(BytecodeInstr::make_load_member(object_value, name, target_value));
        }

        void visit_tuple_field(const LValue::TupleField& t) {
            auto tuple_value = self.value(t.object);
            auto target_value = self.value(target);
            self.builder().emit(
                BytecodeInstr::make_load_tuple_member(tuple_value, t.index, target_value));
        }

        void visit_index(const LValue::Index& i) {
            auto array_value = self.value(i.object);
            auto index_value = self.value(i.index);
            auto target_value = self.value(target);
            self.builder().emit(
                BytecodeInstr::make_load_index(array_value, index_value, target_value));
        }
    };
    source.visit(Visitor{*this, target});
}

void FunctionCompiler::compile_lvalue_write(LocalId source, const LValue& target) {
    struct Visitor {
        FunctionCompiler& self;
        BytecodeRegister source_value;

        void visit_param(const LValue::Param& p) {
            auto target_param = BytecodeParam(p.target.value());
            self.builder().emit(BytecodeInstr::make_store_param(source_value, target_param));
        }

        void visit_closure(const LValue::Closure& c) {
            auto env_value = self.value(c.env);
            self.builder().emit(
                BytecodeInstr::make_store_env(source_value, env_value, c.levels, c.index));
        }

        void visit_module(const LValue::Module& m) {
            auto target = self.object().use_member(m.member);
            self.builder().emit(BytecodeInstr::make_store_module(source_value, target));
        }

        void visit_field(const LValue::Field& f) {
            auto object_value = self.value(f.object);
            auto name = self.object().use_symbol(f.name);
            self.builder().emit(BytecodeInstr::make_store_member(source_value, object_value, name));
        }

        void visit_tuple_field(const LValue::TupleField& t) {
            auto tuple_value = self.value(t.object);
            self.builder().emit(
                BytecodeInstr::make_store_tuple_member(source_value, tuple_value, t.index));
        }

        void visit_index(const LValue::Index& i) {
            auto array_value = self.value(i.object);
            auto index_value = self.value(i.index);
            self.builder().emit(
                BytecodeInstr::make_store_index(source_value, array_value, index_value));
        }
    };
    target.visit(Visitor{*this, value(source)});
}

void FunctionCompiler::compile_constant(const Constant& c, LocalId target) {
    struct Visitor {
        FunctionCompiler& self;
        BytecodeRegister target_value;

        // Improvement: it might be useful to only pack small integers (e.g. up to 32 bit)
        // into the instruction stream and to store large integers as module level constants.
        void visit_integer(const Constant::Integer& i) {
            self.builder().emit(BytecodeInstr::make_load_int(i.value, target_value));
        }

        void visit_float(const Constant::Float& f) {
            self.builder().emit(BytecodeInstr::make_load_float(f.value, target_value));
        }

        void visit_string(const Constant::String& s) {
            auto id = self.object().use_string(s.value);
            self.builder().emit(BytecodeInstr::make_load_module(id, target_value));
        }

        void visit_symbol(const Constant::Symbol& s) {
            auto id = self.object().use_symbol(s.value);
            self.builder().emit(BytecodeInstr::make_load_module(id, target_value));
        }

        void visit_null(const Constant::Null&) {
            self.builder().emit(BytecodeInstr::make_load_null(target_value));
        }

        void visit_true(const Constant::True&) {
            self.builder().emit(BytecodeInstr::make_load_true(target_value));
        }

        void visit_false(const Constant::False&) {
            self.builder().emit(BytecodeInstr::make_load_false(target_value));
        }
    };
    c.visit(Visitor{*this, value(target)});
}

void FunctionCompiler::compile_terminator(const Terminator& term) {
    struct Visitor {
        FunctionCompiler& self;

        void visit_none(const Terminator::None&) {}

        void visit_jump(const Terminator::Jump& j) {
            if (!self.visit(j.target)) {
                const auto offset = self.builder().use_label(j.target);
                self.builder().emit(BytecodeInstr::make_jmp(offset));
            }
        }

        void visit_branch(const Terminator::Branch& b) {
            const auto value = self.value(b.value);
            {
                self.visit(b.target);
                const auto offset = self.builder().use_label(b.target);
                const auto ins = [&]() {
                    switch (b.type) {
                    case BranchType::IfTrue:
                        return BytecodeInstr::make_jmp_true(value, offset);
                    case BranchType::IfFalse:
                        return BytecodeInstr::make_jmp_false(value, offset);
                    case BranchType::IfNull:
                        return BytecodeInstr::make_jmp_null(value, offset);
                    case BranchType::IfNotNull:
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

        void visit_return(const Terminator::Return& r) {
            auto value = self.value(r.value);
            self.builder().emit(BytecodeInstr::make_return(value));
        }

        void visit_exit(const Terminator::Exit&) {}

        void visit_assert_fail(const Terminator::AssertFail& a) {
            auto expr_value = self.value(a.expr);
            auto message_value = self.value(a.message);
            self.builder().emit(BytecodeInstr::make_assert_fail(expr_value, message_value));
        }

        void visit_never(const Terminator::Never&) {}
    };
    term.visit(Visitor{*this});
}

void FunctionCompiler::compile_phi_operands(BlockId pred, const Terminator& term) {
    // Only normal jumps can transport phi operands. Critical edges are removed before codegen.
    if (term.type() != TerminatorType::Jump) {
#if defined(TIRO_DEBUG)
        visit_targets(term, [&](BlockId succ_id) {
            const size_t phi_count = func_[succ_id]->phi_count(func_);
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

BytecodeLocation FunctionCompiler::location(LocalId id) const {
    return storage_location(id, locs_, func_);
}

BytecodeRegister FunctionCompiler::value(LocalId id) const {
    auto loc = location(id);
    TIRO_CHECK(loc.size() == 1,
        "Expected the local {} to be mapped to a single physical "
        "register.",
        id);
    return loc[0];
}

BytecodeLocation
FunctionCompiler::member_location(LocalId aggregate_id, AggregateMember member) const {
    return get_aggregate_member(aggregate_id, member, locs_, func_);
}

BytecodeRegister
FunctionCompiler::member_value(LocalId aggregate_id, AggregateMember member) const {
    auto loc = member_location(aggregate_id, member);
    TIRO_CHECK(loc.size() == 1,
        "Expected the member {}.{} to be mapped to a single physical "
        "register.",
        aggregate_id, member);
    return loc[0];
}

[[maybe_unused]] ModuleMemberId FunctionCompiler::resolve_module_ref(LocalId local_id) {
    auto current_id = local_id;
    while (1) {
        auto local = func_[current_id];
        const auto& rvalue = local->value();

        switch (rvalue.type()) {
        case RValueType::UseLocal:
            current_id = rvalue.as_use_local().target;
            break;
        case RValueType::UseLValue: {
            const auto& lvalue = rvalue.as_use_lvalue().target;
            if (lvalue.type() == LValueType::Module)
                return lvalue.as_module().member;

            TIRO_ERROR("{} did not resolve to a module member reference.", local_id);
            break;
        }
        default:
            TIRO_ERROR("{} did not resolve to a module member reference.", local_id);
        }
    }
}

static InternedString exported_member_name(const ModuleMember& member, const Module& module) {
    struct NameVisitor {
        const Module& module;

        InternedString visit_import(const ModuleMemberData::Import& i) { return i.name; }

        InternedString visit_variable(const ModuleMemberData::Variable& v) { return v.name; }

        InternedString visit_function(const ModuleMemberData::Function& f) {
            auto function = module[f.id];
            TIRO_DEBUG_ASSERT(
                function->type() == FunctionType::Normal, "Only normal functions can be exported.");
            return function->name();
        }
    };

    auto name = member.data().visit(NameVisitor{module});
    TIRO_DEBUG_ASSERT(name, "Anonymous module members cannot be exported.");
    return name;
}

static LinkFunction compile_function(const Module& module, Function& func, LinkObject& object) {
    split_critical_edges(func);

    LinkFunction lf;
    FunctionCompiler compiler(module, func, lf, object);
    compiler.run();
    return lf;
}

static BytecodeMemberId
compile_member(ModuleMemberId member_id, Module& module, LinkObject& object) {
    struct MemberVisitor {
        Module& module;
        LinkObject& object;
        ModuleMemberId member_id;

        BytecodeMemberId visit_import(const ModuleMemberData::Import& i) {
            auto name = object.use_string(i.name);
            return object.define_import(member_id, BytecodeMember::Import(name));
        }

        BytecodeMemberId visit_variable(const ModuleMemberData::Variable& v) {
            // Initial value not implemented yet (always null).
            auto name = object.use_string(v.name);
            return object.define_variable(member_id, BytecodeMember::Variable(name, {}));
        }

        BytecodeMemberId visit_function(const ModuleMemberData::Function& f) {
            auto func = module[f.id];
            return object.define_function(member_id, compile_function(module, *func, object));
        }
    };

    auto member = module[member_id];
    auto compiled_member_id = member->data().visit(MemberVisitor{module, object, member_id});
    if (member->exported()) {
        auto name = exported_member_name(*member, module);
        object.define_export(name, compiled_member_id);
    }
    return compiled_member_id;
}

LinkObject compile_object(Module& module, Span<const ModuleMemberId> members) {
    LinkObject object;
    for (const auto id : members)
        compile_member(id, module, object);
    return object;
}

} // namespace tiro
