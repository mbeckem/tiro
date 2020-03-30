#include "tiro/bytecode/opcode.hpp"

namespace tiro {

/* [[[cog
    import unions
    import bytecode
    unions.implement_type(bytecode.Opcode)
]]] */
std::string_view to_string(Opcode type) {
    switch (type) {
    case Opcode::LoadNull:
        return "LoadNull";
    case Opcode::LoadFalse:
        return "LoadFalse";
    case Opcode::LoadTrue:
        return "LoadTrue";
    case Opcode::LoadInt:
        return "LoadInt";
    case Opcode::LoadFloat:
        return "LoadFloat";
    case Opcode::LoadParam:
        return "LoadParam";
    case Opcode::StoreParam:
        return "StoreParam";
    case Opcode::LoadModule:
        return "LoadModule";
    case Opcode::StoreModule:
        return "StoreModule";
    case Opcode::LoadMember:
        return "LoadMember";
    case Opcode::StoreMember:
        return "StoreMember";
    case Opcode::LoadTupleMember:
        return "LoadTupleMember";
    case Opcode::StoreTupleMember:
        return "StoreTupleMember";
    case Opcode::LoadIndex:
        return "LoadIndex";
    case Opcode::StoreIndex:
        return "StoreIndex";
    case Opcode::LoadClosure:
        return "LoadClosure";
    case Opcode::LoadEnv:
        return "LoadEnv";
    case Opcode::StoreEnv:
        return "StoreEnv";
    case Opcode::Add:
        return "Add";
    case Opcode::Sub:
        return "Sub";
    case Opcode::Mul:
        return "Mul";
    case Opcode::Div:
        return "Div";
    case Opcode::Mod:
        return "Mod";
    case Opcode::Pow:
        return "Pow";
    case Opcode::UAdd:
        return "UAdd";
    case Opcode::UNeg:
        return "UNeg";
    case Opcode::LSh:
        return "LSh";
    case Opcode::RSh:
        return "RSh";
    case Opcode::BAnd:
        return "BAnd";
    case Opcode::BOr:
        return "BOr";
    case Opcode::BXor:
        return "BXor";
    case Opcode::BNot:
        return "BNot";
    case Opcode::Gt:
        return "Gt";
    case Opcode::Gte:
        return "Gte";
    case Opcode::Lt:
        return "Lt";
    case Opcode::Lte:
        return "Lte";
    case Opcode::Eq:
        return "Eq";
    case Opcode::NEq:
        return "NEq";
    case Opcode::LNot:
        return "LNot";
    case Opcode::Array:
        return "Array";
    case Opcode::Tuple:
        return "Tuple";
    case Opcode::Set:
        return "Set";
    case Opcode::Map:
        return "Map";
    case Opcode::Env:
        return "Env";
    case Opcode::Closure:
        return "Closure";
    case Opcode::Formatter:
        return "Formatter";
    case Opcode::AppendFormat:
        return "AppendFormat";
    case Opcode::FormatResult:
        return "FormatResult";
    case Opcode::Copy:
        return "Copy";
    case Opcode::Swap:
        return "Swap";
    case Opcode::Push:
        return "Push";
    case Opcode::Pop:
        return "Pop";
    case Opcode::PopTo:
        return "PopTo";
    case Opcode::Jmp:
        return "Jmp";
    case Opcode::JmpTrue:
        return "JmpTrue";
    case Opcode::JmpFalse:
        return "JmpFalse";
    case Opcode::Call:
        return "Call";
    case Opcode::LoadMethod:
        return "LoadMethod";
    case Opcode::CallMethod:
        return "CallMethod";
    case Opcode::Return:
        return "Return";
    case Opcode::AssertFail:
        return "AssertFail";
    }
    TIRO_UNREACHABLE("Invalid Opcode.");
}
// [[[end]]]

/* [[[cog
    import cog
    import bytecode

    first = bytecode.InstructionList[0];
    last = bytecode.InstructionList[-1];

    cog.outl(f"static constexpr auto first_opcode = Opcode::{first.name};")
    cog.outl(f"static constexpr auto last_opcode = Opcode::{last.name};")
]]] */
static constexpr auto first_opcode = Opcode::LoadNull;
static constexpr auto last_opcode = Opcode::AssertFail;
/// [[[end]]]

bool valid_opcode(u8 raw_op) {
    return raw_op >= static_cast<u8>(first_opcode)
           && raw_op <= static_cast<u8>(last_opcode);
}

bool references_offset(Opcode op) {
    switch (op) {
    /* [[[cog
        import cog
        import bytecode

        count = 0
        for ins in bytecode.InstructionList:
            if any(isinstance(p, bytecode.Offset) for p in ins.params):
                cog.outl(f"case Opcode::{ins.name}:")
                count += 1
        
        if count > 0:
            cog.outl("return true;")
    ]]] */
    case Opcode::Jmp:
    case Opcode::JmpTrue:
    case Opcode::JmpFalse:
        return true;
        /// [[[end]]]

    default:
        return false;
    }
}

bool references_module(Opcode op) {
    switch (op) {
    /* [[[cog
        import cog
        import bytecode

        count = 0
        for ins in bytecode.InstructionList:
            if any(isinstance(p, bytecode.Module) for p in ins.params):
                cog.outl(f"case Opcode::{ins.name}:")
                count += 1
        
        if count > 0:
            cog.outl("return true;")
    ]]] */
    case Opcode::LoadModule:
    case Opcode::StoreModule:
    case Opcode::LoadMember:
    case Opcode::StoreMember:
    case Opcode::LoadMethod:
        return true;
        /// [[[end]]]

    default:
        return false;
    }
}

} // namespace tiro
