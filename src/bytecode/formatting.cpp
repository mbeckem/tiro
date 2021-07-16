#include "bytecode/formatting.hpp"

#include "bytecode/function.hpp"
#include "bytecode/module.hpp"
#include "bytecode/op.hpp"
#include "bytecode/reader.hpp"
#include "common/assert.hpp"
#include "common/format.hpp"
#include "common/text/string_utils.hpp"

namespace tiro {

namespace {

template<typename Entity>
struct DumpHelper {
    Entity entity;

    void format(FormatStream& stream) const { dump_impl(entity, stream); }
};

class Disassembler final {
public:
    explicit Disassembler(Span<const byte> code, FormatStream& out)
        : in_(code)
        , out_(out) {
        max_column_length_ = fmt::formatted_size("{}", code.size() == 0 ? 0 : code.size() - 1);
    }

    void run();

private:
    void disassemble_instruction();

private:
    BytecodeReader in_;
    FormatStream& out_;
    size_t max_column_length_ = 0;
};

} // namespace

static void dump_impl(BytecodeMemberId id, FormatStream& stream) {
    if (!id) {
        stream.format("None");
    } else {
        stream.format("m:{}", id.value());
    }
}

static void dump_impl(BytecodeOffset offset, FormatStream& stream) {
    if (!offset) {
        stream.format("None");
    } else {
        stream.format("o:{}", offset.value());
    }
}

static void dump_impl(BytecodeParam reg, FormatStream& stream) {
    if (!reg) {
        stream.format("None");
    } else {
        stream.format("p:{}", reg.value());
    }
}

static void dump_impl(BytecodeRegister reg, FormatStream& stream) {
    if (!reg) {
        stream.format("None");
    } else {
        stream.format("l:{}", reg.value());
    }
}

template<typename Entity>
static DumpHelper<Entity> dump(Entity entity) {
    return {entity};
}

void format_record_template(const BytecodeRecordTemplate& tmpl, FormatStream& stream) {
    stream.format("Record template\n");
    for (const auto& key : tmpl.keys()) {
        stream.format("- {}\n", dump(key));
    }
}

void format_function(const BytecodeFunction& func, FormatStream& stream) {
    stream.format(
        "Function\n"
        "  Name: {}\n"
        "  Type: {}\n"
        "  Params: {}\n"
        "  Locals: {}\n"
        "\n",
        dump(func.name()), func.type(), func.params(), func.locals());

    stream.format("Code:\n");
    Disassembler(func.code(), stream).run();

    const auto& handlers = func.handlers();
    if (!handlers.empty()) {
        stream.format("\nHandlers:\n");
        for (const auto& handler : handlers) {
            stream.format("  from: {}, to: {}, target: {}\n", dump(handler.from), dump(handler.to),
                dump(handler.target));
        }
    }
}

void format_module(const BytecodeModule& module, FormatStream& stream) {
    struct MemberVisitor {
        const BytecodeModule& module;
        FormatStream& stream;

        void visit_integer(const BytecodeMember::Integer& i) {
            stream.format("Integer({})\n", i.value);
        }

        void visit_float(const BytecodeMember::Float& f) { stream.format("Float({})\n", f.value); }

        void visit_string(const BytecodeMember::String& s) {
            std::string_view str = module.strings().value(s.value);
            stream.format("String(\"{}\")\n", escape_string(str));
        }

        void visit_symbol(const BytecodeMember::Symbol& s) {
            stream.format("Symbol(name: {})\n", dump(s.name));
        }

        void visit_import(const BytecodeMember::Import& i) {
            stream.format("Import(module_name: {})\n", dump(i.module_name));
        }

        void visit_variable(const BytecodeMember::Variable& v) {
            stream.format("Variable(name: {})\n", dump(v.name));
        }

        void visit_function(const BytecodeMember::Function& f) {
            const auto& function = module[f.id];
            IndentStream indent(stream, 4, false);
            format_function(function, indent);
            stream.format("\n");
        }

        void visit_record_template(const BytecodeMember::RecordTemplate& r) {
            const auto& tmpl = module[r.id];
            IndentStream indent(stream, 4, false);
            format_record_template(tmpl, indent);
        }
    };

    stream.format(
        "Module\n"
        "  Name: {}\n"
        "  Members: {}\n"
        "  Functions: {}\n"
        "  Initializer: {}\n",
        module.strings().dump(module.name()), module.member_count(), module.function_count(),
        dump(module.init()));

    {
        stream.format("\nExports:\n");
        for (auto [symbol_id, value_id] : module.exports()) {
            stream.format("  {} -> {}\n", dump(symbol_id), dump(value_id));
        }
    }

    {
        stream.format("\nMembers:\n");
        const size_t member_count = module.member_count();
        const size_t max_index_length = fmt::formatted_size(
            "{}", member_count == 0 ? 0 : member_count - 1);

        size_t index = 0;
        for (auto member_id : module.member_ids()) {
            stream.format("  {index:>{width}}: ", fmt::arg("index", index),
                fmt::arg("width", max_index_length));

            auto& member = module[member_id];
            member.visit(MemberVisitor{module, stream});
            ++index;
        }
    }
}

void Disassembler::run() {
    while (in_.remaining() > 0) {
        disassemble_instruction();
        out_.format("\n");
    }
}

void Disassembler::disassemble_instruction() {
    const size_t start = in_.pos();
    out_.format(
        "{start:>{width}}: ", fmt::arg("start", start), fmt::arg("width", max_column_length_));

    auto result = in_.read();
    if (auto* error = std::get_if<BytecodeReaderError>(&result))
        TIRO_ERROR("invalid bytecode at offset {}: {}", start, message(*error));

    struct InstructionVisitor {
        FormatStream& out_;

        /* [[[cog
            from cog import outl
            from codegen.bytecode import Instruction, InstructionMap

            def dump_param(member, param):
                if param.cpp_type == param.raw_type:
                    return f"{member.argument_name}.{param.cpp_name}"
                return f"dump({member.argument_name}.{param.cpp_name})"

            for member in Instruction.members:
                ins = InstructionMap[member.name]

                outl(f"void {member.visit_name}(const BytecodeInstr::{member.name}& {member.argument_name}) {{")
                if not ins.params:
                    outl(f"    (void) {member.argument_name};");
                else:
                    format_string = " ".join(param.name + " {}" for param in ins.params)
                    format_args = ", ".join(dump_param(member, param) for param in ins.params)
                    cog.outl(f"    out_.format(\" {format_string}\", {format_args});")


                outl(f"}}")
                outl()
        ]]] */
        void visit_load_null(const BytecodeInstr::LoadNull& load_null) {
            out_.format(" target {}", dump(load_null.target));
        }

        void visit_load_false(const BytecodeInstr::LoadFalse& load_false) {
            out_.format(" target {}", dump(load_false.target));
        }

        void visit_load_true(const BytecodeInstr::LoadTrue& load_true) {
            out_.format(" target {}", dump(load_true.target));
        }

        void visit_load_int(const BytecodeInstr::LoadInt& load_int) {
            out_.format(" constant {} target {}", load_int.constant, dump(load_int.target));
        }

        void visit_load_float(const BytecodeInstr::LoadFloat& load_float) {
            out_.format(" constant {} target {}", load_float.constant, dump(load_float.target));
        }

        void visit_load_param(const BytecodeInstr::LoadParam& load_param) {
            out_.format(" source {} target {}", dump(load_param.source), dump(load_param.target));
        }

        void visit_store_param(const BytecodeInstr::StoreParam& store_param) {
            out_.format(" source {} target {}", dump(store_param.source), dump(store_param.target));
        }

        void visit_load_module(const BytecodeInstr::LoadModule& load_module) {
            out_.format(" source {} target {}", dump(load_module.source), dump(load_module.target));
        }

        void visit_store_module(const BytecodeInstr::StoreModule& store_module) {
            out_.format(
                " source {} target {}", dump(store_module.source), dump(store_module.target));
        }

        void visit_load_member(const BytecodeInstr::LoadMember& load_member) {
            out_.format(" object {} name {} target {}", dump(load_member.object),
                dump(load_member.name), dump(load_member.target));
        }

        void visit_store_member(const BytecodeInstr::StoreMember& store_member) {
            out_.format(" source {} object {} name {}", dump(store_member.source),
                dump(store_member.object), dump(store_member.name));
        }

        void visit_load_tuple_member(const BytecodeInstr::LoadTupleMember& load_tuple_member) {
            out_.format(" tuple {} index {} target {}", dump(load_tuple_member.tuple),
                load_tuple_member.index, dump(load_tuple_member.target));
        }

        void visit_store_tuple_member(const BytecodeInstr::StoreTupleMember& store_tuple_member) {
            out_.format(" source {} tuple {} index {}", dump(store_tuple_member.source),
                dump(store_tuple_member.tuple), store_tuple_member.index);
        }

        void visit_load_index(const BytecodeInstr::LoadIndex& load_index) {
            out_.format(" array {} index {} target {}", dump(load_index.array),
                dump(load_index.index), dump(load_index.target));
        }

        void visit_store_index(const BytecodeInstr::StoreIndex& store_index) {
            out_.format(" source {} array {} index {}", dump(store_index.source),
                dump(store_index.array), dump(store_index.index));
        }

        void visit_load_closure(const BytecodeInstr::LoadClosure& load_closure) {
            out_.format(" target {}", dump(load_closure.target));
        }

        void visit_load_env(const BytecodeInstr::LoadEnv& load_env) {
            out_.format(" env {} level {} index {} target {}", dump(load_env.env), load_env.level,
                load_env.index, dump(load_env.target));
        }

        void visit_store_env(const BytecodeInstr::StoreEnv& store_env) {
            out_.format(" source {} env {} level {} index {}", dump(store_env.source),
                dump(store_env.env), store_env.level, store_env.index);
        }

        void visit_add(const BytecodeInstr::Add& add) {
            out_.format(" lhs {} rhs {} target {}", dump(add.lhs), dump(add.rhs), dump(add.target));
        }

        void visit_sub(const BytecodeInstr::Sub& sub) {
            out_.format(" lhs {} rhs {} target {}", dump(sub.lhs), dump(sub.rhs), dump(sub.target));
        }

        void visit_mul(const BytecodeInstr::Mul& mul) {
            out_.format(" lhs {} rhs {} target {}", dump(mul.lhs), dump(mul.rhs), dump(mul.target));
        }

        void visit_div(const BytecodeInstr::Div& div) {
            out_.format(" lhs {} rhs {} target {}", dump(div.lhs), dump(div.rhs), dump(div.target));
        }

        void visit_mod(const BytecodeInstr::Mod& mod) {
            out_.format(" lhs {} rhs {} target {}", dump(mod.lhs), dump(mod.rhs), dump(mod.target));
        }

        void visit_pow(const BytecodeInstr::Pow& pow) {
            out_.format(" lhs {} rhs {} target {}", dump(pow.lhs), dump(pow.rhs), dump(pow.target));
        }

        void visit_uadd(const BytecodeInstr::UAdd& uadd) {
            out_.format(" value {} target {}", dump(uadd.value), dump(uadd.target));
        }

        void visit_uneg(const BytecodeInstr::UNeg& uneg) {
            out_.format(" value {} target {}", dump(uneg.value), dump(uneg.target));
        }

        void visit_lsh(const BytecodeInstr::LSh& lsh) {
            out_.format(" lhs {} rhs {} target {}", dump(lsh.lhs), dump(lsh.rhs), dump(lsh.target));
        }

        void visit_rsh(const BytecodeInstr::RSh& rsh) {
            out_.format(" lhs {} rhs {} target {}", dump(rsh.lhs), dump(rsh.rhs), dump(rsh.target));
        }

        void visit_band(const BytecodeInstr::BAnd& band) {
            out_.format(
                " lhs {} rhs {} target {}", dump(band.lhs), dump(band.rhs), dump(band.target));
        }

        void visit_bor(const BytecodeInstr::BOr& bor) {
            out_.format(" lhs {} rhs {} target {}", dump(bor.lhs), dump(bor.rhs), dump(bor.target));
        }

        void visit_bxor(const BytecodeInstr::BXor& bxor) {
            out_.format(
                " lhs {} rhs {} target {}", dump(bxor.lhs), dump(bxor.rhs), dump(bxor.target));
        }

        void visit_bnot(const BytecodeInstr::BNot& bnot) {
            out_.format(" value {} target {}", dump(bnot.value), dump(bnot.target));
        }

        void visit_gt(const BytecodeInstr::Gt& gt) {
            out_.format(" lhs {} rhs {} target {}", dump(gt.lhs), dump(gt.rhs), dump(gt.target));
        }

        void visit_gte(const BytecodeInstr::Gte& gte) {
            out_.format(" lhs {} rhs {} target {}", dump(gte.lhs), dump(gte.rhs), dump(gte.target));
        }

        void visit_lt(const BytecodeInstr::Lt& lt) {
            out_.format(" lhs {} rhs {} target {}", dump(lt.lhs), dump(lt.rhs), dump(lt.target));
        }

        void visit_lte(const BytecodeInstr::Lte& lte) {
            out_.format(" lhs {} rhs {} target {}", dump(lte.lhs), dump(lte.rhs), dump(lte.target));
        }

        void visit_eq(const BytecodeInstr::Eq& eq) {
            out_.format(" lhs {} rhs {} target {}", dump(eq.lhs), dump(eq.rhs), dump(eq.target));
        }

        void visit_neq(const BytecodeInstr::NEq& neq) {
            out_.format(" lhs {} rhs {} target {}", dump(neq.lhs), dump(neq.rhs), dump(neq.target));
        }

        void visit_lnot(const BytecodeInstr::LNot& lnot) {
            out_.format(" value {} target {}", dump(lnot.value), dump(lnot.target));
        }

        void visit_array(const BytecodeInstr::Array& array) {
            out_.format(" count {} target {}", array.count, dump(array.target));
        }

        void visit_tuple(const BytecodeInstr::Tuple& tuple) {
            out_.format(" count {} target {}", tuple.count, dump(tuple.target));
        }

        void visit_set(const BytecodeInstr::Set& set) {
            out_.format(" count {} target {}", set.count, dump(set.target));
        }

        void visit_map(const BytecodeInstr::Map& map) {
            out_.format(" count {} target {}", map.count, dump(map.target));
        }

        void visit_env(const BytecodeInstr::Env& env) {
            out_.format(
                " parent {} size {} target {}", dump(env.parent), env.size, dump(env.target));
        }

        void visit_closure(const BytecodeInstr::Closure& closure) {
            out_.format(" template {} env {} target {}", dump(closure.tmpl), dump(closure.env),
                dump(closure.target));
        }

        void visit_record(const BytecodeInstr::Record& record) {
            out_.format(" template {} target {}", dump(record.tmpl), dump(record.target));
        }

        void visit_iterator(const BytecodeInstr::Iterator& iterator) {
            out_.format(" container {} target {}", dump(iterator.container), dump(iterator.target));
        }

        void visit_iterator_next(const BytecodeInstr::IteratorNext& iterator_next) {
            out_.format(" iterator {} valid {} value {}", dump(iterator_next.iterator),
                dump(iterator_next.valid), dump(iterator_next.value));
        }

        void visit_formatter(const BytecodeInstr::Formatter& formatter) {
            out_.format(" target {}", dump(formatter.target));
        }

        void visit_append_format(const BytecodeInstr::AppendFormat& append_format) {
            out_.format(
                " value {} formatter {}", dump(append_format.value), dump(append_format.formatter));
        }

        void visit_format_result(const BytecodeInstr::FormatResult& format_result) {
            out_.format(" formatter {} target {}", dump(format_result.formatter),
                dump(format_result.target));
        }

        void visit_copy(const BytecodeInstr::Copy& copy) {
            out_.format(" source {} target {}", dump(copy.source), dump(copy.target));
        }

        void visit_swap(const BytecodeInstr::Swap& swap) {
            out_.format(" a {} b {}", dump(swap.a), dump(swap.b));
        }

        void visit_push(const BytecodeInstr::Push& push) {
            out_.format(" value {}", dump(push.value));
        }

        void visit_pop(const BytecodeInstr::Pop& pop) { (void) pop; }

        void visit_pop_to(const BytecodeInstr::PopTo& pop_to) {
            out_.format(" target {}", dump(pop_to.target));
        }

        void visit_jmp(const BytecodeInstr::Jmp& jmp) {
            out_.format(" offset {}", dump(jmp.offset));
        }

        void visit_jmp_true(const BytecodeInstr::JmpTrue& jmp_true) {
            out_.format(" condition {} offset {}", dump(jmp_true.condition), dump(jmp_true.offset));
        }

        void visit_jmp_false(const BytecodeInstr::JmpFalse& jmp_false) {
            out_.format(
                " condition {} offset {}", dump(jmp_false.condition), dump(jmp_false.offset));
        }

        void visit_jmp_null(const BytecodeInstr::JmpNull& jmp_null) {
            out_.format(" condition {} offset {}", dump(jmp_null.condition), dump(jmp_null.offset));
        }

        void visit_jmp_not_null(const BytecodeInstr::JmpNotNull& jmp_not_null) {
            out_.format(
                " condition {} offset {}", dump(jmp_not_null.condition), dump(jmp_not_null.offset));
        }

        void visit_call(const BytecodeInstr::Call& call) {
            out_.format(" function {} count {}", dump(call.function), call.count);
        }

        void visit_load_method(const BytecodeInstr::LoadMethod& load_method) {
            out_.format(" object {} name {} this {} method {}", dump(load_method.object),
                dump(load_method.name), dump(load_method.thiz), dump(load_method.method));
        }

        void visit_call_method(const BytecodeInstr::CallMethod& call_method) {
            out_.format(" method {} count {}", dump(call_method.method), call_method.count);
        }

        void visit_return(const BytecodeInstr::Return& ret) {
            out_.format(" value {}", dump(ret.value));
        }

        void visit_rethrow(const BytecodeInstr::Rethrow& rethrow) { (void) rethrow; }

        void visit_assert_fail(const BytecodeInstr::AssertFail& assert_fail) {
            out_.format(" expr {} message {}", dump(assert_fail.expr), dump(assert_fail.message));
        }

        // [[[end]]]
    };

    const auto& ins = std::get<BytecodeInstr>(result);
    out_.format("{}", ins.type());
    ins.visit(InstructionVisitor{out_});
}

} // namespace tiro

template<typename Entity>
struct tiro::EnableFormatMode<tiro::DumpHelper<Entity>> {
    static constexpr tiro::FormatMode value = tiro::FormatMode::MemberFormat;
};
