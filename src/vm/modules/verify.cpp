#include "vm/modules/verify.hpp"

#include "bytecode/function.hpp"
#include "bytecode/module.hpp"
#include "bytecode/reader.hpp"

#include <absl/container/flat_hash_set.h>

#include <algorithm>

namespace tiro::vm {

namespace {

class ModuleVerifier final {
public:
    explicit ModuleVerifier(const BytecodeModule& module)
        : module_(module) {}

    void verify();

    // Performs bounds checking on the given member id
    // and returns a reference to the member on success.
    const BytecodeMember& check_reference(BytecodeMemberId id, BytecodeMemberId parent);
    [[noreturn]] TIRO_COLD void fail(std::string_view message);
    const BytecodeModule& module() const { return module_; }

    // module member visitor
    void visit_integer(const BytecodeMember::Integer& i, BytecodeMemberId id);
    void visit_float(const BytecodeMember::Float& f, BytecodeMemberId id);
    void visit_string(const BytecodeMember::String& s, BytecodeMemberId id);
    void visit_symbol(const BytecodeMember::Symbol& s, BytecodeMemberId id);
    void visit_import(const BytecodeMember::Import& i, BytecodeMemberId id);
    void visit_variable(const BytecodeMember::Variable& v, BytecodeMemberId id);
    void visit_function(const BytecodeMember::Function& f, BytecodeMemberId id);
    void visit_record_template(const BytecodeMember::RecordTemplate& r, BytecodeMemberId id);

private:
    const BytecodeModule& module_;
    u32 seen_member_ids_ = 0;
};

class FunctionVerifier final {
public:
    explicit FunctionVerifier(
        BytecodeMemberId id, const BytecodeFunction& function, ModuleVerifier& parent)
        : id_(id)
        , function_(function)
        , parent_(parent) {
        TIRO_DEBUG_ASSERT(id_, "invalid function member id");
    }

    void verify();

    const BytecodeMember& check_reference(BytecodeMemberId id) {
        return parent_.check_reference(id, id_);
    }

    [[noreturn]] TIRO_COLD void fail(std::string_view message) {
        parent_.fail(fmt::format("{} (in function member {})", message, id_.value()));
    }

private:
    static_assert(std::is_same_v<BytecodeOffset::UnderlyingType, u32>);

    struct InsEntry {
        u32 offset; // byte offset of the instruction's start
        BytecodeInstr ins;
    };

    std::vector<InsEntry> read_instructions();

    struct InstructionVisitor {
        FunctionVerifier& self;

