#include "compiler/bytecode/disassembler.hpp"

#include "common/format.hpp"
#include "compiler/binary.hpp"
#include "compiler/bytecode/op.hpp"

namespace tiro {

static void
disassemble_instruction(CheckedBinaryReader& in, FormatStream& out, size_t max_size_length) {

    const size_t start = in.pos();
    out.format("{start:>{width}}: ", fmt::arg("start", start), fmt::arg("width", max_size_length));

    const u8 raw_op = in.read_u8();
    if (!valid_opcode(raw_op))
        TIRO_ERROR("Invalid opcode at offset {}: {}.", start, raw_op);

    const BytecodeOp op = static_cast<BytecodeOp>(raw_op);
    out.format("{}", op);

    switch (op) {
    /* [[[cog
            import cog
            from codegen.bytecode import InstructionList

            for ins in InstructionList:
                cog.outl(f"case BytecodeOp::{ins.name}: {{")

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
    case BytecodeOp::LoadNull: {
        const auto p_target = in.read_u32();
        out.format(" target {}", p_target);
        break;
    }
    case BytecodeOp::LoadFalse: {
        const auto p_target = in.read_u32();
        out.format(" target {}", p_target);
        break;
    }
    case BytecodeOp::LoadTrue: {
        const auto p_target = in.read_u32();
        out.format(" target {}", p_target);
        break;
    }
    case BytecodeOp::LoadInt: {
        const auto p_constant = in.read_i64();
        const auto p_target = in.read_u32();
        out.format(" constant {} target {}", p_constant, p_target);
        break;
    }
    case BytecodeOp::LoadFloat: {
        const auto p_constant = in.read_f64();
        const auto p_target = in.read_u32();
        out.format(" constant {} target {}", p_constant, p_target);
        break;
    }
    case BytecodeOp::LoadParam: {
        const auto p_source = in.read_u32();
        const auto p_target = in.read_u32();
        out.format(" source {} target {}", p_source, p_target);
        break;
    }
    case BytecodeOp::StoreParam: {
        const auto p_source = in.read_u32();
        const auto p_target = in.read_u32();
        out.format(" source {} target {}", p_source, p_target);
        break;
    }
    case BytecodeOp::LoadModule: {
        const auto p_source = in.read_u32();
        const auto p_target = in.read_u32();
        out.format(" source {} target {}", p_source, p_target);
        break;
    }
    case BytecodeOp::StoreModule: {
        const auto p_source = in.read_u32();
        const auto p_target = in.read_u32();
        out.format(" source {} target {}", p_source, p_target);
        break;
    }
    case BytecodeOp::LoadMember: {
        const auto p_object = in.read_u32();
        const auto p_name = in.read_u32();
        const auto p_target = in.read_u32();
        out.format(" object {} name {} target {}", p_object, p_name, p_target);
        break;
    }
    case BytecodeOp::StoreMember: {
        const auto p_source = in.read_u32();
        const auto p_object = in.read_u32();
        const auto p_name = in.read_u32();
        out.format(" source {} object {} name {}", p_source, p_object, p_name);
        break;
    }
    case BytecodeOp::LoadTupleMember: {
        const auto p_tuple = in.read_u32();
        const auto p_index = in.read_u32();
        const auto p_target = in.read_u32();
        out.format(" tuple {} index {} target {}", p_tuple, p_index, p_target);
        break;
    }
    case BytecodeOp::StoreTupleMember: {
        const auto p_source = in.read_u32();
        const auto p_tuple = in.read_u32();
        const auto p_index = in.read_u32();
        out.format(" source {} tuple {} index {}", p_source, p_tuple, p_index);
        break;
    }
    case BytecodeOp::LoadIndex: {
        const auto p_array = in.read_u32();
        const auto p_index = in.read_u32();
        const auto p_target = in.read_u32();
        out.format(" array {} index {} target {}", p_array, p_index, p_target);
        break;
    }
    case BytecodeOp::StoreIndex: {
        const auto p_source = in.read_u32();
        const auto p_array = in.read_u32();
        const auto p_index = in.read_u32();
        out.format(" source {} array {} index {}", p_source, p_array, p_index);
        break;
    }
    case BytecodeOp::LoadClosure: {
        const auto p_target = in.read_u32();
        out.format(" target {}", p_target);
        break;
    }
    case BytecodeOp::LoadEnv: {
        const auto p_env = in.read_u32();
        const auto p_level = in.read_u32();
        const auto p_index = in.read_u32();
        const auto p_target = in.read_u32();
        out.format(" env {} level {} index {} target {}", p_env, p_level, p_index, p_target);
        break;
    }
    case BytecodeOp::StoreEnv: {
        const auto p_source = in.read_u32();
        const auto p_env = in.read_u32();
        const auto p_level = in.read_u32();
        const auto p_index = in.read_u32();
        out.format(" source {} env {} level {} index {}", p_source, p_env, p_level, p_index);
        break;
    }
    case BytecodeOp::Add: {
        const auto p_lhs = in.read_u32();
        const auto p_rhs = in.read_u32();
        const auto p_target = in.read_u32();
        out.format(" lhs {} rhs {} target {}", p_lhs, p_rhs, p_target);
        break;
    }
    case BytecodeOp::Sub: {
        const auto p_lhs = in.read_u32();
        const auto p_rhs = in.read_u32();
        const auto p_target = in.read_u32();
        out.format(" lhs {} rhs {} target {}", p_lhs, p_rhs, p_target);
        break;
    }
    case BytecodeOp::Mul: {
        const auto p_lhs = in.read_u32();
        const auto p_rhs = in.read_u32();
        const auto p_target = in.read_u32();
        out.format(" lhs {} rhs {} target {}", p_lhs, p_rhs, p_target);
        break;
    }
    case BytecodeOp::Div: {
        const auto p_lhs = in.read_u32();
        const auto p_rhs = in.read_u32();
        const auto p_target = in.read_u32();
        out.format(" lhs {} rhs {} target {}", p_lhs, p_rhs, p_target);
        break;
    }
    case BytecodeOp::Mod: {
        const auto p_lhs = in.read_u32();
        const auto p_rhs = in.read_u32();
        const auto p_target = in.read_u32();
        out.format(" lhs {} rhs {} target {}", p_lhs, p_rhs, p_target);
        break;
    }
    case BytecodeOp::Pow: {
        const auto p_lhs = in.read_u32();
        const auto p_rhs = in.read_u32();
        const auto p_target = in.read_u32();
        out.format(" lhs {} rhs {} target {}", p_lhs, p_rhs, p_target);
        break;
    }
    case BytecodeOp::UAdd: {
        const auto p_value = in.read_u32();
        const auto p_target = in.read_u32();
        out.format(" value {} target {}", p_value, p_target);
        break;
    }
    case BytecodeOp::UNeg: {
        const auto p_value = in.read_u32();
        const auto p_target = in.read_u32();
        out.format(" value {} target {}", p_value, p_target);
        break;
    }
    case BytecodeOp::LSh: {
        const auto p_lhs = in.read_u32();
        const auto p_rhs = in.read_u32();
        const auto p_target = in.read_u32();
        out.format(" lhs {} rhs {} target {}", p_lhs, p_rhs, p_target);
        break;
    }
    case BytecodeOp::RSh: {
        const auto p_lhs = in.read_u32();
        const auto p_rhs = in.read_u32();
        const auto p_target = in.read_u32();
        out.format(" lhs {} rhs {} target {}", p_lhs, p_rhs, p_target);
        break;
    }
    case BytecodeOp::BAnd: {
        const auto p_lhs = in.read_u32();
        const auto p_rhs = in.read_u32();
        const auto p_target = in.read_u32();
        out.format(" lhs {} rhs {} target {}", p_lhs, p_rhs, p_target);
        break;
    }
    case BytecodeOp::BOr: {
        const auto p_lhs = in.read_u32();
        const auto p_rhs = in.read_u32();
        const auto p_target = in.read_u32();
        out.format(" lhs {} rhs {} target {}", p_lhs, p_rhs, p_target);
        break;
    }
    case BytecodeOp::BXor: {
        const auto p_lhs = in.read_u32();
        const auto p_rhs = in.read_u32();
        const auto p_target = in.read_u32();
        out.format(" lhs {} rhs {} target {}", p_lhs, p_rhs, p_target);
        break;
    }
    case BytecodeOp::BNot: {
        const auto p_value = in.read_u32();
        const auto p_target = in.read_u32();
        out.format(" value {} target {}", p_value, p_target);
        break;
    }
    case BytecodeOp::Gt: {
        const auto p_lhs = in.read_u32();
        const auto p_rhs = in.read_u32();
        const auto p_target = in.read_u32();
        out.format(" lhs {} rhs {} target {}", p_lhs, p_rhs, p_target);
        break;
    }
    case BytecodeOp::Gte: {
        const auto p_lhs = in.read_u32();
        const auto p_rhs = in.read_u32();
        const auto p_target = in.read_u32();
        out.format(" lhs {} rhs {} target {}", p_lhs, p_rhs, p_target);
        break;
    }
    case BytecodeOp::Lt: {
        const auto p_lhs = in.read_u32();
        const auto p_rhs = in.read_u32();
        const auto p_target = in.read_u32();
        out.format(" lhs {} rhs {} target {}", p_lhs, p_rhs, p_target);
        break;
    }
    case BytecodeOp::Lte: {
        const auto p_lhs = in.read_u32();
        const auto p_rhs = in.read_u32();
        const auto p_target = in.read_u32();
        out.format(" lhs {} rhs {} target {}", p_lhs, p_rhs, p_target);
        break;
    }
    case BytecodeOp::Eq: {
        const auto p_lhs = in.read_u32();
        const auto p_rhs = in.read_u32();
        const auto p_target = in.read_u32();
        out.format(" lhs {} rhs {} target {}", p_lhs, p_rhs, p_target);
        break;
    }
    case BytecodeOp::NEq: {
        const auto p_lhs = in.read_u32();
        const auto p_rhs = in.read_u32();
        const auto p_target = in.read_u32();
        out.format(" lhs {} rhs {} target {}", p_lhs, p_rhs, p_target);
        break;
    }
    case BytecodeOp::LNot: {
        const auto p_value = in.read_u32();
        const auto p_target = in.read_u32();
        out.format(" value {} target {}", p_value, p_target);
        break;
    }
    case BytecodeOp::Array: {
        const auto p_count = in.read_u32();
        const auto p_target = in.read_u32();
        out.format(" count {} target {}", p_count, p_target);
        break;
    }
    case BytecodeOp::Tuple: {
        const auto p_count = in.read_u32();
        const auto p_target = in.read_u32();
        out.format(" count {} target {}", p_count, p_target);
        break;
    }
    case BytecodeOp::Set: {
        const auto p_count = in.read_u32();
        const auto p_target = in.read_u32();
        out.format(" count {} target {}", p_count, p_target);
        break;
    }
    case BytecodeOp::Map: {
        const auto p_count = in.read_u32();
        const auto p_target = in.read_u32();
        out.format(" count {} target {}", p_count, p_target);
        break;
    }
    case BytecodeOp::Env: {
        const auto p_parent = in.read_u32();
        const auto p_size = in.read_u32();
        const auto p_target = in.read_u32();
        out.format(" parent {} size {} target {}", p_parent, p_size, p_target);
        break;
    }
    case BytecodeOp::Closure: {
        const auto p_template = in.read_u32();
        const auto p_env = in.read_u32();
        const auto p_target = in.read_u32();
        out.format(" template {} env {} target {}", p_template, p_env, p_target);
        break;
    }
    case BytecodeOp::Formatter: {
        const auto p_target = in.read_u32();
        out.format(" target {}", p_target);
        break;
    }
    case BytecodeOp::AppendFormat: {
        const auto p_value = in.read_u32();
        const auto p_formatter = in.read_u32();
        out.format(" value {} formatter {}", p_value, p_formatter);
        break;
    }
    case BytecodeOp::FormatResult: {
        const auto p_formatter = in.read_u32();
        const auto p_target = in.read_u32();
        out.format(" formatter {} target {}", p_formatter, p_target);
        break;
    }
    case BytecodeOp::Copy: {
        const auto p_source = in.read_u32();
        const auto p_target = in.read_u32();
        out.format(" source {} target {}", p_source, p_target);
        break;
    }
    case BytecodeOp::Swap: {
        const auto p_a = in.read_u32();
        const auto p_b = in.read_u32();
        out.format(" a {} b {}", p_a, p_b);
        break;
    }
    case BytecodeOp::Push: {
        const auto p_value = in.read_u32();
        out.format(" value {}", p_value);
        break;
    }
    case BytecodeOp::Pop: {
        break;
    }
    case BytecodeOp::PopTo: {
        const auto p_target = in.read_u32();
        out.format(" target {}", p_target);
        break;
    }
    case BytecodeOp::Jmp: {
        const auto p_offset = in.read_u32();
        out.format(" offset {}", p_offset);
        break;
    }
    case BytecodeOp::JmpTrue: {
        const auto p_condition = in.read_u32();
        const auto p_offset = in.read_u32();
        out.format(" condition {} offset {}", p_condition, p_offset);
        break;
    }
    case BytecodeOp::JmpFalse: {
        const auto p_condition = in.read_u32();
        const auto p_offset = in.read_u32();
        out.format(" condition {} offset {}", p_condition, p_offset);
        break;
    }
    case BytecodeOp::JmpNull: {
        const auto p_condition = in.read_u32();
        const auto p_offset = in.read_u32();
        out.format(" condition {} offset {}", p_condition, p_offset);
        break;
    }
    case BytecodeOp::JmpNotNull: {
        const auto p_condition = in.read_u32();
        const auto p_offset = in.read_u32();
        out.format(" condition {} offset {}", p_condition, p_offset);
        break;
    }
    case BytecodeOp::Call: {
        const auto p_function = in.read_u32();
        const auto p_count = in.read_u32();
        out.format(" function {} count {}", p_function, p_count);
        break;
    }
    case BytecodeOp::LoadMethod: {
        const auto p_object = in.read_u32();
        const auto p_name = in.read_u32();
        const auto p_this = in.read_u32();
        const auto p_method = in.read_u32();
        out.format(" object {} name {} this {} method {}", p_object, p_name, p_this, p_method);
        break;
    }
    case BytecodeOp::CallMethod: {
        const auto p_method = in.read_u32();
        const auto p_count = in.read_u32();
        out.format(" method {} count {}", p_method, p_count);
        break;
    }
    case BytecodeOp::Return: {
        const auto p_value = in.read_u32();
        out.format(" value {}", p_value);
        break;
    }
    case BytecodeOp::AssertFail: {
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
    const size_t max_size_length = fmt::formatted_size("{}", size == 0 ? 0 : size - 1);

    while (in.remaining() > 0) {
        disassemble_instruction(in, out, max_size_length);
        out.format("\n");
    }

    return out.take_str();
}

} // namespace tiro
