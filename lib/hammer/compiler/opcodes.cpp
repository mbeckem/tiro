#include "hammer/compiler/opcodes.hpp"

#include "hammer/compiler/binary.hpp"
#include "hammer/core/defs.hpp"

#include <fmt/format.h>

namespace hammer {

std::string_view to_string(Opcode op) {
#define HAMMER_CASE(code) \
    case Opcode::code:    \
        return #code;

    switch (op) {
        HAMMER_CASE(Invalid)

        HAMMER_CASE(LoadNull)
        HAMMER_CASE(LoadFalse)
        HAMMER_CASE(LoadTrue)
        HAMMER_CASE(LoadInt)
        HAMMER_CASE(LoadFloat)
        HAMMER_CASE(LoadConst)

        HAMMER_CASE(LoadParam)
        HAMMER_CASE(StoreParam)
        HAMMER_CASE(LoadLocal)
        HAMMER_CASE(StoreLocal)
        HAMMER_CASE(LoadEnv)
        HAMMER_CASE(StoreEnv)
        HAMMER_CASE(LoadMember)
        HAMMER_CASE(StoreMember)
        HAMMER_CASE(LoadIndex)
        HAMMER_CASE(StoreIndex)
        HAMMER_CASE(LoadModule)
        HAMMER_CASE(StoreModule)
        HAMMER_CASE(LoadGlobal)

        HAMMER_CASE(Dup)
        HAMMER_CASE(Pop)
        HAMMER_CASE(Rot2)
        HAMMER_CASE(Rot3)
        HAMMER_CASE(Rot4)

        HAMMER_CASE(Add)
        HAMMER_CASE(Sub)
        HAMMER_CASE(Mul)
        HAMMER_CASE(Div)
        HAMMER_CASE(Mod)
        HAMMER_CASE(Pow)
        HAMMER_CASE(LNot)
        HAMMER_CASE(BNot)
        HAMMER_CASE(UPos)
        HAMMER_CASE(UNeg)

        HAMMER_CASE(LSh)
        HAMMER_CASE(RSh)
        HAMMER_CASE(BAnd)
        HAMMER_CASE(BOr)
        HAMMER_CASE(BXor)

        HAMMER_CASE(Gt)
        HAMMER_CASE(Gte)
        HAMMER_CASE(Lt)
        HAMMER_CASE(Lte)
        HAMMER_CASE(Eq)
        HAMMER_CASE(NEq)

        HAMMER_CASE(MkArray)
        HAMMER_CASE(MkTuple)
        HAMMER_CASE(MkSet)
        HAMMER_CASE(MkMap)

        HAMMER_CASE(Jmp)
        HAMMER_CASE(JmpTrue)
        HAMMER_CASE(JmpTruePop)
        HAMMER_CASE(JmpFalse)
        HAMMER_CASE(JmpFalsePop)
        HAMMER_CASE(Call)
        HAMMER_CASE(Ret)

        HAMMER_CASE(AssertFail)
    }

#undef HAMMER_CASE

    HAMMER_UNREACHABLE("Invalid opcode.");
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
            HAMMER_ERROR("Invalid opcode number: {}.", op_raw);

        const Opcode op = static_cast<Opcode>(op_raw);

        fmt::format_to(buf, "{0: >{1}}: {2}", pos, pos_digits, to_string(op));

        switch (op) {
        case Opcode::Invalid:
            HAMMER_ERROR("Invalid instruction at position {}.", pos);
            break;

        case Opcode::LoadInt:
            fmt::format_to(buf, " {}", reader.read_i64());
            break;

        case Opcode::LoadFloat:
            fmt::format_to(buf, " {}", reader.read_f64());
            break;

        case Opcode::LoadConst:
            fmt::format_to(buf, " {}", reader.read_u32());
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
            fmt::format_to(buf, " {}", reader.read_u32());
            break;

        case Opcode::LoadEnv:
        case Opcode::StoreEnv:
            fmt::format_to(buf, " {} {}", reader.read_u32(), reader.read_u32());
            break;

        case Opcode::MkArray:
        case Opcode::MkTuple:
        case Opcode::MkMap:
        case Opcode::MkSet:
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

        case Opcode::LoadNull:
        case Opcode::LoadFalse:
        case Opcode::LoadTrue:
        case Opcode::LoadIndex:
        case Opcode::StoreIndex:
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
        case Opcode::Ret:

        case Opcode::AssertFail:
            break;
        }

        fmt::format_to(buf, "\n");
    }

    return to_string(buf);
}

} // namespace hammer
