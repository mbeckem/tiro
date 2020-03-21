#include "tiro/bytecode_gen/gen_func.hpp"

#include "tiro/bytecode_gen/bytecode_builder.hpp"
#include "tiro/bytecode_gen/locations.hpp"
#include "tiro/mir/construct_cssa.hpp"
#include "tiro/mir/critical_edges.hpp"
#include "tiro/mir/traversal.hpp"

#include <unordered_set>

namespace tiro {

namespace {

class FunctionCompiler final {
public:
    explicit FunctionCompiler(const Module& module, Function& func,
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
    CompiledLocations& locs() { return locs_; }
    Function& func() { return func_; }
    LinkFunction& result() { return result_; }
    LinkObject& object() { return object_; }

private:
    bool visit(BlockID block);

    void compile_rvalue(const RValue& value, CompiledLocalID target);
    void compile_lvalue_read(const LValue& value, CompiledLocalID target);
    void compile_lvalue_write(CompiledLocalID value, const LValue& target);
    void compile_constant(const Constant& constant, CompiledLocalID target);
    void compile_terminator(const Terminator& term);

    ModuleMemberID resolve_module_ref(LocalID local);

private:
    const Module& module_;
    Function& func_;
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
    const ModuleMemberID& mir_id, const CompiledModuleMember& value) {
    return Definition{mir_id, value};
}

LinkItem::LinkItem(const Use& use)
    : type_(LinkItemType::Use)
    , use_(use) {}

LinkItem::LinkItem(const Definition& definition)
    : type_(LinkItemType::Definition)
    , definition_(definition) {}

const LinkItem::Use& LinkItem::as_use() const {
    TIRO_ASSERT(type_ == LinkItemType::Use,
        "Bad member access on LinkItem: not a Use.");
    return use_;
}

const LinkItem::Definition& LinkItem::as_definition() const {
    TIRO_ASSERT(type_ == LinkItemType::Definition,
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
            stream.format("Definition(mir_id: {}, value: {})",
                definition.mir_id, definition.value);
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
            h.append(definition.mir_id).append(definition.value);
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
            return definition.mir_id == other.mir_id
                   && definition.value == other.value;
        }
    };
    return lhs.visit(EqualityVisitor{rhs});
}

bool operator!=(const LinkItem& lhs, const LinkItem& rhs) {
    return !(lhs == rhs);
}
// [[[end]]]

// Completely inefficient algorithm to leave ssa.
// Remove all critical edges, construct cssa and then
// assign every ssa variable its own local in the bytecode function,
// with the exceptions of phi arguments (they do not interfere and
// can share the same slot).
//
// TODO: Implement a real ssa elimination algorithm, e.g. by using the approach
// outlined in
//
//      Benoit Boissinot, Alain Darte, Fabrice Rastello, Benoît Dupont de Dinechin, Christophe Guillon.
//          Revisiting Out-of-SSA Translation for Correctness, Code Quality, and Efficiency.
//          [Research Report] 2008, pp.14. ￿inria-00349925v1
void FunctionCompiler::run() {
    split_critical_edges(func_);
    construct_cssa(func_);
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
                compile_lvalue_write(locs_.get(assign.value), assign.target);
                break;
            }
            case StmtType::Define:
                const auto& define = stmt.as_define();
                const auto local_id = define.local;
                compile_rvalue(func_[local_id]->value(), locs_.get(local_id));
                break;
            }
        }

        compile_terminator(block->terminator());
    }
    builder_.finish();

    result_.func.name(func_.name());
    result_.func.type(func_.type() == FunctionType::Closure
                          ? CompiledFunctionType::Closure
                          : CompiledFunctionType::Normal);
    result_.func.params(func_.param_count());
    result_.func.locals(locs_.physical_locals());
    result_.refs_ = builder_.take_module_refs();
}

bool FunctionCompiler::visit(BlockID block) {
    if (seen_[block])
        return false;

    seen_[block] = true;
    stack_.push_back(block);
    return true;
}