        /* [[[cog
            from cog import outl
            from codegen.bytecode import Instruction
            for member in Instruction.members:
                outl(f"void {member.visit_name}(const BytecodeInstr::{member.name}& {member.argument_name});")
        ]]] */
        void visit_load_null(const BytecodeInstr::LoadNull& load_null);
        void visit_load_false(const BytecodeInstr::LoadFalse& load_false);
        void visit_load_true(const BytecodeInstr::LoadTrue& load_true);
        void visit_load_int(const BytecodeInstr::LoadInt& load_int);
        void visit_load_float(const BytecodeInstr::LoadFloat& load_float);
        void visit_load_param(const BytecodeInstr::LoadParam& load_param);
        void visit_store_param(const BytecodeInstr::StoreParam& store_param);
        void visit_load_module(const BytecodeInstr::LoadModule& load_module);
        void visit_store_module(const BytecodeInstr::StoreModule& store_module);
        void visit_load_member(const BytecodeInstr::LoadMember& load_member);
        void visit_store_member(const BytecodeInstr::StoreMember& store_member);
        void visit_load_tuple_member(const BytecodeInstr::LoadTupleMember& load_tuple_member);
        void visit_store_tuple_member(const BytecodeInstr::StoreTupleMember& store_tuple_member);
        void visit_load_index(const BytecodeInstr::LoadIndex& load_index);
        void visit_store_index(const BytecodeInstr::StoreIndex& store_index);
        void visit_load_closure(const BytecodeInstr::LoadClosure& load_closure);
        void visit_load_env(const BytecodeInstr::LoadEnv& load_env);
        void visit_store_env(const BytecodeInstr::StoreEnv& store_env);
        void visit_add(const BytecodeInstr::Add& add);
        void visit_sub(const BytecodeInstr::Sub& sub);
        void visit_mul(const BytecodeInstr::Mul& mul);
        void visit_div(const BytecodeInstr::Div& div);
        void visit_mod(const BytecodeInstr::Mod& mod);
        void visit_pow(const BytecodeInstr::Pow& pow);
        void visit_uadd(const BytecodeInstr::UAdd& uadd);
        void visit_uneg(const BytecodeInstr::UNeg& uneg);
        void visit_lsh(const BytecodeInstr::LSh& lsh);
        void visit_rsh(const BytecodeInstr::RSh& rsh);
        void visit_band(const BytecodeInstr::BAnd& band);
        void visit_bor(const BytecodeInstr::BOr& bor);
        void visit_bxor(const BytecodeInstr::BXor& bxor);
        void visit_bnot(const BytecodeInstr::BNot& bnot);
        void visit_gt(const BytecodeInstr::Gt& gt);
        void visit_gte(const BytecodeInstr::Gte& gte);
        void visit_lt(const BytecodeInstr::Lt& lt);
        void visit_lte(const BytecodeInstr::Lte& lte);
        void visit_eq(const BytecodeInstr::Eq& eq);
        void visit_neq(const BytecodeInstr::NEq& neq);
        void visit_lnot(const BytecodeInstr::LNot& lnot);
        void visit_array(const BytecodeInstr::Array& array);
        void visit_tuple(const BytecodeInstr::Tuple& tuple);
        void visit_set(const BytecodeInstr::Set& set);
        void visit_map(const BytecodeInstr::Map& map);
        void visit_env(const BytecodeInstr::Env& env);
        void visit_closure(const BytecodeInstr::Closure& closure);
        void visit_record(const BytecodeInstr::Record& record);
        void visit_iterator(const BytecodeInstr::Iterator& iterator);
        void visit_iterator_next(const BytecodeInstr::IteratorNext& iterator_next);
        void visit_formatter(const BytecodeInstr::Formatter& formatter);
        void visit_append_format(const BytecodeInstr::AppendFormat& append_format);
        void visit_format_result(const BytecodeInstr::FormatResult& format_result);
        void visit_copy(const BytecodeInstr::Copy& copy);
        void visit_swap(const BytecodeInstr::Swap& swap);
        void visit_push(const BytecodeInstr::Push& push);
        void visit_pop(const BytecodeInstr::Pop& pop);
        void visit_pop_to(const BytecodeInstr::PopTo& pop_to);
        void visit_jmp(const BytecodeInstr::Jmp& jmp);
        void visit_jmp_true(const BytecodeInstr::JmpTrue& jmp_true);
        void visit_jmp_false(const BytecodeInstr::JmpFalse& jmp_false);
        void visit_jmp_null(const BytecodeInstr::JmpNull& jmp_null);
        void visit_jmp_not_null(const BytecodeInstr::JmpNotNull& jmp_not_null);
        void visit_call(const BytecodeInstr::Call& call);
        void visit_load_method(const BytecodeInstr::LoadMethod& load_method);
        void visit_call_method(const BytecodeInstr::CallMethod& call_method);
        void visit_return(const BytecodeInstr::Return& ret);
        void visit_rethrow(const BytecodeInstr::Rethrow& rethrow);
        void visit_assert_fail(const BytecodeInstr::AssertFail& assert_fail);
        // [[[end]]]
    };

    bool is_instruction_start(BytecodeOffset offset) {
        // instructions are ordered by offset
        auto begin = parsed_instructions_.begin();
        auto end = parsed_instructions_.end();
        auto it = std::lower_bound(begin, end, offset.value(),
            [](const InsEntry& lhs, u32 rhs) { return lhs.offset < rhs; });
        return it != end && it->offset == offset.value();
    }

