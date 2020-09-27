#include "compiler/bytecode/op.hpp"

namespace tiro {

/* [[[cog
    from codegen.unions import implement
    from codegen.bytecode import BytecodeOp
    implement(BytecodeOp)
]]] */
std::string_view to_string(BytecodeOp type) {
    switch (type) {
    case BytecodeOp::LoadNull:
        return "LoadNull";
    case BytecodeOp::LoadFalse:
        return "LoadFalse";
    case BytecodeOp::LoadTrue:
        return "LoadTrue";
    case BytecodeOp::LoadInt:
        return "LoadInt";
    case BytecodeOp::LoadFloat:
        return "LoadFloat";
    case BytecodeOp::LoadParam:
        return "LoadParam";
    case BytecodeOp::StoreParam:
        return "StoreParam";
    case BytecodeOp::LoadModule:
        return "LoadModule";
    case BytecodeOp::StoreModule:
        return "StoreModule";
    case BytecodeOp::LoadMember:
        return "LoadMember";
    case BytecodeOp::StoreMember:
        return "StoreMember";
    case BytecodeOp::LoadTupleMember:
        return "LoadTupleMember";
    case BytecodeOp::StoreTupleMember:
        return "StoreTupleMember";
    case BytecodeOp::LoadIndex:
        return "LoadIndex";
    case BytecodeOp::StoreIndex:
        return "StoreIndex";
    case BytecodeOp::LoadClosure:
        return "LoadClosure";
    case BytecodeOp::LoadEnv:
        return "LoadEnv";
    case BytecodeOp::StoreEnv:
        return "StoreEnv";
    case BytecodeOp::Add:
        return "Add";
    case BytecodeOp::Sub:
        return "Sub";
    case BytecodeOp::Mul:
        return "Mul";
    case BytecodeOp::Div:
        return "Div";
    case BytecodeOp::Mod:
        return "Mod";
    case BytecodeOp::Pow:
        return "Pow";
    case BytecodeOp::UAdd:
        return "UAdd";
    case BytecodeOp::UNeg:
        return "UNeg";
    case BytecodeOp::LSh:
        return "LSh";
    case BytecodeOp::RSh:
        return "RSh";
    case BytecodeOp::BAnd:
        return "BAnd";
    case BytecodeOp::BOr:
        return "BOr";
    case BytecodeOp::BXor:
        return "BXor";
    case BytecodeOp::BNot:
        return "BNot";
    case BytecodeOp::Gt:
        return "Gt";
    case BytecodeOp::Gte:
        return "Gte";
    case BytecodeOp::Lt:
        return "Lt";
    case BytecodeOp::Lte:
        return "Lte";
    case BytecodeOp::Eq:
        return "Eq";
    case BytecodeOp::NEq:
        return "NEq";
    case BytecodeOp::LNot:
        return "LNot";
    case BytecodeOp::Array:
        return "Array";
    case BytecodeOp::Tuple:
        return "Tuple";
    case BytecodeOp::Set:
        return "Set";
    case BytecodeOp::Map:
        return "Map";
    case BytecodeOp::Env:
        return "Env";
    case BytecodeOp::Closure:
        return "Closure";
    case BytecodeOp::Record:
        return "Record";
    case BytecodeOp::Iterator:
        return "Iterator";
    case BytecodeOp::IteratorNext:
        return "IteratorNext";
    case BytecodeOp::Formatter:
        return "Formatter";
    case BytecodeOp::AppendFormat:
        return "AppendFormat";
    case BytecodeOp::FormatResult:
        return "FormatResult";
    case BytecodeOp::Copy:
        return "Copy";
    case BytecodeOp::Swap:
        return "Swap";
    case BytecodeOp::Push:
        return "Push";
    case BytecodeOp::Pop:
        return "Pop";
    case BytecodeOp::PopTo:
        return "PopTo";
    case BytecodeOp::Jmp:
        return "Jmp";
    case BytecodeOp::JmpTrue:
        return "JmpTrue";
    case BytecodeOp::JmpFalse:
        return "JmpFalse";
    case BytecodeOp::JmpNull:
        return "JmpNull";
    case BytecodeOp::JmpNotNull:
        return "JmpNotNull";
    case BytecodeOp::Call:
        return "Call";
    case BytecodeOp::LoadMethod:
        return "LoadMethod";
    case BytecodeOp::CallMethod:
        return "CallMethod";
    case BytecodeOp::Return:
        return "Return";
    case BytecodeOp::AssertFail:
        return "AssertFail";
    }
    TIRO_UNREACHABLE("Invalid BytecodeOp.");
}
// [[[end]]]

/* [[[cog
    import cog
    from codegen.bytecode import InstructionList

    first = InstructionList[0];
    last = InstructionList[-1];

    cog.outl(f"static constexpr auto first_opcode = BytecodeOp::{first.name};")
    cog.outl(f"static constexpr auto last_opcode = BytecodeOp::{last.name};")
]]] */
static constexpr auto first_opcode = BytecodeOp::LoadNull;
static constexpr auto last_opcode = BytecodeOp::AssertFail;
/// [[[end]]]

bool valid_opcode(u8 raw_op) {
    return raw_op >= static_cast<u8>(first_opcode) && raw_op <= static_cast<u8>(last_opcode);
}

bool references_offset(BytecodeOp op) {
    switch (op) {
    /* [[[cog
        import cog
        from codegen.bytecode import InstructionList, Offset

        count = 0
        for ins in InstructionList:
            if any(isinstance(p, Offset) for p in ins.params):
                cog.outl(f"case BytecodeOp::{ins.name}:")
                count += 1
        
        if count > 0:
            cog.outl("return true;")
    ]]] */
    case BytecodeOp::Jmp:
    case BytecodeOp::JmpTrue:
    case BytecodeOp::JmpFalse:
    case BytecodeOp::JmpNull:
    case BytecodeOp::JmpNotNull:
        return true;
        /// [[[end]]]

    default:
        return false;
    }
}

bool references_module(BytecodeOp op) {
    switch (op) {
    /* [[[cog
        import cog
        from codegen.bytecode import InstructionList, Module

        count = 0
        for ins in InstructionList:
            if any(isinstance(p, Module) for p in ins.params):
                cog.outl(f"case BytecodeOp::{ins.name}:")
                count += 1
        
        if count > 0:
            cog.outl("return true;")
    ]]] */
    case BytecodeOp::LoadModule:
    case BytecodeOp::StoreModule:
    case BytecodeOp::LoadMember:
    case BytecodeOp::StoreMember:
    case BytecodeOp::Record:
    case BytecodeOp::LoadMethod:
        return true;
        /// [[[end]]]

    default:
        return false;
    }
}

} // namespace tiro