void FunctionCompiler::compile_rvalue(
    const RValue& value, CompiledLocalID target) {
    struct Visitor {
        FunctionCompiler& self;
        CompiledLocalID target;

        void visit_use_lvalue(const RValue::UseLValue& u) {
            self.compile_lvalue_read(u.target, target);
        }

        void visit_use_local(const RValue::UseLocal& u) {
            auto value = self.locs().get(u.target);
            if (target != value)
                self.builder().emit(Instruction::make_copy(value, target));
        }

        void visit_phi(const RValue::Phi&) {
            // Don't do anything. The phi node lifting algorithm (in cssa construction)
            // currently ensures that there will be a local ssa lvalue that
            // reads from the phi node.
            // The write part already happened with the definition of the phi argument.
        }

        void visit_phi0(const RValue::Phi0&) {}

        void visit_constant(const Constant& constant) {
            self.compile_constant(constant, target);
        }

        void visit_outer_environment(const RValue::OuterEnvironment&) {
            self.builder().emit(Instruction::make_load_closure(target));
        }

        void visit_binary_op(const RValue::BinaryOp& bin) {
            auto lhs = self.locs().get(bin.left);
            auto rhs = self.locs().get(bin.right);

            auto instruction = [&]() {
                switch (bin.op) {
#define TIRO_CASE(op, ins) \
    case BinaryOpType::op: \
        return Instruction::make_##ins(lhs, rhs, target);

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
            auto operand = self.locs().get(un.operand);

            auto instruction = [&]() {
                switch (un.op) {
#define TIRO_CASE(op, ins) \
    case UnaryOpType::op:  \
        return Instruction::make_##ins(operand, target);

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
            auto func = self.locs().get(c.func);
            auto argc = push_args(c.args);
            self.builder().emit(Instruction::make_call(func, argc, target));
        }

        void visit_method_handle(const RValue::MethodHandle&) {
            TIRO_NOT_IMPLEMENTED(); // FIXME
        }

        void visit_method_call(const RValue::MethodCall&) {
            TIRO_NOT_IMPLEMENTED(); // FIXME
        }

        void visit_make_environment(const RValue::MakeEnvironment& e) {
            auto parent = self.locs().get(e.parent);

            self.builder().emit(Instruction::make_env(parent, e.size, target));
        }

        void visit_make_closure(const RValue::MakeClosure& c) {
            auto tmpl = self.locs().get(c.func);
            auto env = self.locs().get(c.env);
            self.builder().emit(Instruction::make_closure(tmpl, env, target));
        }

        void visit_container(const RValue::Container& c) {
            auto argc = push_args(c.args);
            auto instruction = [&]() {
                switch (c.container) {
                case ContainerType::Array:
                    return Instruction::make_array(argc, target);
                case ContainerType::Tuple:
                    return Instruction::make_tuple(argc, target);
                case ContainerType::Set:
                    return Instruction::make_set(argc, target);
                case ContainerType::Map:
                    return Instruction::make_map(argc, target);
                }
                TIRO_UNREACHABLE("Invalid container type.");
            }();
            self.builder().emit(instruction);
        }

        void visit_format(const RValue::Format& f) {
            const auto args = self.func()[f.args];

            self.builder().emit(Instruction::make_formatter(target));
            for (const auto& mir_arg : *args) {
                auto arg = self.locs().get(mir_arg);
                self.builder().emit(
                    Instruction::make_append_format(arg, target));
            }
            self.builder().emit(
                Instruction::make_format_result(target, target));
        }

        u32 push_args(LocalListID list_id) {
            const auto args = self.func()[list_id];
            const u32 argc = args->size();
            for (const auto& mir_arg : *args) {
                auto arg = self.locs().get(mir_arg);
                self.builder().emit(Instruction::make_push(arg));
            }
            return argc;
        }
    };
    value.visit(Visitor{*this, target});
}

void FunctionCompiler::compile_lvalue_read(
    const LValue& value, CompiledLocalID target) {
    struct Visitor {
        FunctionCompiler& self;
        CompiledLocalID target;

        void visit_param(const LValue::Param& p) {
            CompiledParamID source(p.target.value());
            self.builder().emit(Instruction::make_load_param(source, target));
        }

        void visit_closure(const LValue::Closure& c) {
            auto env = self.locs().get(c.env);
            self.builder().emit(
                Instruction::make_load_env(env, c.levels, c.index, target));
        }

        void visit_module(const LValue::Module& m) {
            auto source = self.object().use_member(m.member);
            self.builder().emit(Instruction::make_load_module(source, target));
        }

        void visit_field(const LValue::Field& f) {
            auto object = self.locs().get(f.object);
            auto name = self.object().use_symbol(f.name);
            self.builder().emit(
                Instruction::make_load_member(object, name, target));
        }

        void visit_tuple_field(const LValue::TupleField& t) {
            auto tuple = self.locs().get(t.object);
            self.builder().emit(
                Instruction::make_load_tuple_member(tuple, t.index, target));
        }

        void visit_index(const LValue::Index& i) {
            auto array = self.locs().get(i.object);
            auto index = self.locs().get(i.index);
            self.builder().emit(
                Instruction::make_load_index(array, index, target));
        }
    };
    value.visit(Visitor{*this, target});
}

void FunctionCompiler::compile_lvalue_write(
    CompiledLocalID value, const LValue& target) {
    struct Visitor {
        FunctionCompiler& self;
        CompiledLocalID value;

        void visit_param(const LValue::Param& p) {
            CompiledParamID target(p.target.value());
            self.builder().emit(Instruction::make_store_param(value, target));
        }

        void visit_closure(const LValue::Closure& c) {
            auto env = self.locs().get(c.env);
            self.builder().emit(
                Instruction::make_store_env(value, env, c.levels, c.index));
        }

        void visit_module(const LValue::Module& m) {
            auto target = self.object().use_member(m.member);
            self.builder().emit(Instruction::make_store_module(value, target));
        }

        void visit_field(const LValue::Field& f) {
            auto object = self.locs().get(f.object);
            auto name = self.object().use_symbol(f.name);
            self.builder().emit(
                Instruction::make_store_member(value, object, name));
        }

        void visit_tuple_field(const LValue::TupleField& t) {
            auto tuple = self.locs().get(t.object);
            self.builder().emit(
                Instruction::make_store_tuple_member(value, tuple, t.index));
        }

        void visit_index(const LValue::Index& i) {
            auto array = self.locs().get(i.object);
            auto index = self.locs().get(i.index);
            self.builder().emit(
                Instruction::make_store_index(value, array, index));
        }
    };
    target.visit(Visitor{*this, value});
}

void FunctionCompiler::compile_constant(
    const Constant& c, CompiledLocalID target) {
    struct Visitor {
        FunctionCompiler& self;
        CompiledLocalID target;

        // Improvement: it might be useful to only pack small integers (e.g. up to 32 bit)
        // into the instruction stream and to store large integers as module level constants.
        void visit_integer(const Constant::Integer& i) {
            self.builder().emit(Instruction::make_load_int(i.value, target));
        }

        void visit_float(const Constant::Float& f) {
            self.builder().emit(Instruction::make_load_float(f.value, target));
        }

        void visit_string(const Constant::String& s) {
            auto id = self.object().use_string(s.value);
            self.builder().emit(Instruction::make_load_module(id, target));
        }

        void visit_symbol(const Constant::Symbol& s) {
            auto id = self.object().use_symbol(s.value);
            self.builder().emit(Instruction::make_load_module(id, target));
        }

        void visit_null(const Constant::Null&) {
            self.builder().emit(Instruction::make_load_null(target));
        }

        void visit_true(const Constant::True&) {
            self.builder().emit(Instruction::make_load_true(target));
        }

        void visit_false(const Constant::False&) {
            self.builder().emit(Instruction::make_load_false(target));
        }
    };
    c.visit(Visitor{*this, target});
}

void FunctionCompiler::compile_terminator(const Terminator& term) {
    struct Visitor {
        FunctionCompiler& self;

        void visit_none(const Terminator::None&) {}

        void visit_jump(const Terminator::Jump& j) {
            if (!self.visit(j.target)) {
                const auto offset = self.builder().use_label(j.target);
                self.builder().emit(Instruction::make_jmp(offset));
            }
        }

        void visit_branch(const Terminator::Branch& b) {
            const auto value = self.locs().get(b.value);
            {
                self.visit(b.target);
                const auto offset = self.builder().use_label(b.target);
                const auto ins = [&]() {
                    switch (b.type) {
                    case BranchType::IfTrue:
                        return Instruction::make_jmp_true(value, offset);
                    case BranchType::IfFalse:
                        return Instruction::make_jmp_false(value, offset);
                    }
                    TIRO_UNREACHABLE("Invalid branch type.");
                }();

                self.builder().emit(ins);
            }

            if (!self.visit(b.fallthrough)) {
                const auto offset = self.builder().use_label(b.fallthrough);
                self.builder().emit(Instruction::make_jmp(offset));
            }
        }

        void visit_return(const Terminator::Return& r) {
            auto value = self.locs().get(r.value);
            self.builder().emit(Instruction::make_return(value));
        }

        void visit_exit(const Terminator::Exit&) {}

        void visit_assert_fail(const Terminator::AssertFail& a) {
            auto expr = self.locs().get(a.expr);
            auto message = self.locs().get(a.message);
            self.builder().emit(Instruction::make_assert_fail(expr, message));
        }

        void visit_never(const Terminator::Never&) {}
    };
    term.visit(Visitor{*this});
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

LinkObject::LinkObject() {}

LinkObject::~LinkObject() {}

CompiledModuleMemberID LinkObject::use_integer(i64 value) {
    return add_member(LinkItem::make_definition(
        {}, CompiledModuleMember::make_integer(value)));
}

CompiledModuleMemberID LinkObject::use_float(f64 value) {
    return add_member(
        LinkItem::make_definition({}, CompiledModuleMember::make_float(value)));
}

CompiledModuleMemberID LinkObject::use_string(InternedString value) {
    TIRO_ASSERT(value, "Invalid string.");
    return add_member(LinkItem::make_definition(
        {}, CompiledModuleMember::make_string(value)));
}

CompiledModuleMemberID LinkObject::use_symbol(InternedString sym) {
    const auto str = use_string(sym);
    return add_member(
        LinkItem::make_definition({}, CompiledModuleMember::make_symbol(str)));
}

CompiledModuleMemberID LinkObject::use_member(ModuleMemberID mir_id) {
    return add_member(LinkItem::make_use(mir_id));
}

void LinkObject::define_import(
    ModuleMemberID mir_id, const CompiledModuleMember::Import& import) {
    add_member(LinkItem::make_definition(mir_id, import));
}

void LinkObject::define_variable(
    ModuleMemberID mir_id, const CompiledModuleMember::Variable& var) {
    add_member(LinkItem::make_definition(mir_id, var));
}

void LinkObject::define_function(ModuleMemberID mir_id, LinkFunction&& func) {
    auto func_id = functions_.push_back(std::move(func));
    add_member(LinkItem::make_definition(
        mir_id, CompiledModuleMember::Function{func_id}));
}

CompiledModuleMemberID LinkObject::add_member(const LinkItem& member) {
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
            object.define_import(id, CompiledModuleMember::Import(name));
        }

        void visit_variable(const ModuleMember::Variable& v) {
            // Initial value not implemented yet (always null).
            auto name = object.use_string(v.name);
            object.define_variable(
                id, CompiledModuleMember::Variable(name, {}));
        }

        void visit_function(const ModuleMember::Function& f) {
            auto func = module[f.id];

            LinkFunction lf;
            FunctionCompiler compiler(module, *func, lf, object);
            compiler.run();

            object.define_function(id, std::move(lf));
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