    void check(BytecodeOffset target);
    void check(BytecodeParam param);
    void check(BytecodeRegister local);
    BytecodeMemberType check(BytecodeMemberId member);

private:
    BytecodeMemberId id_;
    const BytecodeFunction& function_;
    ModuleVerifier& parent_;
    std::vector<InsEntry> parsed_instructions_;
};

} // namespace

void ModuleVerifier::verify() {
    if (!module_.name())
        fail("module does not have a valid name");

    // Validate data
    for (const auto member_id : module_.member_ids()) {
        const auto& member = module_[member_id];
        member.visit(*this, member_id);
        ++seen_member_ids_;
    }

    // Validate functions
    for (const auto& member_id : module_.member_ids()) {
        const auto& member = module_[member_id];
        if (member.type() != BytecodeMemberType::Function)
            continue;

        FunctionVerifier verifier(member_id, module_[member.as_function().id], *this);
        verifier.verify();
    }

    // Validate initializer function (if present)
    if (auto init_id = module_.init()) {
        const auto& init = check_reference(init_id, {});
        if (init.type() != BytecodeMemberType::Function) {
            fail(fmt::format(
                "member {} is not a function (required by module init)", init_id.value()));
        }

        auto& func = module_[init.as_function().id];
        if (func.type() != BytecodeFunctionType::Normal) {
            fail(fmt::format(
                "member {} is not a normal function (required by module init)", init_id.value()));
        }
    }

    // Validate exports
    for (const auto [symbol_id, value_id] : module_.exports()) {
        const auto& symbol = check_reference(symbol_id, {});
        if (symbol.type() != BytecodeMemberType::Symbol) {
            fail(fmt::format(
                "member {} is not a symbol (required by usage as export name)", symbol_id.value()));
        }

        const auto& value = check_reference(value_id, {});
        switch (value.type()) {
        case BytecodeMemberType::Import:
        case BytecodeMemberType::RecordTemplate:
            fail("forbidden export of internal type");

        case BytecodeMemberType::Function: {
            auto& func = module_[value.as_function().id];
            if (func.type() != BytecodeFunctionType::Normal) {
                fail(fmt::format(
                    "member {} is not a normal function (required by export)", value_id.value()));
            }
        }

        default:
            break;
        }
    }
}

void ModuleVerifier::visit_integer(
    [[maybe_unused]] const BytecodeMember::Integer& i, [[maybe_unused]] BytecodeMemberId id) {}

void ModuleVerifier::visit_float(
    [[maybe_unused]] const BytecodeMember::Float& f, [[maybe_unused]] BytecodeMemberId id) {}

void ModuleVerifier::visit_string(const BytecodeMember::String& s, BytecodeMemberId id) {
    if (!s.value) {
        fail(fmt::format("invalid string (in member {})", id.value()));
    }
}

void ModuleVerifier::visit_symbol(const BytecodeMember::Symbol& s, BytecodeMemberId id) {
    const auto& name = check_reference(s.name, id);
    if (name.type() != BytecodeMemberType::String) {
        fail(fmt::format(
            "member {} is not a string (required by symbol at {})", s.name.value(), id.value()));
    }
}

void ModuleVerifier::visit_import(const BytecodeMember::Import& i, BytecodeMemberId id) {
    const auto& module_name = check_reference(i.module_name, id);
    if (module_name.type() != BytecodeMemberType::String) {
        fail(fmt::format("member {} is not a string (required by import at {})",
            i.module_name.value(), id.value()));
    }
}

void ModuleVerifier::visit_variable(
    [[maybe_unused]] const BytecodeMember::Variable& v, [[maybe_unused]] BytecodeMemberId id) {}

void ModuleVerifier::visit_function(const BytecodeMember::Function& f, BytecodeMemberId id) {
    if (!f.id) {
        fail(fmt::format("invalid function reference (in member {})", id.value()));
    }

    const auto& func = module_[f.id];
    if (auto name_id = func.name()) {
        const auto& name = check_reference(func.name(), id);
        if (name.type() != BytecodeMemberType::String) {
            fail(fmt::format("member {} is not a string (required by function at {})",
                name_id.value(), id.value()));
        }
    }

    // Code and handlers are verified when all members have been seen (see FunctionVerifier).
}

void ModuleVerifier::visit_record_template(
    const BytecodeMember::RecordTemplate& r, BytecodeMemberId id) {
    if (!r.id) {
        fail(fmt::format("invalid record template reference (in member {})", id.value()));
    }

    const auto& tmpl = module_[r.id];
    for (auto key_id : tmpl.keys()) {
        const auto& key = check_reference(key_id, id);
        if (key.type() != BytecodeMemberType::Symbol) {
            fail(fmt::format("member {} is not a symbol (required by record template at {})",
                key_id.value(), id.value()));
        }
    }
}

const BytecodeMember&
ModuleVerifier::check_reference(BytecodeMemberId id, BytecodeMemberId parent) {
    auto context = [&]() -> std::string {
        if (parent) {
            return fmt::format("(referenced by member {})", parent.value());
        }
        return "(referenced by module)";
    };

    if (!id)
        fail(fmt::format("invalid module member id {}", context()));

    if (!module_.members().in_bounds(id))
        fail(fmt::format("member id {} is out of bounds {}", id.value(), context()));

    if (id.value() >= seen_member_ids_)
        fail(fmt::format("member id {} has not been visited yet {}", id.value(), context()));

    return module_[id];
}

void ModuleVerifier::fail(std::string_view message) {
    // TODO: Mark exception as bytecode verification failure, e.g. with a kind enum?
    std::string_view name = module_.strings().dump(module_.name());
    TIRO_ERROR("module '{}' verification error: {}", name, message);
}

void FunctionVerifier::verify() {
    if (function_.locals() > max_locals)
        fail(fmt::format("function uses too many locals ({} locals, maximum is {})",
            function_.locals(), max_locals));

    // Name has already been verified in ModuleVerifier#visit_function
    const auto& insts = parsed_instructions_ = read_instructions();

    // Verify individual instructions
    {
        InstructionVisitor visitor{*this};
        for (const auto& entry : insts) {
            const BytecodeInstr& ins = entry.ins;
            ins.visit(visitor);
        }
    }
    if (insts.empty()) {
        fail("function body must not be empty");
    }

    // Eliminates checking for end of bytecode in interpreter loop
    {
        auto back_type = insts.back().ins.type();
        switch (back_type) {
        case BytecodeOp::Return:
        case BytecodeOp::Rethrow:
        case BytecodeOp::Jmp:
        case BytecodeOp::AssertFail:
            break;
        default:
            fail("function body must end with a halting instruction");
            break;
        }
    }

    // Verify exception handler regions and jump destinations
    {
        const auto& handlers = function_.handlers();
        size_t size = handlers.size();
        for (size_t i = 0; i < size; ++i) {
            auto& current = handlers[i];
            auto* prev = i > 0 ? &handlers[i - 1] : nullptr;

            if (!current.from || !is_instruction_start(current.from))
                fail("invalid exception handler start instruction");
            if (prev && current.from.value() < prev->to.value())
                fail("exception handler entries must be ordered");

            // 'to' is exclusive and may point to the end of the code
            if (!current.to
                || !(is_instruction_start(current.to)
                     || current.to.value() == function_.code().size()))
                fail("invalid exception handler end instruction");
            if (current.to.value() <= current.from.value())
                fail("invalid exception handler interval");

            if (!current.target || !is_instruction_start(current.target))
                fail("invalid exception handler target instruction");
        }
    }
}

std::vector<FunctionVerifier::InsEntry> FunctionVerifier::read_instructions() {
    if (function_.code().size() >= std::numeric_limits<u32>::max())
        fail("bytecode too long");

    std::vector<FunctionVerifier::InsEntry> entries;
    BytecodeReader reader(function_.code());
    while (reader.remaining() > 0) {
        auto pos = reader.pos();
        auto decoded = reader.read();
        if (std::holds_alternative<BytecodeReaderError>(decoded)) {
            fail(fmt::format(
                "invalid bytecode: {}", message(std::get<BytecodeReaderError>(decoded))));
        }
        entries.push_back(InsEntry{static_cast<u32>(pos), std::get<BytecodeInstr>(decoded)});
    }
    return entries;
}

void FunctionVerifier::check(BytecodeOffset target) {
    if (!target)
        fail("invalid jump destination");

    if (!is_instruction_start(target))
        fail("jump destination does not point to the start of an instruction");
}

void FunctionVerifier::check(BytecodeParam param) {
    if (!param)
        fail("invalid parameter");

    const auto index = param.value();
    if (index >= function_.params())
        fail("parameter index out of bounds");
}

void FunctionVerifier::check(BytecodeRegister local) {
    if (!local)
        fail("invalid local");

    const auto index = local.value();
    if (index >= function_.locals())
        fail("local index out of bounds");
}

BytecodeMemberType FunctionVerifier::check(BytecodeMemberId member_id) {
    const auto& member = check_reference(member_id);
    return member.type();
}

void FunctionVerifier::InstructionVisitor::visit_load_null(
    const BytecodeInstr::LoadNull& load_null) {
    self.check(load_null.target);
}

void FunctionVerifier::InstructionVisitor::visit_load_false(
    const BytecodeInstr::LoadFalse& load_false) {
    self.check(load_false.target);
}

void FunctionVerifier::InstructionVisitor::visit_load_true(
    const BytecodeInstr::LoadTrue& load_true) {
    self.check(load_true.target);
}

void FunctionVerifier::InstructionVisitor::visit_load_int(const BytecodeInstr::LoadInt& load_int) {
    self.check(load_int.target);
}

void FunctionVerifier::InstructionVisitor::visit_load_float(
    const BytecodeInstr::LoadFloat& load_float) {
    self.check(load_float.target);
}

void FunctionVerifier::InstructionVisitor::visit_load_param(
    const BytecodeInstr::LoadParam& load_param) {
    self.check(load_param.source);
    self.check(load_param.target);
}

void FunctionVerifier::InstructionVisitor::visit_store_param(
    const BytecodeInstr::StoreParam& store_param) {
    self.check(store_param.source);
    self.check(store_param.target);
}

void FunctionVerifier::InstructionVisitor::visit_load_module(
    const BytecodeInstr::LoadModule& load_module) {
    self.check(load_module.source);
    self.check(load_module.target);
}

void FunctionVerifier::InstructionVisitor::visit_store_module(
    const BytecodeInstr::StoreModule& store_module) {
    self.check(store_module.source);
    self.check(store_module.target);
}

void FunctionVerifier::InstructionVisitor::visit_load_member(
    const BytecodeInstr::LoadMember& load_member) {
    self.check(load_member.object);
    if (self.check(load_member.name) != BytecodeMemberType::Symbol)
        self.fail("name in LoadMember instruction must reference a symbol");
    self.check(load_member.target);
}

void FunctionVerifier::InstructionVisitor::visit_store_member(
    const BytecodeInstr::StoreMember& store_member) {
    self.check(store_member.source);
    self.check(store_member.object);
    if (self.check(store_member.name) != BytecodeMemberType::Symbol)
        self.fail("name in StoreMember instruction must reference a symbol");
}

void FunctionVerifier::InstructionVisitor::visit_load_tuple_member(
    const BytecodeInstr::LoadTupleMember& load_tuple_member) {
    self.check(load_tuple_member.tuple);
    self.check(load_tuple_member.target);
}

void FunctionVerifier::InstructionVisitor::visit_store_tuple_member(
    const BytecodeInstr::StoreTupleMember& store_tuple_member) {
    self.check(store_tuple_member.source);
    self.check(store_tuple_member.tuple);
}

void FunctionVerifier::InstructionVisitor::visit_load_index(
    const BytecodeInstr::LoadIndex& load_index) {
    self.check(load_index.array);
    self.check(load_index.target);
}

void FunctionVerifier::InstructionVisitor::visit_store_index(
    const BytecodeInstr::StoreIndex& store_index) {
    self.check(store_index.source);
    self.check(store_index.array);
}

void FunctionVerifier::InstructionVisitor::visit_load_closure(
    const BytecodeInstr::LoadClosure& load_closure) {
    if (self.function_.type() != BytecodeFunctionType::Closure)
        self.fail("only closure functions can use the LoadClosure instruction");

    self.check(load_closure.target);
}

void FunctionVerifier::InstructionVisitor::visit_load_env(const BytecodeInstr::LoadEnv& load_env) {
    self.check(load_env.env);
    self.check(load_env.target);
}

void FunctionVerifier::InstructionVisitor::visit_store_env(
    const BytecodeInstr::StoreEnv& store_env) {
    self.check(store_env.source);
    self.check(store_env.env);
}

void FunctionVerifier::InstructionVisitor::visit_add(const BytecodeInstr::Add& add) {
    self.check(add.lhs);
    self.check(add.rhs);
    self.check(add.target);
}

void FunctionVerifier::InstructionVisitor::visit_sub(const BytecodeInstr::Sub& sub) {
    self.check(sub.lhs);
    self.check(sub.rhs);
    self.check(sub.target);
}

void FunctionVerifier::InstructionVisitor::visit_mul(const BytecodeInstr::Mul& mul) {
    self.check(mul.lhs);
    self.check(mul.rhs);
    self.check(mul.target);
}

void FunctionVerifier::InstructionVisitor::visit_div(const BytecodeInstr::Div& div) {
    self.check(div.lhs);
    self.check(div.rhs);
    self.check(div.target);
}

void FunctionVerifier::InstructionVisitor::visit_mod(const BytecodeInstr::Mod& mod) {
    self.check(mod.lhs);
    self.check(mod.rhs);
    self.check(mod.target);
}

void FunctionVerifier::InstructionVisitor::visit_pow(const BytecodeInstr::Pow& pow) {
    self.check(pow.lhs);
    self.check(pow.rhs);
    self.check(pow.target);
}

void FunctionVerifier::InstructionVisitor::visit_uadd(const BytecodeInstr::UAdd& uadd) {
    self.check(uadd.value);
    self.check(uadd.target);
}

void FunctionVerifier::InstructionVisitor::visit_uneg(const BytecodeInstr::UNeg& uneg) {
    self.check(uneg.value);
    self.check(uneg.target);
}

void FunctionVerifier::InstructionVisitor::visit_lsh(const BytecodeInstr::LSh& lsh) {
    self.check(lsh.lhs);
    self.check(lsh.rhs);
    self.check(lsh.target);
}

void FunctionVerifier::InstructionVisitor::visit_rsh(const BytecodeInstr::RSh& rsh) {
    self.check(rsh.lhs);
    self.check(rsh.rhs);
    self.check(rsh.target);
}

void FunctionVerifier::InstructionVisitor::visit_band(const BytecodeInstr::BAnd& band) {
    self.check(band.lhs);
    self.check(band.rhs);
    self.check(band.target);
}

void FunctionVerifier::InstructionVisitor::visit_bor(const BytecodeInstr::BOr& bor) {
    self.check(bor.lhs);
    self.check(bor.rhs);
    self.check(bor.target);
}

void FunctionVerifier::InstructionVisitor::visit_bxor(const BytecodeInstr::BXor& bxor) {
    self.check(bxor.lhs);
    self.check(bxor.rhs);
    self.check(bxor.target);
}

void FunctionVerifier::InstructionVisitor::visit_bnot(const BytecodeInstr::BNot& bnot) {
    self.check(bnot.value);
    self.check(bnot.target);
}

void FunctionVerifier::InstructionVisitor::visit_gt(const BytecodeInstr::Gt& gt) {
    self.check(gt.lhs);
    self.check(gt.rhs);
    self.check(gt.target);
}

void FunctionVerifier::InstructionVisitor::visit_gte(const BytecodeInstr::Gte& gte) {
    self.check(gte.lhs);
    self.check(gte.rhs);
    self.check(gte.target);
}

void FunctionVerifier::InstructionVisitor::visit_lt(const BytecodeInstr::Lt& lt) {
    self.check(lt.lhs);
    self.check(lt.rhs);
    self.check(lt.target);
}

void FunctionVerifier::InstructionVisitor::visit_lte(const BytecodeInstr::Lte& lte) {
    self.check(lte.lhs);
    self.check(lte.rhs);
    self.check(lte.target);
}

void FunctionVerifier::InstructionVisitor::visit_eq(const BytecodeInstr::Eq& eq) {
    self.check(eq.lhs);
    self.check(eq.rhs);
    self.check(eq.target);
}

void FunctionVerifier::InstructionVisitor::visit_neq(const BytecodeInstr::NEq& neq) {
    self.check(neq.lhs);
    self.check(neq.rhs);
    self.check(neq.target);
}

void FunctionVerifier::InstructionVisitor::visit_lnot(const BytecodeInstr::LNot& lnot) {
    self.check(lnot.value);
    self.check(lnot.target);
}

void FunctionVerifier::InstructionVisitor::visit_array(const BytecodeInstr::Array& array) {
    self.check(array.target);

    if (array.count > max_container_args)
        self.fail("Too many arguments in array construction");
}

void FunctionVerifier::InstructionVisitor::visit_tuple(const BytecodeInstr::Tuple& tuple) {
    self.check(tuple.target);

    if (tuple.count > max_container_args)
        self.fail("Too many arguments in tuple construction");
}

void FunctionVerifier::InstructionVisitor::visit_set(const BytecodeInstr::Set& set) {
    self.check(set.target);

    if (set.count > max_container_args)
        self.fail("Too many arguments in set construction");
}

void FunctionVerifier::InstructionVisitor::visit_map(const BytecodeInstr::Map& map) {
    self.check(map.target);

    if (map.count % 2 != 0)
        self.fail("Map instruction must specify an even number of keys and values");
    if (map.count > max_container_args)
        self.fail("Too many arguments in map construction");
}

void FunctionVerifier::InstructionVisitor::visit_env(const BytecodeInstr::Env& env) {
    self.check(env.parent);
    self.check(env.target);
}

void FunctionVerifier::InstructionVisitor::visit_closure(const BytecodeInstr::Closure& closure) {
    self.check(closure.tmpl);
    self.check(closure.env);
    self.check(closure.target);

    auto& member = self.check_reference(closure.tmpl);
    if (member.type() != BytecodeMemberType::Function)
        self.fail("Closure instruction must reference a closure function");

    auto& func = self.parent_.module()[member.as_function().id];
    if (func.type() != BytecodeFunctionType::Closure)
        self.fail("Closure instruction must reference a closure function");
}

void FunctionVerifier::InstructionVisitor::visit_record(const BytecodeInstr::Record& record) {
    if (self.check(record.tmpl) != BytecodeMemberType::RecordTemplate)
        self.fail("Record instruction must reference a record template");
    self.check(record.target);
}

void FunctionVerifier::InstructionVisitor::visit_iterator(const BytecodeInstr::Iterator& iterator) {
    self.check(iterator.container);
    self.check(iterator.target);
}

void FunctionVerifier::InstructionVisitor::visit_iterator_next(
    const BytecodeInstr::IteratorNext& iterator_next) {
    self.check(iterator_next.iterator);
    self.check(iterator_next.valid);
    self.check(iterator_next.value);
}

void FunctionVerifier::InstructionVisitor::visit_formatter(
    const BytecodeInstr::Formatter& formatter) {
    self.check(formatter.target);
}

void FunctionVerifier::InstructionVisitor::visit_append_format(
    const BytecodeInstr::AppendFormat& append_format) {
    self.check(append_format.value);
    self.check(append_format.formatter);
}

void FunctionVerifier::InstructionVisitor::visit_format_result(
    const BytecodeInstr::FormatResult& format_result) {
    self.check(format_result.formatter);
    self.check(format_result.target);
}

void FunctionVerifier::InstructionVisitor::visit_copy(const BytecodeInstr::Copy& copy) {
    self.check(copy.source);
    self.check(copy.target);
}

void FunctionVerifier::InstructionVisitor::visit_swap(const BytecodeInstr::Swap& swap) {
    self.check(swap.a);
    self.check(swap.b);
}

void FunctionVerifier::InstructionVisitor::visit_push(const BytecodeInstr::Push& push) {
    self.check(push.value);
}

void FunctionVerifier::InstructionVisitor::visit_pop(const BytecodeInstr::Pop& pop) {
    (void) pop;
}

void FunctionVerifier::InstructionVisitor::visit_pop_to(const BytecodeInstr::PopTo& pop_to) {
    self.check(pop_to.target);
}

void FunctionVerifier::InstructionVisitor::visit_jmp(const BytecodeInstr::Jmp& jmp) {
    self.check(jmp.offset);
}

void FunctionVerifier::InstructionVisitor::visit_jmp_true(const BytecodeInstr::JmpTrue& jmp_true) {
    self.check(jmp_true.condition);
    self.check(jmp_true.offset);
}

void FunctionVerifier::InstructionVisitor::visit_jmp_false(
    const BytecodeInstr::JmpFalse& jmp_false) {
    self.check(jmp_false.condition);
    self.check(jmp_false.offset);
}

void FunctionVerifier::InstructionVisitor::visit_jmp_null(const BytecodeInstr::JmpNull& jmp_null) {
    self.check(jmp_null.condition);
    self.check(jmp_null.offset);
}

void FunctionVerifier::InstructionVisitor::visit_jmp_not_null(
    const BytecodeInstr::JmpNotNull& jmp_not_null) {
    self.check(jmp_not_null.condition);
    self.check(jmp_not_null.offset);
}

void FunctionVerifier::InstructionVisitor::visit_call(const BytecodeInstr::Call& call) {
    self.check(call.function);
}

void FunctionVerifier::InstructionVisitor::visit_load_method(
    const BytecodeInstr::LoadMethod& load_method) {
    self.check(load_method.object);
    if (self.check(load_method.name) != BytecodeMemberType::Symbol)
        self.fail("name in LoadMethod instruction must reference a symbol");
    self.check(load_method.thiz);
    self.check(load_method.method);
}

void FunctionVerifier::InstructionVisitor::visit_call_method(
    const BytecodeInstr::CallMethod& call_method) {
    self.check(call_method.method);
}

void FunctionVerifier::InstructionVisitor::visit_return(const BytecodeInstr::Return& ret) {
    self.check(ret.value);
}

void FunctionVerifier::InstructionVisitor::visit_rethrow(const BytecodeInstr::Rethrow& rethrow) {
    (void) rethrow;
    // TODO: Verify that we are inside a handler
}

void FunctionVerifier::InstructionVisitor::visit_assert_fail(
    const BytecodeInstr::AssertFail& assert_fail) {
    self.check(assert_fail.expr);
    self.check(assert_fail.message);
}

void verify_module(const BytecodeModule& module) {
    ModuleVerifier verifier(module);
    verifier.verify();
}

} // namespace tiro::vm
