#include "bytecode/formatting.hpp"

#include "bytecode/function.hpp"
#include "bytecode/module.hpp"
#include "bytecode/op.hpp"
#include "common/assert.hpp"
#include "common/format.hpp"
#include "common/memory/binary.hpp"
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
    CheckedBinaryReader in_;
    FormatStream& out_;
    size_t max_column_length_ = 0;
};

} // namespace

static void dump_impl(BytecodeMemberId id, FormatStream& stream) {
    if (!id) {
        stream.format("None");
    } else {
        stream.format("m{}", id.value());
    }
}

static void dump_impl(BytecodeOffset offset, FormatStream& stream) {
    if (!offset) {
        stream.format("None");
    } else {
        stream.format("{}", offset.value());
    }
}

static void dump_impl(BytecodeParam reg, FormatStream& stream) {
    if (!reg) {
        stream.format("None");
    } else {
        stream.format("p{}", reg.value());
    }
}

static void dump_impl(BytecodeRegister reg, FormatStream& stream) {
    if (!reg) {
        stream.format("None");
    } else {
        stream.format("l{}", reg.value());
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
            format_function(*function, indent);
            stream.format("\n");
        }

        void visit_record_template(const BytecodeMember::RecordTemplate& r) {
            const auto& tmpl = module[r.id];
            IndentStream indent(stream, 4, false);
            format_record_template(*tmpl, indent);
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

            module[member_id]->visit(MemberVisitor{module, stream});
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

    const u8 raw_op = in_.read_u8();
    if (!valid_opcode(raw_op))
        TIRO_ERROR("Invalid opcode at offset {}: {}.", start, raw_op);

    const BytecodeOp op = static_cast<BytecodeOp>(raw_op);
    out_.format("{}", op);

    switch (op) {
    /* [[[cog
            import cog
            from codegen.bytecode import InstructionList

            def var_name(param):
                return f"p_{param.name}"

            def read_param(param):
                name = var_name(param)
                return f"const auto {name} = {param.cpp_type}(in_.read_{param.raw_type}());"

            def dump_param(param):
                name = var_name(param)
                if param.cpp_type == param.raw_type:
                    return name
                return f"dump({name})"

            for ins in InstructionList:
                cog.outl(f"case BytecodeOp::{ins.name}: {{")

                for param in ins.params:
                    cog.outl(read_param(param))

                def format_string():
                    return " ".join(param.name + " {}" for param in ins.params)

                def format_args():
                    return ", ".join(dump_param(param) for param in ins.params)
                
                if len(ins.params) > 0:
                    cog.outl(f"out_.format(\" {format_string()}\", {format_args()});")

                cog.outl(f"break;")
                cog.outl(f"}}")
        ]]] */
    case BytecodeOp::LoadNull: {
        const auto p_target = BytecodeRegister(in_.read_u32());
        out_.format(" target {}", dump(p_target));
        break;
    }
    case BytecodeOp::LoadFalse: {
        const auto p_target = BytecodeRegister(in_.read_u32());
        out_.format(" target {}", dump(p_target));
        break;
    }
    case BytecodeOp::LoadTrue: {
        const auto p_target = BytecodeRegister(in_.read_u32());
        out_.format(" target {}", dump(p_target));
        break;
    }
    case BytecodeOp::LoadInt: {
        const auto p_constant = i64(in_.read_i64());
        const auto p_target = BytecodeRegister(in_.read_u32());
        out_.format(" constant {} target {}", p_constant, dump(p_target));
        break;
    }
    case BytecodeOp::LoadFloat: {
        const auto p_constant = f64(in_.read_f64());
        const auto p_target = BytecodeRegister(in_.read_u32());
        out_.format(" constant {} target {}", p_constant, dump(p_target));
        break;
    }
    case BytecodeOp::LoadParam: {
        const auto p_source = BytecodeParam(in_.read_u32());
        const auto p_target = BytecodeRegister(in_.read_u32());
        out_.format(" source {} target {}", dump(p_source), dump(p_target));
        break;
    }
    case BytecodeOp::StoreParam: {
        const auto p_source = BytecodeRegister(in_.read_u32());
        const auto p_target = BytecodeParam(in_.read_u32());
        out_.format(" source {} target {}", dump(p_source), dump(p_target));
        break;
    }
    case BytecodeOp::LoadModule: {
        const auto p_source = BytecodeMemberId(in_.read_u32());
        const auto p_target = BytecodeRegister(in_.read_u32());
        out_.format(" source {} target {}", dump(p_source), dump(p_target));
        break;
    }
    case BytecodeOp::StoreModule: {
        const auto p_source = BytecodeRegister(in_.read_u32());
        const auto p_target = BytecodeMemberId(in_.read_u32());
        out_.format(" source {} target {}", dump(p_source), dump(p_target));
        break;
    }
    case BytecodeOp::LoadMember: {
        const auto p_object = BytecodeRegister(in_.read_u32());
        const auto p_name = BytecodeMemberId(in_.read_u32());
        const auto p_target = BytecodeRegister(in_.read_u32());
        out_.format(" object {} name {} target {}", dump(p_object), dump(p_name), dump(p_target));
        break;
    }
    case BytecodeOp::StoreMember: {
        const auto p_source = BytecodeRegister(in_.read_u32());
        const auto p_object = BytecodeRegister(in_.read_u32());
        const auto p_name = BytecodeMemberId(in_.read_u32());
        out_.format(" source {} object {} name {}", dump(p_source), dump(p_object), dump(p_name));
        break;
    }
    case BytecodeOp::LoadTupleMember: {
        const auto p_tuple = BytecodeRegister(in_.read_u32());
        const auto p_index = u32(in_.read_u32());
        const auto p_target = BytecodeRegister(in_.read_u32());
        out_.format(" tuple {} index {} target {}", dump(p_tuple), p_index, dump(p_target));
        break;
    }
    case BytecodeOp::StoreTupleMember: {
        const auto p_source = BytecodeRegister(in_.read_u32());
        const auto p_tuple = BytecodeRegister(in_.read_u32());
        const auto p_index = u32(in_.read_u32());
        out_.format(" source {} tuple {} index {}", dump(p_source), dump(p_tuple), p_index);
        break;
    }
    case BytecodeOp::LoadIndex: {
        const auto p_array = BytecodeRegister(in_.read_u32());
        const auto p_index = BytecodeRegister(in_.read_u32());
        const auto p_target = BytecodeRegister(in_.read_u32());
        out_.format(" array {} index {} target {}", dump(p_array), dump(p_index), dump(p_target));
        break;
    }
    case BytecodeOp::StoreIndex: {
        const auto p_source = BytecodeRegister(in_.read_u32());
        const auto p_array = BytecodeRegister(in_.read_u32());
        const auto p_index = BytecodeRegister(in_.read_u32());
        out_.format(" source {} array {} index {}", dump(p_source), dump(p_array), dump(p_index));
        break;
    }
    case BytecodeOp::LoadClosure: {
        const auto p_target = BytecodeRegister(in_.read_u32());
        out_.format(" target {}", dump(p_target));
        break;
    }
    case BytecodeOp::LoadEnv: {
        const auto p_env = BytecodeRegister(in_.read_u32());
        const auto p_level = u32(in_.read_u32());
        const auto p_index = u32(in_.read_u32());
        const auto p_target = BytecodeRegister(in_.read_u32());
        out_.format(
            " env {} level {} index {} target {}", dump(p_env), p_level, p_index, dump(p_target));
        break;
    }
    case BytecodeOp::StoreEnv: {
        const auto p_source = BytecodeRegister(in_.read_u32());
        const auto p_env = BytecodeRegister(in_.read_u32());
        const auto p_level = u32(in_.read_u32());
        const auto p_index = u32(in_.read_u32());
        out_.format(
            " source {} env {} level {} index {}", dump(p_source), dump(p_env), p_level, p_index);
        break;
    }
    case BytecodeOp::Add: {
        const auto p_lhs = BytecodeRegister(in_.read_u32());
        const auto p_rhs = BytecodeRegister(in_.read_u32());
        const auto p_target = BytecodeRegister(in_.read_u32());
        out_.format(" lhs {} rhs {} target {}", dump(p_lhs), dump(p_rhs), dump(p_target));
        break;
    }
    case BytecodeOp::Sub: {
        const auto p_lhs = BytecodeRegister(in_.read_u32());
        const auto p_rhs = BytecodeRegister(in_.read_u32());
        const auto p_target = BytecodeRegister(in_.read_u32());
        out_.format(" lhs {} rhs {} target {}", dump(p_lhs), dump(p_rhs), dump(p_target));
        break;
    }
    case BytecodeOp::Mul: {
        const auto p_lhs = BytecodeRegister(in_.read_u32());
        const auto p_rhs = BytecodeRegister(in_.read_u32());
        const auto p_target = BytecodeRegister(in_.read_u32());
        out_.format(" lhs {} rhs {} target {}", dump(p_lhs), dump(p_rhs), dump(p_target));
        break;
    }
    case BytecodeOp::Div: {
        const auto p_lhs = BytecodeRegister(in_.read_u32());
        const auto p_rhs = BytecodeRegister(in_.read_u32());
        const auto p_target = BytecodeRegister(in_.read_u32());
        out_.format(" lhs {} rhs {} target {}", dump(p_lhs), dump(p_rhs), dump(p_target));
        break;
    }
    case BytecodeOp::Mod: {
        const auto p_lhs = BytecodeRegister(in_.read_u32());
        const auto p_rhs = BytecodeRegister(in_.read_u32());
        const auto p_target = BytecodeRegister(in_.read_u32());
        out_.format(" lhs {} rhs {} target {}", dump(p_lhs), dump(p_rhs), dump(p_target));
        break;
    }
    case BytecodeOp::Pow: {
        const auto p_lhs = BytecodeRegister(in_.read_u32());
        const auto p_rhs = BytecodeRegister(in_.read_u32());
        const auto p_target = BytecodeRegister(in_.read_u32());
        out_.format(" lhs {} rhs {} target {}", dump(p_lhs), dump(p_rhs), dump(p_target));
        break;
    }
    case BytecodeOp::UAdd: {
        const auto p_value = BytecodeRegister(in_.read_u32());
        const auto p_target = BytecodeRegister(in_.read_u32());
        out_.format(" value {} target {}", dump(p_value), dump(p_target));
        break;
    }
    case BytecodeOp::UNeg: {
        const auto p_value = BytecodeRegister(in_.read_u32());
        const auto p_target = BytecodeRegister(in_.read_u32());
        out_.format(" value {} target {}", dump(p_value), dump(p_target));
        break;
    }
    case BytecodeOp::LSh: {
        const auto p_lhs = BytecodeRegister(in_.read_u32());
        const auto p_rhs = BytecodeRegister(in_.read_u32());
        const auto p_target = BytecodeRegister(in_.read_u32());
        out_.format(" lhs {} rhs {} target {}", dump(p_lhs), dump(p_rhs), dump(p_target));
        break;
    }
    case BytecodeOp::RSh: {
        const auto p_lhs = BytecodeRegister(in_.read_u32());
        const auto p_rhs = BytecodeRegister(in_.read_u32());
        const auto p_target = BytecodeRegister(in_.read_u32());
        out_.format(" lhs {} rhs {} target {}", dump(p_lhs), dump(p_rhs), dump(p_target));
        break;
    }
    case BytecodeOp::BAnd: {
        const auto p_lhs = BytecodeRegister(in_.read_u32());
        const auto p_rhs = BytecodeRegister(in_.read_u32());
        const auto p_target = BytecodeRegister(in_.read_u32());
        out_.format(" lhs {} rhs {} target {}", dump(p_lhs), dump(p_rhs), dump(p_target));
        break;
    }
    case BytecodeOp::BOr: {
        const auto p_lhs = BytecodeRegister(in_.read_u32());
        const auto p_rhs = BytecodeRegister(in_.read_u32());
        const auto p_target = BytecodeRegister(in_.read_u32());
        out_.format(" lhs {} rhs {} target {}", dump(p_lhs), dump(p_rhs), dump(p_target));
        break;
    }
    case BytecodeOp::BXor: {
        const auto p_lhs = BytecodeRegister(in_.read_u32());
        const auto p_rhs = BytecodeRegister(in_.read_u32());
        const auto p_target = BytecodeRegister(in_.read_u32());
        out_.format(" lhs {} rhs {} target {}", dump(p_lhs), dump(p_rhs), dump(p_target));
        break;
    }
    case BytecodeOp::BNot: {
        const auto p_value = BytecodeRegister(in_.read_u32());
        const auto p_target = BytecodeRegister(in_.read_u32());
        out_.format(" value {} target {}", dump(p_value), dump(p_target));
        break;
    }
    case BytecodeOp::Gt: {
        const auto p_lhs = BytecodeRegister(in_.read_u32());
        const auto p_rhs = BytecodeRegister(in_.read_u32());
        const auto p_target = BytecodeRegister(in_.read_u32());
        out_.format(" lhs {} rhs {} target {}", dump(p_lhs), dump(p_rhs), dump(p_target));
        break;
    }
    case BytecodeOp::Gte: {
        const auto p_lhs = BytecodeRegister(in_.read_u32());
        const auto p_rhs = BytecodeRegister(in_.read_u32());
        const auto p_target = BytecodeRegister(in_.read_u32());
        out_.format(" lhs {} rhs {} target {}", dump(p_lhs), dump(p_rhs), dump(p_target));
        break;
    }
    case BytecodeOp::Lt: {
        const auto p_lhs = BytecodeRegister(in_.read_u32());
        const auto p_rhs = BytecodeRegister(in_.read_u32());
        const auto p_target = BytecodeRegister(in_.read_u32());
        out_.format(" lhs {} rhs {} target {}", dump(p_lhs), dump(p_rhs), dump(p_target));
        break;
    }
    case BytecodeOp::Lte: {
        const auto p_lhs = BytecodeRegister(in_.read_u32());
        const auto p_rhs = BytecodeRegister(in_.read_u32());
        const auto p_target = BytecodeRegister(in_.read_u32());
        out_.format(" lhs {} rhs {} target {}", dump(p_lhs), dump(p_rhs), dump(p_target));
        break;
    }
    case BytecodeOp::Eq: {
        const auto p_lhs = BytecodeRegister(in_.read_u32());
        const auto p_rhs = BytecodeRegister(in_.read_u32());
        const auto p_target = BytecodeRegister(in_.read_u32());
        out_.format(" lhs {} rhs {} target {}", dump(p_lhs), dump(p_rhs), dump(p_target));
        break;
    }
    case BytecodeOp::NEq: {
        const auto p_lhs = BytecodeRegister(in_.read_u32());
        const auto p_rhs = BytecodeRegister(in_.read_u32());
        const auto p_target = BytecodeRegister(in_.read_u32());
        out_.format(" lhs {} rhs {} target {}", dump(p_lhs), dump(p_rhs), dump(p_target));
        break;
    }
    case BytecodeOp::LNot: {
        const auto p_value = BytecodeRegister(in_.read_u32());
        const auto p_target = BytecodeRegister(in_.read_u32());
        out_.format(" value {} target {}", dump(p_value), dump(p_target));
        break;
    }
    case BytecodeOp::Array: {
        const auto p_count = u32(in_.read_u32());
        const auto p_target = BytecodeRegister(in_.read_u32());
        out_.format(" count {} target {}", p_count, dump(p_target));
        break;
    }
    case BytecodeOp::Tuple: {
        const auto p_count = u32(in_.read_u32());
        const auto p_target = BytecodeRegister(in_.read_u32());
        out_.format(" count {} target {}", p_count, dump(p_target));
        break;
    }
    case BytecodeOp::Set: {
        const auto p_count = u32(in_.read_u32());
        const auto p_target = BytecodeRegister(in_.read_u32());
        out_.format(" count {} target {}", p_count, dump(p_target));
        break;
    }
    case BytecodeOp::Map: {
        const auto p_count = u32(in_.read_u32());
        const auto p_target = BytecodeRegister(in_.read_u32());
        out_.format(" count {} target {}", p_count, dump(p_target));
        break;
    }
    case BytecodeOp::Env: {
        const auto p_parent = BytecodeRegister(in_.read_u32());
        const auto p_size = u32(in_.read_u32());
        const auto p_target = BytecodeRegister(in_.read_u32());
        out_.format(" parent {} size {} target {}", dump(p_parent), p_size, dump(p_target));
        break;
    }
    case BytecodeOp::Closure: {
        const auto p_template = BytecodeRegister(in_.read_u32());
        const auto p_env = BytecodeRegister(in_.read_u32());
        const auto p_target = BytecodeRegister(in_.read_u32());
        out_.format(" template {} env {} target {}", dump(p_template), dump(p_env), dump(p_target));
        break;
    }
    case BytecodeOp::Record: {
        const auto p_template = BytecodeMemberId(in_.read_u32());
        const auto p_target = BytecodeRegister(in_.read_u32());
        out_.format(" template {} target {}", dump(p_template), dump(p_target));
        break;
    }
    case BytecodeOp::Iterator: {
        const auto p_container = BytecodeRegister(in_.read_u32());
        const auto p_target = BytecodeRegister(in_.read_u32());
        out_.format(" container {} target {}", dump(p_container), dump(p_target));
        break;
    }
    case BytecodeOp::IteratorNext: {
        const auto p_iterator = BytecodeRegister(in_.read_u32());
        const auto p_valid = BytecodeRegister(in_.read_u32());
        const auto p_value = BytecodeRegister(in_.read_u32());
        out_.format(
            " iterator {} valid {} value {}", dump(p_iterator), dump(p_valid), dump(p_value));
        break;
    }
    case BytecodeOp::Formatter: {
        const auto p_target = BytecodeRegister(in_.read_u32());
        out_.format(" target {}", dump(p_target));
        break;
    }
    case BytecodeOp::AppendFormat: {
        const auto p_value = BytecodeRegister(in_.read_u32());
        const auto p_formatter = BytecodeRegister(in_.read_u32());
        out_.format(" value {} formatter {}", dump(p_value), dump(p_formatter));
        break;
    }
    case BytecodeOp::FormatResult: {
        const auto p_formatter = BytecodeRegister(in_.read_u32());
        const auto p_target = BytecodeRegister(in_.read_u32());
        out_.format(" formatter {} target {}", dump(p_formatter), dump(p_target));
        break;
    }
    case BytecodeOp::Copy: {
        const auto p_source = BytecodeRegister(in_.read_u32());
        const auto p_target = BytecodeRegister(in_.read_u32());
        out_.format(" source {} target {}", dump(p_source), dump(p_target));
        break;
    }
    case BytecodeOp::Swap: {
        const auto p_a = BytecodeRegister(in_.read_u32());
        const auto p_b = BytecodeRegister(in_.read_u32());
        out_.format(" a {} b {}", dump(p_a), dump(p_b));
        break;
    }
    case BytecodeOp::Push: {
        const auto p_value = BytecodeRegister(in_.read_u32());
        out_.format(" value {}", dump(p_value));
        break;
    }
    case BytecodeOp::Pop: {
        break;
    }
    case BytecodeOp::PopTo: {
        const auto p_target = BytecodeRegister(in_.read_u32());
        out_.format(" target {}", dump(p_target));
        break;
    }
    case BytecodeOp::Jmp: {
        const auto p_offset = BytecodeOffset(in_.read_u32());
        out_.format(" offset {}", dump(p_offset));
        break;
    }
    case BytecodeOp::JmpTrue: {
        const auto p_condition = BytecodeRegister(in_.read_u32());
        const auto p_offset = BytecodeOffset(in_.read_u32());
        out_.format(" condition {} offset {}", dump(p_condition), dump(p_offset));
        break;
    }
    case BytecodeOp::JmpFalse: {
        const auto p_condition = BytecodeRegister(in_.read_u32());
        const auto p_offset = BytecodeOffset(in_.read_u32());
        out_.format(" condition {} offset {}", dump(p_condition), dump(p_offset));
        break;
    }
    case BytecodeOp::JmpNull: {
        const auto p_condition = BytecodeRegister(in_.read_u32());
        const auto p_offset = BytecodeOffset(in_.read_u32());
        out_.format(" condition {} offset {}", dump(p_condition), dump(p_offset));
        break;
    }
    case BytecodeOp::JmpNotNull: {
        const auto p_condition = BytecodeRegister(in_.read_u32());
        const auto p_offset = BytecodeOffset(in_.read_u32());
        out_.format(" condition {} offset {}", dump(p_condition), dump(p_offset));
        break;
    }
    case BytecodeOp::Call: {
        const auto p_function = BytecodeRegister(in_.read_u32());
        const auto p_count = u32(in_.read_u32());
        out_.format(" function {} count {}", dump(p_function), p_count);
        break;
    }
    case BytecodeOp::LoadMethod: {
        const auto p_object = BytecodeRegister(in_.read_u32());
        const auto p_name = BytecodeMemberId(in_.read_u32());
        const auto p_this = BytecodeRegister(in_.read_u32());
        const auto p_method = BytecodeRegister(in_.read_u32());
        out_.format(" object {} name {} this {} method {}", dump(p_object), dump(p_name),
            dump(p_this), dump(p_method));
        break;
    }
    case BytecodeOp::CallMethod: {
        const auto p_method = BytecodeRegister(in_.read_u32());
        const auto p_count = u32(in_.read_u32());
        out_.format(" method {} count {}", dump(p_method), p_count);
        break;
    }
    case BytecodeOp::Return: {
        const auto p_value = BytecodeRegister(in_.read_u32());
        out_.format(" value {}", dump(p_value));
        break;
    }
    case BytecodeOp::Rethrow: {
        break;
    }
    case BytecodeOp::AssertFail: {
        const auto p_expr = BytecodeRegister(in_.read_u32());
        const auto p_message = BytecodeRegister(in_.read_u32());
        out_.format(" expr {} message {}", dump(p_expr), dump(p_message));
        break;
    }
        // [[[end]]]
    }
}
} // namespace tiro

template<typename Entity>
struct tiro::EnableFormatMode<tiro::DumpHelper<Entity>> {
    static constexpr tiro::FormatMode value = tiro::FormatMode::MemberFormat;
};
