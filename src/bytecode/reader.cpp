#include "bytecode/reader.hpp"

#include "common/assert.hpp"

namespace tiro {

std::string_view message(BytecodeReaderError error) {
    switch (error) {
    case BytecodeReaderError::InvalidOpcode:
        return "invalid opcode";
    case BytecodeReaderError::IncompleteInstruction:
        return "incomplete instruction";
    case BytecodeReaderError::End:
        return "no more instructions";
    }

    TIRO_UNREACHABLE("invalid bytecode reader error");
}

BytecodeReader::BytecodeReader(Span<const byte> bytecode)
    : r_(bytecode) {}

std::variant<BytecodeInstr, BytecodeReaderError> BytecodeReader::read() {
#define TIRO_CHECK_OPERAND_SIZE(required) \
    if (r_.remaining() < (required))      \
        return BytecodeReaderError::IncompleteInstruction;

    if (r_.remaining() != 0) {
        const u8 raw_op = r_.read_u8();
        if (!valid_opcode(raw_op))
            return BytecodeReaderError::InvalidOpcode;

        switch (static_cast<BytecodeOp>(raw_op)) {
        /* [[[cog
                import cog
                from codegen.bytecode import InstructionList

                def var_name(param):
                    return f"p_{param.name}"

                def read_param(param):
                    name = var_name(param)
                    return f"    const auto {name} = {param.cpp_type}(r_.read_{param.raw_type}());"

                def build_ins(ins):
                    params = ", ".join(var_name(param) for param in ins.params)
                    return f"    return BytecodeInstr::{ins.name}{{{params}}};"

                def operand_size(ins):
                    return sum(param.raw_size for param in ins.params)

                for ins in InstructionList:
                    cog.outl(f"case BytecodeOp::{ins.name}: {{")
                    if ins.params:
                        cog.outl(f"    static constexpr auto operand_size = {operand_size(ins)};")
                        cog.outl(f"    TIRO_CHECK_OPERAND_SIZE(operand_size);")

                    for param in ins.params:
                        cog.outl(read_param(param))

                    cog.outl(build_ins(ins))
                    cog.outl(f"}}")
            ]]] */
        case BytecodeOp::LoadNull: {
            static constexpr auto operand_size = 4;
            TIRO_CHECK_OPERAND_SIZE(operand_size);
            const auto p_target = BytecodeRegister(r_.read_u32());
            return BytecodeInstr::LoadNull{p_target};
        }
        case BytecodeOp::LoadFalse: {
            static constexpr auto operand_size = 4;
            TIRO_CHECK_OPERAND_SIZE(operand_size);
            const auto p_target = BytecodeRegister(r_.read_u32());
            return BytecodeInstr::LoadFalse{p_target};
        }
        case BytecodeOp::LoadTrue: {
            static constexpr auto operand_size = 4;
            TIRO_CHECK_OPERAND_SIZE(operand_size);
            const auto p_target = BytecodeRegister(r_.read_u32());
            return BytecodeInstr::LoadTrue{p_target};
        }
        case BytecodeOp::LoadInt: {
            static constexpr auto operand_size = 12;
            TIRO_CHECK_OPERAND_SIZE(operand_size);
            const auto p_constant = i64(r_.read_i64());
            const auto p_target = BytecodeRegister(r_.read_u32());
            return BytecodeInstr::LoadInt{p_constant, p_target};
        }
        case BytecodeOp::LoadFloat: {
            static constexpr auto operand_size = 12;
            TIRO_CHECK_OPERAND_SIZE(operand_size);
            const auto p_constant = f64(r_.read_f64());
            const auto p_target = BytecodeRegister(r_.read_u32());
            return BytecodeInstr::LoadFloat{p_constant, p_target};
        }
        case BytecodeOp::LoadParam: {
            static constexpr auto operand_size = 8;
            TIRO_CHECK_OPERAND_SIZE(operand_size);
            const auto p_source = BytecodeParam(r_.read_u32());
            const auto p_target = BytecodeRegister(r_.read_u32());
            return BytecodeInstr::LoadParam{p_source, p_target};
        }
        case BytecodeOp::StoreParam: {
            static constexpr auto operand_size = 8;
            TIRO_CHECK_OPERAND_SIZE(operand_size);
            const auto p_source = BytecodeRegister(r_.read_u32());
            const auto p_target = BytecodeParam(r_.read_u32());
            return BytecodeInstr::StoreParam{p_source, p_target};
        }
        case BytecodeOp::LoadModule: {
            static constexpr auto operand_size = 8;
            TIRO_CHECK_OPERAND_SIZE(operand_size);
            const auto p_source = BytecodeMemberId(r_.read_u32());
            const auto p_target = BytecodeRegister(r_.read_u32());
            return BytecodeInstr::LoadModule{p_source, p_target};
        }
        case BytecodeOp::StoreModule: {
            static constexpr auto operand_size = 8;
            TIRO_CHECK_OPERAND_SIZE(operand_size);
            const auto p_source = BytecodeRegister(r_.read_u32());
            const auto p_target = BytecodeMemberId(r_.read_u32());
            return BytecodeInstr::StoreModule{p_source, p_target};
        }
        case BytecodeOp::LoadMember: {
            static constexpr auto operand_size = 12;
            TIRO_CHECK_OPERAND_SIZE(operand_size);
            const auto p_object = BytecodeRegister(r_.read_u32());
            const auto p_name = BytecodeMemberId(r_.read_u32());
            const auto p_target = BytecodeRegister(r_.read_u32());
            return BytecodeInstr::LoadMember{p_object, p_name, p_target};
        }
        case BytecodeOp::StoreMember: {
            static constexpr auto operand_size = 12;
            TIRO_CHECK_OPERAND_SIZE(operand_size);
            const auto p_source = BytecodeRegister(r_.read_u32());
            const auto p_object = BytecodeRegister(r_.read_u32());
            const auto p_name = BytecodeMemberId(r_.read_u32());
            return BytecodeInstr::StoreMember{p_source, p_object, p_name};
        }
        case BytecodeOp::LoadTupleMember: {
            static constexpr auto operand_size = 12;
            TIRO_CHECK_OPERAND_SIZE(operand_size);
            const auto p_tuple = BytecodeRegister(r_.read_u32());
            const auto p_index = u32(r_.read_u32());
            const auto p_target = BytecodeRegister(r_.read_u32());
            return BytecodeInstr::LoadTupleMember{p_tuple, p_index, p_target};
        }
        case BytecodeOp::StoreTupleMember: {
            static constexpr auto operand_size = 12;
            TIRO_CHECK_OPERAND_SIZE(operand_size);
            const auto p_source = BytecodeRegister(r_.read_u32());
            const auto p_tuple = BytecodeRegister(r_.read_u32());
            const auto p_index = u32(r_.read_u32());
            return BytecodeInstr::StoreTupleMember{p_source, p_tuple, p_index};
        }
        case BytecodeOp::LoadIndex: {
            static constexpr auto operand_size = 12;
            TIRO_CHECK_OPERAND_SIZE(operand_size);
            const auto p_array = BytecodeRegister(r_.read_u32());
            const auto p_index = BytecodeRegister(r_.read_u32());
            const auto p_target = BytecodeRegister(r_.read_u32());
            return BytecodeInstr::LoadIndex{p_array, p_index, p_target};
        }
        case BytecodeOp::StoreIndex: {
            static constexpr auto operand_size = 12;
            TIRO_CHECK_OPERAND_SIZE(operand_size);
            const auto p_source = BytecodeRegister(r_.read_u32());
            const auto p_array = BytecodeRegister(r_.read_u32());
            const auto p_index = BytecodeRegister(r_.read_u32());
            return BytecodeInstr::StoreIndex{p_source, p_array, p_index};
        }
        case BytecodeOp::LoadClosure: {
            static constexpr auto operand_size = 4;
            TIRO_CHECK_OPERAND_SIZE(operand_size);
            const auto p_target = BytecodeRegister(r_.read_u32());
            return BytecodeInstr::LoadClosure{p_target};
        }
        case BytecodeOp::LoadEnv: {
            static constexpr auto operand_size = 16;
            TIRO_CHECK_OPERAND_SIZE(operand_size);
            const auto p_env = BytecodeRegister(r_.read_u32());
            const auto p_level = u32(r_.read_u32());
            const auto p_index = u32(r_.read_u32());
            const auto p_target = BytecodeRegister(r_.read_u32());
            return BytecodeInstr::LoadEnv{p_env, p_level, p_index, p_target};
        }
        case BytecodeOp::StoreEnv: {
            static constexpr auto operand_size = 16;
            TIRO_CHECK_OPERAND_SIZE(operand_size);
            const auto p_source = BytecodeRegister(r_.read_u32());
            const auto p_env = BytecodeRegister(r_.read_u32());
            const auto p_level = u32(r_.read_u32());
            const auto p_index = u32(r_.read_u32());
            return BytecodeInstr::StoreEnv{p_source, p_env, p_level, p_index};
        }
        case BytecodeOp::Add: {
            static constexpr auto operand_size = 12;
            TIRO_CHECK_OPERAND_SIZE(operand_size);
            const auto p_lhs = BytecodeRegister(r_.read_u32());
            const auto p_rhs = BytecodeRegister(r_.read_u32());
            const auto p_target = BytecodeRegister(r_.read_u32());
            return BytecodeInstr::Add{p_lhs, p_rhs, p_target};
        }
        case BytecodeOp::Sub: {
            static constexpr auto operand_size = 12;
            TIRO_CHECK_OPERAND_SIZE(operand_size);
            const auto p_lhs = BytecodeRegister(r_.read_u32());
            const auto p_rhs = BytecodeRegister(r_.read_u32());
            const auto p_target = BytecodeRegister(r_.read_u32());
            return BytecodeInstr::Sub{p_lhs, p_rhs, p_target};
        }
        case BytecodeOp::Mul: {
            static constexpr auto operand_size = 12;
            TIRO_CHECK_OPERAND_SIZE(operand_size);
            const auto p_lhs = BytecodeRegister(r_.read_u32());
            const auto p_rhs = BytecodeRegister(r_.read_u32());
            const auto p_target = BytecodeRegister(r_.read_u32());
            return BytecodeInstr::Mul{p_lhs, p_rhs, p_target};
        }
        case BytecodeOp::Div: {
            static constexpr auto operand_size = 12;
            TIRO_CHECK_OPERAND_SIZE(operand_size);
            const auto p_lhs = BytecodeRegister(r_.read_u32());
            const auto p_rhs = BytecodeRegister(r_.read_u32());
            const auto p_target = BytecodeRegister(r_.read_u32());
            return BytecodeInstr::Div{p_lhs, p_rhs, p_target};
        }
        case BytecodeOp::Mod: {
            static constexpr auto operand_size = 12;
            TIRO_CHECK_OPERAND_SIZE(operand_size);
            const auto p_lhs = BytecodeRegister(r_.read_u32());
            const auto p_rhs = BytecodeRegister(r_.read_u32());
            const auto p_target = BytecodeRegister(r_.read_u32());
            return BytecodeInstr::Mod{p_lhs, p_rhs, p_target};
        }
        case BytecodeOp::Pow: {
            static constexpr auto operand_size = 12;
            TIRO_CHECK_OPERAND_SIZE(operand_size);
            const auto p_lhs = BytecodeRegister(r_.read_u32());
            const auto p_rhs = BytecodeRegister(r_.read_u32());
            const auto p_target = BytecodeRegister(r_.read_u32());
            return BytecodeInstr::Pow{p_lhs, p_rhs, p_target};
        }
        case BytecodeOp::UAdd: {
            static constexpr auto operand_size = 8;
            TIRO_CHECK_OPERAND_SIZE(operand_size);
            const auto p_value = BytecodeRegister(r_.read_u32());
            const auto p_target = BytecodeRegister(r_.read_u32());
            return BytecodeInstr::UAdd{p_value, p_target};
        }
        case BytecodeOp::UNeg: {
            static constexpr auto operand_size = 8;
            TIRO_CHECK_OPERAND_SIZE(operand_size);
            const auto p_value = BytecodeRegister(r_.read_u32());
            const auto p_target = BytecodeRegister(r_.read_u32());
            return BytecodeInstr::UNeg{p_value, p_target};
        }
        case BytecodeOp::LSh: {
            static constexpr auto operand_size = 12;
            TIRO_CHECK_OPERAND_SIZE(operand_size);
            const auto p_lhs = BytecodeRegister(r_.read_u32());
            const auto p_rhs = BytecodeRegister(r_.read_u32());
            const auto p_target = BytecodeRegister(r_.read_u32());
            return BytecodeInstr::LSh{p_lhs, p_rhs, p_target};
        }
        case BytecodeOp::RSh: {
            static constexpr auto operand_size = 12;
            TIRO_CHECK_OPERAND_SIZE(operand_size);
            const auto p_lhs = BytecodeRegister(r_.read_u32());
            const auto p_rhs = BytecodeRegister(r_.read_u32());
            const auto p_target = BytecodeRegister(r_.read_u32());
            return BytecodeInstr::RSh{p_lhs, p_rhs, p_target};
        }
        case BytecodeOp::BAnd: {
            static constexpr auto operand_size = 12;
            TIRO_CHECK_OPERAND_SIZE(operand_size);
            const auto p_lhs = BytecodeRegister(r_.read_u32());
            const auto p_rhs = BytecodeRegister(r_.read_u32());
            const auto p_target = BytecodeRegister(r_.read_u32());
            return BytecodeInstr::BAnd{p_lhs, p_rhs, p_target};
        }
        case BytecodeOp::BOr: {
            static constexpr auto operand_size = 12;
            TIRO_CHECK_OPERAND_SIZE(operand_size);
            const auto p_lhs = BytecodeRegister(r_.read_u32());
            const auto p_rhs = BytecodeRegister(r_.read_u32());
            const auto p_target = BytecodeRegister(r_.read_u32());
            return BytecodeInstr::BOr{p_lhs, p_rhs, p_target};
        }
        case BytecodeOp::BXor: {
            static constexpr auto operand_size = 12;
            TIRO_CHECK_OPERAND_SIZE(operand_size);
            const auto p_lhs = BytecodeRegister(r_.read_u32());
            const auto p_rhs = BytecodeRegister(r_.read_u32());
            const auto p_target = BytecodeRegister(r_.read_u32());
            return BytecodeInstr::BXor{p_lhs, p_rhs, p_target};
        }
        case BytecodeOp::BNot: {
            static constexpr auto operand_size = 8;
            TIRO_CHECK_OPERAND_SIZE(operand_size);
            const auto p_value = BytecodeRegister(r_.read_u32());
            const auto p_target = BytecodeRegister(r_.read_u32());
            return BytecodeInstr::BNot{p_value, p_target};
        }
        case BytecodeOp::Gt: {
            static constexpr auto operand_size = 12;
            TIRO_CHECK_OPERAND_SIZE(operand_size);
            const auto p_lhs = BytecodeRegister(r_.read_u32());
            const auto p_rhs = BytecodeRegister(r_.read_u32());
            const auto p_target = BytecodeRegister(r_.read_u32());
            return BytecodeInstr::Gt{p_lhs, p_rhs, p_target};
        }
        case BytecodeOp::Gte: {
            static constexpr auto operand_size = 12;
            TIRO_CHECK_OPERAND_SIZE(operand_size);
            const auto p_lhs = BytecodeRegister(r_.read_u32());
            const auto p_rhs = BytecodeRegister(r_.read_u32());
            const auto p_target = BytecodeRegister(r_.read_u32());
            return BytecodeInstr::Gte{p_lhs, p_rhs, p_target};
        }
        case BytecodeOp::Lt: {
            static constexpr auto operand_size = 12;
            TIRO_CHECK_OPERAND_SIZE(operand_size);
            const auto p_lhs = BytecodeRegister(r_.read_u32());
            const auto p_rhs = BytecodeRegister(r_.read_u32());
            const auto p_target = BytecodeRegister(r_.read_u32());
            return BytecodeInstr::Lt{p_lhs, p_rhs, p_target};
        }
        case BytecodeOp::Lte: {
            static constexpr auto operand_size = 12;
            TIRO_CHECK_OPERAND_SIZE(operand_size);
            const auto p_lhs = BytecodeRegister(r_.read_u32());
            const auto p_rhs = BytecodeRegister(r_.read_u32());
            const auto p_target = BytecodeRegister(r_.read_u32());
            return BytecodeInstr::Lte{p_lhs, p_rhs, p_target};
        }
        case BytecodeOp::Eq: {
            static constexpr auto operand_size = 12;
            TIRO_CHECK_OPERAND_SIZE(operand_size);
            const auto p_lhs = BytecodeRegister(r_.read_u32());
            const auto p_rhs = BytecodeRegister(r_.read_u32());
            const auto p_target = BytecodeRegister(r_.read_u32());
            return BytecodeInstr::Eq{p_lhs, p_rhs, p_target};
        }
        case BytecodeOp::NEq: {
            static constexpr auto operand_size = 12;
            TIRO_CHECK_OPERAND_SIZE(operand_size);
            const auto p_lhs = BytecodeRegister(r_.read_u32());
            const auto p_rhs = BytecodeRegister(r_.read_u32());
            const auto p_target = BytecodeRegister(r_.read_u32());
            return BytecodeInstr::NEq{p_lhs, p_rhs, p_target};
        }
        case BytecodeOp::LNot: {
            static constexpr auto operand_size = 8;
            TIRO_CHECK_OPERAND_SIZE(operand_size);
            const auto p_value = BytecodeRegister(r_.read_u32());
            const auto p_target = BytecodeRegister(r_.read_u32());
            return BytecodeInstr::LNot{p_value, p_target};
        }
        case BytecodeOp::Array: {
            static constexpr auto operand_size = 8;
            TIRO_CHECK_OPERAND_SIZE(operand_size);
            const auto p_count = u32(r_.read_u32());
            const auto p_target = BytecodeRegister(r_.read_u32());
            return BytecodeInstr::Array{p_count, p_target};
        }
        case BytecodeOp::Tuple: {
            static constexpr auto operand_size = 8;
            TIRO_CHECK_OPERAND_SIZE(operand_size);
            const auto p_count = u32(r_.read_u32());
            const auto p_target = BytecodeRegister(r_.read_u32());
            return BytecodeInstr::Tuple{p_count, p_target};
        }
        case BytecodeOp::Set: {
            static constexpr auto operand_size = 8;
            TIRO_CHECK_OPERAND_SIZE(operand_size);
            const auto p_count = u32(r_.read_u32());
            const auto p_target = BytecodeRegister(r_.read_u32());
            return BytecodeInstr::Set{p_count, p_target};
        }
        case BytecodeOp::Map: {
            static constexpr auto operand_size = 8;
            TIRO_CHECK_OPERAND_SIZE(operand_size);
            const auto p_count = u32(r_.read_u32());
            const auto p_target = BytecodeRegister(r_.read_u32());
            return BytecodeInstr::Map{p_count, p_target};
        }
        case BytecodeOp::Env: {
            static constexpr auto operand_size = 12;
            TIRO_CHECK_OPERAND_SIZE(operand_size);
            const auto p_parent = BytecodeRegister(r_.read_u32());
            const auto p_size = u32(r_.read_u32());
            const auto p_target = BytecodeRegister(r_.read_u32());
            return BytecodeInstr::Env{p_parent, p_size, p_target};
        }
        case BytecodeOp::Closure: {
            static constexpr auto operand_size = 12;
            TIRO_CHECK_OPERAND_SIZE(operand_size);
            const auto p_template = BytecodeMemberId(r_.read_u32());
            const auto p_env = BytecodeRegister(r_.read_u32());
            const auto p_target = BytecodeRegister(r_.read_u32());
            return BytecodeInstr::Closure{p_template, p_env, p_target};
        }
        case BytecodeOp::Record: {
            static constexpr auto operand_size = 8;
            TIRO_CHECK_OPERAND_SIZE(operand_size);
            const auto p_template = BytecodeMemberId(r_.read_u32());
            const auto p_target = BytecodeRegister(r_.read_u32());
            return BytecodeInstr::Record{p_template, p_target};
        }
        case BytecodeOp::Iterator: {
            static constexpr auto operand_size = 8;
            TIRO_CHECK_OPERAND_SIZE(operand_size);
            const auto p_container = BytecodeRegister(r_.read_u32());
            const auto p_target = BytecodeRegister(r_.read_u32());
            return BytecodeInstr::Iterator{p_container, p_target};
        }
        case BytecodeOp::IteratorNext: {
            static constexpr auto operand_size = 12;
            TIRO_CHECK_OPERAND_SIZE(operand_size);
            const auto p_iterator = BytecodeRegister(r_.read_u32());
            const auto p_valid = BytecodeRegister(r_.read_u32());
            const auto p_value = BytecodeRegister(r_.read_u32());
            return BytecodeInstr::IteratorNext{p_iterator, p_valid, p_value};
        }
        case BytecodeOp::Formatter: {
            static constexpr auto operand_size = 4;
            TIRO_CHECK_OPERAND_SIZE(operand_size);
            const auto p_target = BytecodeRegister(r_.read_u32());
            return BytecodeInstr::Formatter{p_target};
        }
        case BytecodeOp::AppendFormat: {
            static constexpr auto operand_size = 8;
            TIRO_CHECK_OPERAND_SIZE(operand_size);
            const auto p_value = BytecodeRegister(r_.read_u32());
            const auto p_formatter = BytecodeRegister(r_.read_u32());
            return BytecodeInstr::AppendFormat{p_value, p_formatter};
        }
        case BytecodeOp::FormatResult: {
            static constexpr auto operand_size = 8;
            TIRO_CHECK_OPERAND_SIZE(operand_size);
            const auto p_formatter = BytecodeRegister(r_.read_u32());
            const auto p_target = BytecodeRegister(r_.read_u32());
            return BytecodeInstr::FormatResult{p_formatter, p_target};
        }
        case BytecodeOp::Copy: {
            static constexpr auto operand_size = 8;
            TIRO_CHECK_OPERAND_SIZE(operand_size);
            const auto p_source = BytecodeRegister(r_.read_u32());
            const auto p_target = BytecodeRegister(r_.read_u32());
            return BytecodeInstr::Copy{p_source, p_target};
        }
        case BytecodeOp::Swap: {
            static constexpr auto operand_size = 8;
            TIRO_CHECK_OPERAND_SIZE(operand_size);
            const auto p_a = BytecodeRegister(r_.read_u32());
            const auto p_b = BytecodeRegister(r_.read_u32());
            return BytecodeInstr::Swap{p_a, p_b};
        }
        case BytecodeOp::Push: {
            static constexpr auto operand_size = 4;
            TIRO_CHECK_OPERAND_SIZE(operand_size);
            const auto p_value = BytecodeRegister(r_.read_u32());
            return BytecodeInstr::Push{p_value};
        }
        case BytecodeOp::Pop: {
            return BytecodeInstr::Pop{};
        }
        case BytecodeOp::PopTo: {
            static constexpr auto operand_size = 4;
            TIRO_CHECK_OPERAND_SIZE(operand_size);
            const auto p_target = BytecodeRegister(r_.read_u32());
            return BytecodeInstr::PopTo{p_target};
        }
        case BytecodeOp::Jmp: {
            static constexpr auto operand_size = 4;
            TIRO_CHECK_OPERAND_SIZE(operand_size);
            const auto p_offset = BytecodeOffset(r_.read_u32());
            return BytecodeInstr::Jmp{p_offset};
        }
        case BytecodeOp::JmpTrue: {
            static constexpr auto operand_size = 8;
            TIRO_CHECK_OPERAND_SIZE(operand_size);
            const auto p_condition = BytecodeRegister(r_.read_u32());
            const auto p_offset = BytecodeOffset(r_.read_u32());
            return BytecodeInstr::JmpTrue{p_condition, p_offset};
        }
        case BytecodeOp::JmpFalse: {
            static constexpr auto operand_size = 8;
            TIRO_CHECK_OPERAND_SIZE(operand_size);
            const auto p_condition = BytecodeRegister(r_.read_u32());
            const auto p_offset = BytecodeOffset(r_.read_u32());
            return BytecodeInstr::JmpFalse{p_condition, p_offset};
        }
        case BytecodeOp::JmpNull: {
            static constexpr auto operand_size = 8;
            TIRO_CHECK_OPERAND_SIZE(operand_size);
            const auto p_condition = BytecodeRegister(r_.read_u32());
            const auto p_offset = BytecodeOffset(r_.read_u32());
            return BytecodeInstr::JmpNull{p_condition, p_offset};
        }
        case BytecodeOp::JmpNotNull: {
            static constexpr auto operand_size = 8;
            TIRO_CHECK_OPERAND_SIZE(operand_size);
            const auto p_condition = BytecodeRegister(r_.read_u32());
            const auto p_offset = BytecodeOffset(r_.read_u32());
            return BytecodeInstr::JmpNotNull{p_condition, p_offset};
        }
        case BytecodeOp::Call: {
            static constexpr auto operand_size = 8;
            TIRO_CHECK_OPERAND_SIZE(operand_size);
            const auto p_function = BytecodeRegister(r_.read_u32());
            const auto p_count = u32(r_.read_u32());
            return BytecodeInstr::Call{p_function, p_count};
        }
        case BytecodeOp::LoadMethod: {
            static constexpr auto operand_size = 16;
            TIRO_CHECK_OPERAND_SIZE(operand_size);
            const auto p_object = BytecodeRegister(r_.read_u32());
            const auto p_name = BytecodeMemberId(r_.read_u32());
            const auto p_this = BytecodeRegister(r_.read_u32());
            const auto p_method = BytecodeRegister(r_.read_u32());
            return BytecodeInstr::LoadMethod{p_object, p_name, p_this, p_method};
        }
        case BytecodeOp::CallMethod: {
            static constexpr auto operand_size = 8;
            TIRO_CHECK_OPERAND_SIZE(operand_size);
            const auto p_method = BytecodeRegister(r_.read_u32());
            const auto p_count = u32(r_.read_u32());
            return BytecodeInstr::CallMethod{p_method, p_count};
        }
        case BytecodeOp::Return: {
            static constexpr auto operand_size = 4;
            TIRO_CHECK_OPERAND_SIZE(operand_size);
            const auto p_value = BytecodeRegister(r_.read_u32());
            return BytecodeInstr::Return{p_value};
        }
        case BytecodeOp::Rethrow: {
            return BytecodeInstr::Rethrow{};
        }
        case BytecodeOp::AssertFail: {
            static constexpr auto operand_size = 8;
            TIRO_CHECK_OPERAND_SIZE(operand_size);
            const auto p_expr = BytecodeRegister(r_.read_u32());
            const auto p_message = BytecodeRegister(r_.read_u32());
            return BytecodeInstr::AssertFail{p_expr, p_message};
        }
            // [[[end]]]
        default:
            return BytecodeReaderError::InvalidOpcode;
        }
    }
    return BytecodeReaderError::End;
}

} // namespace tiro
