#include "tiro/compiler/opcodes.hpp"

#include "tiro/compiler/binary.hpp"
#include "tiro/core/defs.hpp"

#include <fmt/format.h>

namespace tiro::compiler {

std::string_view to_string(Opcode op) {
#define TIRO_CASE(code) \
    case Opcode::code:    \
        return #code;

    switch (op) {
        TIRO_CASE(Invalid)

        TIRO_CASE(LoadNull)
        TIRO_CASE(LoadFalse)
        TIRO_CASE(LoadTrue)
        TIRO_CASE(LoadInt)
        TIRO_CASE(LoadFloat)

        TIRO_CASE(LoadParam)
        TIRO_CASE(StoreParam)
        TIRO_CASE(LoadLocal)
        TIRO_CASE(StoreLocal)
        TIRO_CASE(LoadClosure)
        TIRO_CASE(LoadContext)
        TIRO_CASE(StoreContext)

        TIRO_CASE(LoadMember)
        TIRO_CASE(StoreMember)
        TIRO_CASE(LoadTupleMember)
        TIRO_CASE(StoreTupleMember)
        TIRO_CASE(LoadIndex)
        TIRO_CASE(StoreIndex)
        TIRO_CASE(LoadModule)
        TIRO_CASE(StoreModule)
        TIRO_CASE(LoadGlobal)

        TIRO_CASE(Dup)
        TIRO_CASE(Pop)
        TIRO_CASE(Rot2)
        TIRO_CASE(Rot3)
        TIRO_CASE(Rot4)

        TIRO_CASE(Add)
        TIRO_CASE(Sub)
        TIRO_CASE(Mul)
        TIRO_CASE(Div)
        TIRO_CASE(Mod)
        TIRO_CASE(Pow)
        TIRO_CASE(LNot)
        TIRO_CASE(BNot)
        TIRO_CASE(UPos)
        TIRO_CASE(UNeg)

        TIRO_CASE(LSh)
        TIRO_CASE(RSh)
        TIRO_CASE(BAnd)
        TIRO_CASE(BOr)
        TIRO_CASE(BXor)

        TIRO_CASE(Gt)
        TIRO_CASE(Gte)
        TIRO_CASE(Lt)
        TIRO_CASE(Lte)
        TIRO_CASE(Eq)
        TIRO_CASE(NEq)

        TIRO_CASE(MkArray)
        TIRO_CASE(MkTuple)
        TIRO_CASE(MkSet)
        TIRO_CASE(MkMap)
        TIRO_CASE(MkContext)
        TIRO_CASE(MkClosure)

        TIRO_CASE(MkBuilder)
        TIRO_CASE(BuilderAppend)
        TIRO_CASE(BuilderString)

        TIRO_CASE(Jmp)
        TIRO_CASE(JmpTrue)
        TIRO_CASE(JmpTruePop)
        TIRO_CASE(JmpFalse)
        TIRO_CASE(JmpFalsePop)
        TIRO_CASE(Call)
        TIRO_CASE(Ret)

        TIRO_CASE(LoadMethod)
        TIRO_CASE(CallMethod)

        TIRO_CASE(AssertFail)
    }

#undef TIRO_CASE

    TIRO_UNREACHABLE("Invalid opcode.");
}

bool valid_opcode(u8 op) {
    return op > static_cast<u8>(Opcode::Invalid)
           && op <= static_cast<u8>(Opcode::LastOpcode);
}

std::string disassemble_instructions(Span<const byte> code) {
    fmt::memory_buffer buf;

    const size_t pos_digits = fmt::formatted_size("{}", code.size());

    CheckedBinaryReader reader(code);
    while (reader.remaining() > 0) {
        const u32 pos = reader.pos();
        const u8 op_raw = reader.read_u8();
        if (op_raw == 0 || op_raw > static_cast<u8>(Opcode::LastOpcode))
            TIRO_ERROR("Invalid opcode number: {}.", op_raw);

        const Opcode op = static_cast<Opcode>(op_raw);

        fmt::format_to(buf, "{0: >{1}}: {2}", pos, pos_digits, to_string(op));

        switch (op) {
        case Opcode::Invalid:
            TIRO_ERROR("Invalid instruction at position {}.", pos);
            break;

        case Opcode::LoadInt:
            fmt::format_to(buf, " {}", reader.read_i64());
            break;

        case Opcode::LoadFloat:
            fmt::format_to(buf, " {}", reader.read_f64());
            break;

        case Opcode::LoadParam:
        case Opcode::StoreParam:
        case Opcode::LoadLocal:
        case Opcode::StoreLocal:
        case Opcode::LoadMember:
        case Opcode::StoreMember:
        case Opcode::LoadModule:
        case Opcode::StoreModule:
        case Opcode::LoadGlobal:
        case Opcode::LoadTupleMember:
        case Opcode::StoreTupleMember:
            fmt::format_to(buf, " {}", reader.read_u32());
            break;

        case Opcode::LoadContext:
        case Opcode::StoreContext: {
            const u32 n = reader.read_u32();
            const u32 i = reader.read_u32();
            fmt::format_to(buf, " {} {}", n, i);
            break;
        }

        case Opcode::MkArray:
        case Opcode::MkTuple:
        case Opcode::MkMap:
        case Opcode::MkSet:
        case Opcode::MkContext:
            fmt::format_to(buf, " {}", reader.read_u32());
            break;

        case Opcode::Jmp:
        case Opcode::JmpTrue:
        case Opcode::JmpTruePop:
        case Opcode::JmpFalse:
        case Opcode::JmpFalsePop:
            fmt::format_to(buf, " {}", reader.read_u32());
            break;

        case Opcode::Call:
            fmt::format_to(buf, " {}", reader.read_u32());
            break;

        case Opcode::LoadMethod:
        case Opcode::CallMethod:
            fmt::format_to(buf, " {}", reader.read_u32());
            break;

        case Opcode::LoadNull:
        case Opcode::LoadFalse:
        case Opcode::LoadTrue:
        case Opcode::LoadIndex:
        case Opcode::StoreIndex:
        case Opcode::LoadClosure:
        case Opcode::Dup:
        case Opcode::Pop:
        case Opcode::Rot2:
        case Opcode::Rot3:
        case Opcode::Rot4:
        case Opcode::Add:
        case Opcode::Sub:
        case Opcode::Mul:
        case Opcode::Div:
        case Opcode::Mod:
        case Opcode::Pow:
        case Opcode::LNot:
        case Opcode::BNot:
        case Opcode::UPos:
        case Opcode::UNeg:
        case Opcode::MkBuilder:
        case Opcode::BuilderAppend:
        case Opcode::BuilderString:

        case Opcode::LSh:
        case Opcode::RSh:
        case Opcode::BAnd:
        case Opcode::BOr:
        case Opcode::BXor:

        case Opcode::Gt:
        case Opcode::Gte:
        case Opcode::Lt:
        case Opcode::Lte:
        case Opcode::Eq:
        case Opcode::NEq:
        case Opcode::MkClosure:
        case Opcode::Ret:

        case Opcode::AssertFail:
            break;
        }

        fmt::format_to(buf, "\n");
    }

    return to_string(buf);
}

} // namespace tiro::compiler
