#include "tiro/bytecode_gen/gen_func.hpp"

#include "tiro/bytecode_gen/bytecode_builder.hpp"
#include "tiro/bytecode_gen/locations.hpp"
#include "tiro/ir/construct_cssa.hpp"
#include "tiro/ir/critical_edges.hpp"
#include "tiro/ir/traversal.hpp"

#include <unordered_set>

namespace tiro {

namespace {

class FunctionCompiler final {
public:
    explicit FunctionCompiler(const Module& module, const Function& func,
        LinkFunction& result, LinkObject& object)
        : module_(module)
        , func_(func)
        , result_(result)
        , object_(object)
        , builder_(result_.func.code(), func.block_count()) {
        seen_.resize(func_.block_count(), false);
    }

    void run();

    BytecodeBuilder& builder() { return builder_; }
    const Function& func() { return func_; }
    LinkFunction& result() { return result_; }
    LinkObject& object() { return object_; }

private:
    bool visit(BlockID block);

    void compile_rvalue(const RValue& source, LocalID target);
    void compile_lvalue_read(const LValue& source, LocalID target);
    void compile_lvalue_write(LocalID source, const LValue& target);
    void compile_constant(const Constant& constant, LocalID target);
    void compile_terminator(const Terminator& term);
    void
    compile_phi_operands(BlockID predecessor, const Terminator& terminator);

    void
    emit_copy(const CompiledLocation& source, const CompiledLocation& target);

    CompiledLocation location(LocalID id) const;
    BytecodeRegister value(LocalID id) const;
    CompiledLocation::Method method(LocalID id) const;

    ModuleMemberID resolve_module_ref(LocalID local);

private:
    const Module& module_;
    const Function& func_;
    LinkFunction& result_;
    LinkObject& object_;
    BytecodeBuilder builder_;

    CompiledLocations locs_;
    std::vector<BlockID> stack_;
    IndexMap<bool, IDMapper<BlockID>> seen_;
};

} // namespace

/* [[[cog
    import unions
    import bytecode_gen
    unions.implement_type(bytecode_gen.LinkItemType)
]]] */
std::string_view to_string(LinkItemType type) {
    switch (type) {
    case LinkItemType::Use:
        return "Use";
    case LinkItemType::Definition:
        return "Definition";
    }
    TIRO_UNREACHABLE("Invalid LinkItemType.");
}
// [[[end]]]

/* [[[cog
    import unions
    import bytecode_gen
    unions.implement_type(bytecode_gen.LinkItem)
]]] */
LinkItem LinkItem::make_use(const Use& use) {
    return use;
}

LinkItem LinkItem::make_definition(
    const ModuleMemberID& ir_id, const BytecodeMember& value) {
    return Definition{ir_id, value};
}

LinkItem::LinkItem(const Use& use)
    : type_(LinkItemType::Use)
    , use_(use) {}

LinkItem::LinkItem(const Definition& definition)
    : type_(LinkItemType::Definition)
    , definition_(definition) {}

const LinkItem::Use& LinkItem::as_use() const {
    TIRO_DEBUG_ASSERT(type_ == LinkItemType::Use,
        "Bad member access on LinkItem: not a Use.");
    return use_;
}

const LinkItem::Definition& LinkItem::as_definition() const {
    TIRO_DEBUG_ASSERT(type_ == LinkItemType::Definition,
        "Bad member access on LinkItem: not a Definition.");
    return definition_;
}

void LinkItem::format(FormatStream& stream) const {
    struct FormatVisitor {
        FormatStream& stream;

        void visit_use([[maybe_unused]] const Use& use) {
            stream.format("{}", use);
        }

        void visit_definition([[maybe_unused]] const Definition& definition) {
            stream.format("Definition(ir_id: {}, value: {})", definition.ir_id,
                definition.value);
        }
    };
    visit(FormatVisitor{stream});
}

void LinkItem::build_hash(Hasher& h) const {
    h.append(type());

    struct HashVisitor {
        Hasher& h;

        void visit_use([[maybe_unused]] const Use& use) { h.append(use); }

        void visit_definition([[maybe_unused]] const Definition& definition) {
            h.append(definition.ir_id).append(definition.value);
        }
    };
    return visit(HashVisitor{h});
}

bool operator==(const LinkItem& lhs, const LinkItem& rhs) {
    if (lhs.type() != rhs.type())
        return false;

    struct EqualityVisitor {
        const LinkItem& rhs;

        bool visit_use([[maybe_unused]] const LinkItem::Use& use) {
            [[maybe_unused]] const auto& other = rhs.as_use();
            return use == other;
        }

        bool visit_definition(
            [[maybe_unused]] const LinkItem::Definition& definition) {
            [[maybe_unused]] const auto& other = rhs.as_definition();
            return definition.ir_id == other.ir_id
                   && definition.value == other.value;
        }
    };
    return lhs.visit(EqualityVisitor{rhs});
}

bool operator!=(const LinkItem& lhs, const LinkItem& rhs) {
    return !(lhs == rhs);
}
// [[[end]]]

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

