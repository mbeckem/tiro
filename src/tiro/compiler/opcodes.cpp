#include "tiro/compiler/opcodes.hpp"

#include "tiro/compiler/binary.hpp"
#include "tiro/core/defs.hpp"

#include <fmt/format.h>

namespace tiro {

std::string_view to_string(OldOpcode op) {
#define TIRO_CASE(code)   \
    case OldOpcode::code: \
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
        TIRO_CASE(PopN)
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

bool valid_old_opcode(u8 op) {
    return op > static_cast<u8>(OldOpcode::Invalid)
           && op <= static_cast<u8>(OldOpcode::LastOpcode);
}

std::string disassemble_instructions(Span<const byte> code) {
    fmt::memory_buffer buf;

    const size_t pos_digits = fmt::formatted_size("{}", code.size());

    CheckedBinaryReader reader(code);
    while (reader.remaining() > 0) {
        const u32 pos = reader.pos();
        const u8 op_raw = reader.read_u8();
        if (op_raw == 0 || op_raw > static_cast<u8>(OldOpcode::LastOpcode))
            TIRO_ERROR("Invalid opcode number: {}.", op_raw);

        const OldOpcode op = static_cast<OldOpcode>(op_raw);

        fmt::format_to(buf, "{0: >{1}}: {2}", pos, pos_digits, to_string(op));

        switch (op) {
        case OldOpcode::Invalid:
            TIRO_ERROR("Invalid instruction at position {}.", pos);
            break;

        case OldOpcode::LoadInt:
            fmt::format_to(buf, " {}", reader.read_i64());
            break;

        case OldOpcode::LoadFloat:
            fmt::format_to(buf, " {}", reader.read_f64());
            break;

        case OldOpcode::LoadParam:
        case OldOpcode::StoreParam:
        case OldOpcode::LoadLocal:
        case OldOpcode::StoreLocal:
        case OldOpcode::LoadMember:
        case OldOpcode::StoreMember:
        case OldOpcode::LoadModule:
        case OldOpcode::StoreModule:
        case OldOpcode::LoadGlobal:
        case OldOpcode::LoadTupleMember:
        case OldOpcode::StoreTupleMember:
            fmt::format_to(buf, " {}", reader.read_u32());
            break;

        case OldOpcode::LoadContext:
        case OldOpcode::StoreContext: {
            const u32 n = reader.read_u32();
            const u32 i = reader.read_u32();
            fmt::format_to(buf, " {} {}", n, i);
            break;
        }

        case OldOpcode::MkArray:
        case OldOpcode::MkTuple:
        case OldOpcode::MkMap:
        case OldOpcode::MkSet:
        case OldOpcode::MkContext:
            fmt::format_to(buf, " {}", reader.read_u32());
            break;

        case OldOpcode::Jmp:
        case OldOpcode::JmpTrue:
        case OldOpcode::JmpTruePop:
        case OldOpcode::JmpFalse:
        case OldOpcode::JmpFalsePop:
            fmt::format_to(buf, " {}", reader.read_u32());
            break;

        case OldOpcode::Call:
        case OldOpcode::PopN:
            fmt::format_to(buf, " {}", reader.read_u32());
            break;

        case OldOpcode::LoadMethod:
        case OldOpcode::CallMethod:
            fmt::format_to(buf, " {}", reader.read_u32());
            break;

        case OldOpcode::LoadNull:
        case OldOpcode::LoadFalse:
        case OldOpcode::LoadTrue:
        case OldOpcode::LoadIndex:
        case OldOpcode::StoreIndex:
        case OldOpcode::LoadClosure:
        case OldOpcode::Dup:
        case OldOpcode::Pop:
        case OldOpcode::Rot2:
        case OldOpcode::Rot3:
        case OldOpcode::Rot4:
        case OldOpcode::Add:
        case OldOpcode::Sub:
        case OldOpcode::Mul:
        case OldOpcode::Div:
        case OldOpcode::Mod:
        case OldOpcode::Pow:
        case OldOpcode::LNot:
        case OldOpcode::BNot:
        case OldOpcode::UPos:
        case OldOpcode::UNeg:
        case OldOpcode::MkBuilder:
        case OldOpcode::BuilderAppend:
        case OldOpcode::BuilderString:

        case OldOpcode::LSh:
        case OldOpcode::RSh:
        case OldOpcode::BAnd:
        case OldOpcode::BOr:
        case OldOpcode::BXor:

        case OldOpcode::Gt:
        case OldOpcode::Gte:
        case OldOpcode::Lt:
        case OldOpcode::Lte:
        case OldOpcode::Eq:
        case OldOpcode::NEq:
        case OldOpcode::MkClosure:
        case OldOpcode::Ret:

        case OldOpcode::AssertFail:
            break;
        }

        fmt::format_to(buf, "\n");
    }

    return to_string(buf);
}

} // namespace tiro
