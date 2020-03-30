#include "tiro/bytecode/disassembler.hpp"

#include "tiro/bytecode/opcode.hpp"
#include "tiro/compiler/binary.hpp"
#include "tiro/core/format.hpp"

namespace tiro {

static void disassemble_instruction(
    CheckedBinaryReader& in, FormatStream& out, size_t max_size_length) {

    const size_t start = in.pos();
    out.format("{start:>{width}}: ", fmt::arg("start", start),
        fmt::arg("width", max_size_length));

    const u8 raw_op = in.read_u8();
    if (!valid_opcode(raw_op))
        TIRO_ERROR("Invalid opcode at offset {}: {}.", start, raw_op);

    const Opcode op = static_cast<Opcode>(raw_op);
    out.format("{}", op);

    switch (op) {
    /* [[[cog
            import cog
            import bytecode

            for ins in bytecode.InstructionList:
                cog.outl(f"case Opcode::{ins.name}: {{")

                for param in ins.params:
                    cog.outl(f"const auto p_{param.name} = in.read_{param.raw_type}();")

                def format_string():
                    return " ".join(param.name + " {}" for param in ins.params)

                def format_args():
                    return ", ".join("p_" + param.name for param in ins.params)
                
                if len(ins.params) > 0:
                    cog.outl(f"out.format(\" {format_string()}\", {format_args()});")

                cog.outl(f"break;")
                cog.outl(f"}}")
        ]]] */
    case Opcode::LoadNull: {
        const auto p_target = in.read_u32();
        out.format(" target {}", p_target);
        break;
    }
    case Opcode::LoadFalse: {
        const auto p_target = in.read_u32();
        out.format(" target {}", p_target);
        break;
    }
    case Opcode::LoadTrue: {
        const auto p_target = in.read_u32();
        out.format(" target {}", p_target);
        break;
    }
    case Opcode::LoadInt: {
        const auto p_value = in.read_i64();
        const auto p_target = in.read_u32();
        out.format(" value {} target {}", p_value, p_target);
        break;
    }
    case Opcode::LoadFloat: {
        const auto p_value = in.read_f64();
        const auto p_target = in.read_u32();
        out.format(" value {} target {}", p_value, p_target);
        break;
    }
    case Opcode::LoadParam: {
        const auto p_source = in.read_u32();
        const auto p_target = in.read_u32();
        out.format(" source {} target {}", p_source, p_target);
        break;
    }
    case Opcode::StoreParam: {
        const auto p_source = in.read_u32();
        const auto p_target = in.read_u32();
        out.format(" source {} target {}", p_source, p_target);
        break;
    }
    case Opcode::LoadModule: {
        const auto p_source = in.read_u32();
        const auto p_target = in.read_u32();
        out.format(" source {} target {}", p_source, p_target);
        break;
    }
    case Opcode::StoreModule: {
        const auto p_source = in.read_u32();
        const auto p_target = in.read_u32();
        out.format(" source {} target {}", p_source, p_target);
        break;
    }
    case Opcode::LoadMember: {
        const auto p_object = in.read_u32();
        const auto p_name = in.read_u32();
        const auto p_target = in.read_u32();
        out.format(" object {} name {} target {}", p_object, p_name, p_target);
        break;
    }
    case Opcode::StoreMember: {
        const auto p_source = in.read_u32();
        const auto p_object = in.read_u32();
        const auto p_name = in.read_u32();
        out.format(" source {} object {} name {}", p_source, p_object, p_name);
        break;
    }
    case Opcode::LoadTupleMember: {
        const auto p_tuple = in.read_u32();
        const auto p_index = in.read_u32();
        const auto p_target = in.read_u32();
        out.format(" tuple {} index {} target {}", p_tuple, p_index, p_target);
        break;
    }
    case Opcode::StoreTupleMember: {
        const auto p_source = in.read_u32();
        const auto p_tuple = in.read_u32();
        const auto p_index = in.read_u32();
        out.format(" source {} tuple {} index {}", p_source, p_tuple, p_index);
        break;
    }
    case Opcode::LoadIndex: {
        const auto p_array = in.read_u32();
        const auto p_index = in.read_u32();
        const auto p_target = in.read_u32();
        out.format(" array {} index {} target {}", p_array, p_index, p_target);
        break;
    }
    case Opcode::StoreIndex: {
        const auto p_source = in.read_u32();
        const auto p_array = in.read_u32();
        const auto p_index = in.read_u32();
        out.format(" source {} array {} index {}", p_source, p_array, p_index);
        break;
    }
    case Opcode::LoadClosure: {
        const auto p_target = in.read_u32();
        out.format(" target {}", p_target);
        break;
    }
    case Opcode::LoadEnv: {
        const auto p_env = in.read_u32();
        const auto p_level = in.read_u32();
        const auto p_index = in.read_u32();
        const auto p_target = in.read_u32();
        out.format(" env {} level {} index {} target {}", p_env, p_level,
            p_index, p_target);
        break;
    }
    case Opcode::StoreEnv: {
        const auto p_source = in.read_u32();
        const auto p_env = in.read_u32();
        const auto p_level = in.read_u32();
        const auto p_index = in.read_u32();
        out.format(" source {} env {} level {} index {}", p_source, p_env,
            p_level, p_index);
        break;
    }
    case Opcode::Add: {
        const auto p_lhs = in.read_u32();
        const auto p_rhs = in.read_u32();
        const auto p_target = in.read_u32();
        out.format(" lhs {} rhs {} target {}", p_lhs, p_rhs, p_target);
        break;
    }
    case Opcode::Sub: {
        const auto p_lhs = in.read_u32();
        const auto p_rhs = in.read_u32();
        const auto p_target = in.read_u32();
        out.format(" lhs {} rhs {} target {}", p_lhs, p_rhs, p_target);
        break;
    }
    case Opcode::Mul: {
        const auto p_lhs = in.read_u32();
        const auto p_rhs = in.read_u32();
        const auto p_target = in.read_u32();
        out.format(" lhs {} rhs {} target {}", p_lhs, p_rhs, p_target);
        break;
    }
    case Opcode::Div: {
        const auto p_lhs = in.read_u32();
        const auto p_rhs = in.read_u32();
        const auto p_target = in.read_u32();
        out.format(" lhs {} rhs {} target {}", p_lhs, p_rhs, p_target);
        break;
    }
    case Opcode::Mod: {
        const auto p_lhs = in.read_u32();
        const auto p_rhs = in.read_u32();
        const auto p_target = in.read_u32();
        out.format(" lhs {} rhs {} target {}", p_lhs, p_rhs, p_target);
        break;
    }
    case Opcode::Pow: {
        const auto p_lhs = in.read_u32();
        const auto p_rhs = in.read_u32();
        const auto p_target = in.read_u32();
        out.format(" lhs {} rhs {} target {}", p_lhs, p_rhs, p_target);
        break;
    }
    case Opcode::UAdd: {
        const auto p_value = in.read_u32();
        const auto p_target = in.read_u32();
        out.format(" value {} target {}", p_value, p_target);
        break;
    }
    case Opcode::UNeg: {
        const auto p_value = in.read_u32();
        const auto p_target = in.read_u32();
        out.format(" value {} target {}", p_value, p_target);
        break;
    }
    case Opcode::LSh: {
        const auto p_lhs = in.read_u32();
        const auto p_rhs = in.read_u32();
        const auto p_target = in.read_u32();
        out.format(" lhs {} rhs {} target {}", p_lhs, p_rhs, p_target);
        break;
    }
    case Opcode::RSh: {
        const auto p_lhs = in.read_u32();
        const auto p_rhs = in.read_u32();
        const auto p_target = in.read_u32();
        out.format(" lhs {} rhs {} target {}", p_lhs, p_rhs, p_target);
        break;
    }
    case Opcode::BAnd: {
        const auto p_lhs = in.read_u32();
        const auto p_rhs = in.read_u32();
        const auto p_target = in.read_u32();
        out.format(" lhs {} rhs {} target {}", p_lhs, p_rhs, p_target);
        break;
    }
    case Opcode::BOr: {
        const auto p_lhs = in.read_u32();
        const auto p_rhs = in.read_u32();
        const auto p_target = in.read_u32();
        out.format(" lhs {} rhs {} target {}", p_lhs, p_rhs, p_target);
        break;
    }
    case Opcode::BXor: {
        const auto p_lhs = in.read_u32();
        const auto p_rhs = in.read_u32();
        const auto p_target = in.read_u32();
        out.format(" lhs {} rhs {} target {}", p_lhs, p_rhs, p_target);
        break;
    }
    case Opcode::BNot: {
        const auto p_value = in.read_u32();
        const auto p_target = in.read_u32();
        out.format(" value {} target {}", p_value, p_target);
        break;
    }
    case Opcode::Gt: {
        const auto p_lhs = in.read_u32();
        const auto p_rhs = in.read_u32();
        const auto p_target = in.read_u32();
        out.format(" lhs {} rhs {} target {}", p_lhs, p_rhs, p_target);
        break;
    }
    case Opcode::Gte: {
        const auto p_lhs = in.read_u32();
        const auto p_rhs = in.read_u32();
        const auto p_target = in.read_u32();
        out.format(" lhs {} rhs {} target {}", p_lhs, p_rhs, p_target);
        break;
    }
    case Opcode::Lt: {
        const auto p_lhs = in.read_u32();
        const auto p_rhs = in.read_u32();
        const auto p_target = in.read_u32();
        out.format(" lhs {} rhs {} target {}", p_lhs, p_rhs, p_target);
        break;
    }
    case Opcode::Lte: {
        const auto p_lhs = in.read_u32();
        const auto p_rhs = in.read_u32();
        const auto p_target = in.read_u32();
        out.format(" lhs {} rhs {} target {}", p_lhs, p_rhs, p_target);
        break;
    }
    case Opcode::Eq: {
        const auto p_lhs = in.read_u32();
        const auto p_rhs = in.read_u32();
        const auto p_target = in.read_u32();
        out.format(" lhs {} rhs {} target {}", p_lhs, p_rhs, p_target);
        break;
    }
    case Opcode::NEq: {
        const auto p_lhs = in.read_u32();
        const auto p_rhs = in.read_u32();
        const auto p_target = in.read_u32();
        out.format(" lhs {} rhs {} target {}", p_lhs, p_rhs, p_target);
        break;
    }
    case Opcode::LNot: {
        const auto p_value = in.read_u32();
        const auto p_target = in.read_u32();
        out.format(" value {} target {}", p_value, p_target);
        break;
    }
    case Opcode::Array: {
        const auto p_count = in.read_u32();
        const auto p_target = in.read_u32();
        out.format(" count {} target {}", p_count, p_target);
        break;
    }
    case Opcode::Tuple: {
        const auto p_count = in.read_u32();
        const auto p_target = in.read_u32();
        out.format(" count {} target {}", p_count, p_target);
        break;
    }
    case Opcode::Set: {
        const auto p_count = in.read_u32();
        const auto p_target = in.read_u32();
        out.format(" count {} target {}", p_count, p_target);
        break;
    }
    case Opcode::Map: {
        const auto p_count = in.read_u32();
        const auto p_target = in.read_u32();
        out.format(" count {} target {}", p_count, p_target);
        break;
    }
    case Opcode::Env: {
        const auto p_parent = in.read_u32();
        const auto p_size = in.read_u32();
        const auto p_target = in.read_u32();
        out.format(" parent {} size {} target {}", p_parent, p_size, p_target);
        break;
    }
    case Opcode::Closure: {
        const auto p_template = in.read_u32();
        const auto p_env = in.read_u32();
        const auto p_target = in.read_u32();
        out.format(
            " template {} env {} target {}", p_template, p_env, p_target);
        break;
    }
    case Opcode::Formatter: {
        const auto p_target = in.read_u32();
        out.format(" target {}", p_target);
        break;
    }
    case Opcode::AppendFormat: {
        const auto p_value = in.read_u32();
        const auto p_formatter = in.read_u32();
        out.format(" value {} formatter {}", p_value, p_formatter);
        break;
    }
    case Opcode::FormatResult: {
        const auto p_formatter = in.read_u32();
        const auto p_target = in.read_u32();
        out.format(" formatter {} target {}", p_formatter, p_target);
        break;
    }
    case Opcode::Copy: {
        const auto p_source = in.read_u32();
        const auto p_target = in.read_u32();
        out.format(" source {} target {}", p_source, p_target);
        break;
    }
    case Opcode::Swap: {
        const auto p_a = in.read_u32();
        const auto p_b = in.read_u32();
        out.format(" a {} b {}", p_a, p_b);
        break;
    }
    case Opcode::Push: {
        const auto p_value = in.read_u32();
        out.format(" value {}", p_value);
        break;
    }
    case Opcode::Pop: {
        break;
    }
    case Opcode::PopTo: {
        const auto p_target = in.read_u32();
        out.format(" target {}", p_target);
        break;
    }
    case Opcode::Jmp: {
        const auto p_target = in.read_u32();
        out.format(" target {}", p_target);
        break;
    }
    case Opcode::JmpTrue: {
        const auto p_value = in.read_u32();
        const auto p_target = in.read_u32();
        out.format(" value {} target {}", p_value, p_target);
        break;
    }
    case Opcode::JmpFalse: {
        const auto p_value = in.read_u32();
        const auto p_target = in.read_u32();
        out.format(" value {} target {}", p_value, p_target);
        break;
    }
    case Opcode::Call: {
        const auto p_function = in.read_u32();
        const auto p_count = in.read_u32();
        out.format(" function {} count {}", p_function, p_count);
        break;
    }
    case Opcode::LoadMethod: {
        const auto p_object = in.read_u32();
        const auto p_name = in.read_u32();
        const auto p_this = in.read_u32();
        const auto p_method = in.read_u32();
        out.format(" object {} name {} this {} method {}", p_object, p_name,
            p_this, p_method);
        break;
    }
    case Opcode::CallMethod: {
        const auto p_method = in.read_u32();
        const auto p_count = in.read_u32();
        out.format(" method {} count {}", p_method, p_count);
        break;
    }
    case Opcode::Return: {
        const auto p_value = in.read_u32();
        out.format(" value {}", p_value);
        break;
    }
    case Opcode::AssertFail: {
        const auto p_expr = in.read_u32();
        const auto p_message = in.read_u32();
        out.format(" expr {} message {}", p_expr, p_message);
        break;
    }
        // [[[end]]]
    }
}

std::string disassemble(Span<const byte> bytecode) {
    StringFormatStream out;
    CheckedBinaryReader in(bytecode);

    const size_t size = bytecode.size();
    const size_t max_size_length = fmt::formatted_size(
        "{}", size == 0 ? 0 : size - 1);

    while (in.remaining() > 0) {
        disassemble_instruction(in, out, max_size_length);
        out.format("\n");
    }

    return out.take_str();
}

} // namespace tiro