    result_.func.name(func_.name());
    result_.func.type(func_.type() == FunctionType::Closure
                          ? BytecodeFunctionType::Closure
                          : BytecodeFunctionType::Normal);
    result_.func.params(func_.param_count());
    result_.func.locals(locs_.total_registers());
    result_.refs_ = builder_.take_module_refs();
}

bool FunctionCompiler::visit(BlockID block) {
    if (seen_[block])
        return false;

    seen_[block] = true;
    stack_.push_back(block);
    return true;
}

void FunctionCompiler::compile_rvalue(const RValue& source, LocalID target) {
    struct Visitor {
        FunctionCompiler& self;
        LocalID target;

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

        void visit_constant(const Constant& constant) {
            self.compile_constant(constant, target);
        }

        void visit_outer_environment(const RValue::OuterEnvironment&) {
            self.builder().emit(
                BytecodeInstr::make_load_closure(self.value(target)));
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

        void visit_method_handle(const RValue::MethodHandle& h) {
            auto instance_value = self.value(h.instance);
            auto method_index = self.object().use_symbol(h.method);
            auto target_method = self.method(target);
            self.builder().emit(BytecodeInstr::make_load_method(instance_value,
                method_index, target_method.instance, target_method.function));
        }

        void visit_method_call(const RValue::MethodCall& c) {
            auto source_method = self.method(c.method);
            auto target_value = self.value(target);
            self.builder().emit(
                BytecodeInstr::make_push(source_method.instance));

            auto argc = push_args(c.args);
            self.builder().emit(
                BytecodeInstr::make_call_method(source_method.function, argc));
            self.builder().emit(BytecodeInstr::make_pop_to(target_value));
        }

        void visit_make_environment(const RValue::MakeEnvironment& e) {
            auto parent_value = self.value(e.parent);
            auto target_value = self.value(target);
            self.builder().emit(
                BytecodeInstr::make_env(parent_value, e.size, target_value));
        }

        void visit_make_closure(const RValue::MakeClosure& c) {
            auto tmpl_value = self.value(c.func);
            auto env_value = self.value(c.env);
            auto target_value = self.value(target);
            self.builder().emit(BytecodeInstr::make_closure(
                tmpl_value, env_value, target_value));
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
                self.builder().emit(
                    BytecodeInstr::make_append_format(arg_value, target_value));
            }

            self.builder().emit(
                BytecodeInstr::make_format_result(target_value, target_value));
        }

        u32 push_args(LocalListID list_id) {
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

void FunctionCompiler::compile_lvalue_read(
    const LValue& source, LocalID target) {
    struct Visitor {
        FunctionCompiler& self;
        LocalID target;

        void visit_param(const LValue::Param& p) {
            auto source_param = BytecodeParam(p.target.value());
            auto target_value = self.value(target);
            self.builder().emit(
                BytecodeInstr::make_load_param(source_param, target_value));
        }

        void visit_closure(const LValue::Closure& c) {
            auto env_value = self.value(c.env);
            auto target_value = self.value(target);
            self.builder().emit(BytecodeInstr::make_load_env(
                env_value, c.levels, c.index, target_value));
        }

        void visit_module(const LValue::Module& m) {
            auto source = self.object().use_member(m.member);
            auto target_value = self.value(target);
            self.builder().emit(
                BytecodeInstr::make_load_module(source, target_value));
        }

        void visit_field(const LValue::Field& f) {
            auto object_value = self.value(f.object);
            auto name = self.object().use_symbol(f.name);
            auto target_value = self.value(target);
            self.builder().emit(BytecodeInstr::make_load_member(
                object_value, name, target_value));
        }

        void visit_tuple_field(const LValue::TupleField& t) {
            auto tuple_value = self.value(t.object);
            auto target_value = self.value(target);
            self.builder().emit(BytecodeInstr::make_load_tuple_member(
                tuple_value, t.index, target_value));
        }

        void visit_index(const LValue::Index& i) {
            auto array_value = self.value(i.object);
            auto index_value = self.value(i.index);
            auto target_value = self.value(target);
            self.builder().emit(BytecodeInstr::make_load_index(
                array_value, index_value, target_value));
        }
    };
    source.visit(Visitor{*this, target});
}

void FunctionCompiler::compile_lvalue_write(
    LocalID source, const LValue& target) {
    struct Visitor {
        FunctionCompiler& self;
        BytecodeRegister source_value;

        void visit_param(const LValue::Param& p) {
            auto target_param = BytecodeParam(p.target.value());
            self.builder().emit(
                BytecodeInstr::make_store_param(source_value, target_param));
        }

        void visit_closure(const LValue::Closure& c) {
            auto env_value = self.value(c.env);
            self.builder().emit(BytecodeInstr::make_store_env(
                source_value, env_value, c.levels, c.index));
        }

        void visit_module(const LValue::Module& m) {
            auto target = self.object().use_member(m.member);
            self.builder().emit(
                BytecodeInstr::make_store_module(source_value, target));
        }

        void visit_field(const LValue::Field& f) {
            auto object_value = self.value(f.object);
            auto name = self.object().use_symbol(f.name);
            self.builder().emit(BytecodeInstr::make_store_member(
                source_value, object_value, name));
        }

        void visit_tuple_field(const LValue::TupleField& t) {
            auto tuple_value = self.value(t.object);
            self.builder().emit(BytecodeInstr::make_store_tuple_member(
                source_value, tuple_value, t.index));
        }

        void visit_index(const LValue::Index& i) {
            auto array_value = self.value(i.object);
            auto index_value = self.value(i.index);
            self.builder().emit(BytecodeInstr::make_store_index(
                source_value, array_value, index_value));
        }
    };
    target.visit(Visitor{*this, value(source)});
}

void FunctionCompiler::compile_constant(const Constant& c, LocalID target) {
    struct Visitor {
        FunctionCompiler& self;
        BytecodeRegister target_value;

        // Improvement: it might be useful to only pack small integers (e.g. up to 32 bit)
        // into the instruction stream and to store large integers as module level constants.
        void visit_integer(const Constant::Integer& i) {
            self.builder().emit(
                BytecodeInstr::make_load_int(i.value, target_value));
        }

        void visit_float(const Constant::Float& f) {
            self.builder().emit(
                BytecodeInstr::make_load_float(f.value, target_value));
        }

        void visit_string(const Constant::String& s) {
            auto id = self.object().use_string(s.value);
            self.builder().emit(
                BytecodeInstr::make_load_module(id, target_value));
        }

        void visit_symbol(const Constant::Symbol& s) {
            auto id = self.object().use_symbol(s.value);
            self.builder().emit(
                BytecodeInstr::make_load_module(id, target_value));
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
            self.builder().emit(
                BytecodeInstr::make_assert_fail(expr_value, message_value));
        }

        void visit_never(const Terminator::Never&) {}
    };
    term.visit(Visitor{*this});
}

void FunctionCompiler::compile_phi_operands(
    BlockID pred, const Terminator& term) {
    // Only normal jumps can transport phi operands. Critical edges are removed before codegen.
    if (term.type() != TerminatorType::Jump) {
#if defined(TIRO_DEBUG)
        visit_targets(term, [&](BlockID succ_id) {
            const size_t phi_count = func_[succ_id]->phi_count(func_);
            TIRO_DEBUG_ASSERT(phi_count == 0,
                "Successor with phi functions via non-jump edge.");
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

void FunctionCompiler::emit_copy(
    const CompiledLocation& source, const CompiledLocation& target) {
    TIRO_DEBUG_ASSERT(source.type() == target.type(),
        "Cannot copy between operands of different type.");

    struct Visitor {
        FunctionCompiler& self;
        const CompiledLocation& target;

        void visit_method(const CompiledLocation::Method& src_method) {
            const CompiledLocation::Method& dest_method = target.as_method();
            copy(src_method.instance, dest_method.instance);
            copy(src_method.function, dest_method.function);
        }

        void visit_value(BytecodeRegister src_local) {
            copy(src_local, target.as_value());
        }

        void copy(BytecodeRegister src, BytecodeRegister dest) {
            if (src != dest) {
                self.builder().emit(BytecodeInstr::make_copy(src, dest));
            }
        }
    };

    source.visit(Visitor{*this, target});
}

CompiledLocation FunctionCompiler::location(LocalID id) const {
    return locs_.get(id);
}

BytecodeRegister FunctionCompiler::value(LocalID id) const {
    auto loc = locs_.get(id);
    TIRO_CHECK(loc.type() == CompiledLocationType::Value,
        "Expected the virtual local {} to be mapped to a single physical "
        "local.",
        id);
    return loc.as_value();
}

CompiledLocation::Method FunctionCompiler::method(LocalID id) const {
    auto loc = locs_.get(id);
    TIRO_CHECK(loc.type() == CompiledLocationType::Method,
        "Expected the virtual local {} to be mapped to a method location.", id);
    return loc.as_method();
}

[[maybe_unused]] ModuleMemberID
FunctionCompiler::resolve_module_ref(LocalID local_id) {
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

            TIRO_ERROR(
                "{} did not resolve to a module member reference.", local_id);
            break;
        }
        default:
            TIRO_ERROR(
                "{} did not resolve to a module member reference.", local_id);
        }
    }
}

static LinkFunction
compile_function(const Module& module, Function& func, LinkObject& object) {
    split_critical_edges(func);
    //  construct_cssa(func);

    LinkFunction lf;
    FunctionCompiler compiler(module, func, lf, object);
    compiler.run();
    return lf;
}

LinkObject::LinkObject() {}

LinkObject::~LinkObject() {}

BytecodeMemberID LinkObject::use_integer(i64 value) {
    return add_member(
        LinkItem::make_definition({}, BytecodeMember::make_integer(value)));
}

BytecodeMemberID LinkObject::use_float(f64 value) {
    return add_member(
        LinkItem::make_definition({}, BytecodeMember::make_float(value)));
}

BytecodeMemberID LinkObject::use_string(InternedString value) {
    TIRO_DEBUG_ASSERT(value, "Invalid string.");
    return add_member(
        LinkItem::make_definition({}, BytecodeMember::make_string(value)));
}

BytecodeMemberID LinkObject::use_symbol(InternedString sym) {
    const auto str = use_string(sym);
    return add_member(
        LinkItem::make_definition({}, BytecodeMember::make_symbol(str)));
}

BytecodeMemberID LinkObject::use_member(ModuleMemberID ir_id) {
    return add_member(LinkItem::make_use(ir_id));
}

void LinkObject::define_import(
    ModuleMemberID ir_id, const BytecodeMember::Import& import) {
    add_member(LinkItem::make_definition(ir_id, import));
}

void LinkObject::define_variable(
    ModuleMemberID ir_id, const BytecodeMember::Variable& var) {
    add_member(LinkItem::make_definition(ir_id, var));
}

void LinkObject::define_function(ModuleMemberID ir_id, LinkFunction&& func) {
    auto func_id = functions_.push_back(std::move(func));
    add_member(
        LinkItem::make_definition(ir_id, BytecodeMember::Function{func_id}));
}

BytecodeMemberID LinkObject::add_member(const LinkItem& member) {
    if (auto pos = data_index_.find(member); pos != data_index_.end()) {
        return pos->second;
    }

    auto id = data_.push_back(member);
    data_index_[member] = id;
    return id;
}

LinkObject compile_object(Module& module, Span<const ModuleMemberID> members) {
    struct MemberVisitor {
        Module& module;
        LinkObject& object;
        ModuleMemberID id;

        void visit_import(const ModuleMember::Import& i) {
            auto name = object.use_string(i.name);
            object.define_import(id, BytecodeMember::Import(name));
        }

        void visit_variable(const ModuleMember::Variable& v) {
            // Initial value not implemented yet (always null).
            auto name = object.use_string(v.name);
            object.define_variable(id, BytecodeMember::Variable(name, {}));
        }

        void visit_function(const ModuleMember::Function& f) {
            auto func = module[f.id];
            object.define_function(id, compile_function(module, *func, object));
        }
    };

    LinkObject object;
    for (const auto id : members) {
        auto member = module[id];
        member->visit(MemberVisitor{module, object, id});
    }
    return object;
}

} // namespace tiro
