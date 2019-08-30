#include "hammer/compiler/opcodes.hpp"

#include "hammer/core/defs.hpp"
#include "hammer/compiler/binary.hpp"

#include <fmt/format.h>

namespace hammer {

std::string_view to_string(Opcode op) {
#define HAMMER_CASE(code) \
    case Opcode::code:    \
        return #code;

    switch (op) {
        HAMMER_CASE(invalid)

        HAMMER_CASE(load_null)
        HAMMER_CASE(load_false)
        HAMMER_CASE(load_true)
        HAMMER_CASE(load_int)
        HAMMER_CASE(load_float)
        HAMMER_CASE(load_const)

        HAMMER_CASE(load_param)
        HAMMER_CASE(store_param)
        HAMMER_CASE(load_local)
        HAMMER_CASE(store_local)
        HAMMER_CASE(load_env)
        HAMMER_CASE(store_env)
        HAMMER_CASE(load_member)
        HAMMER_CASE(store_member)
        HAMMER_CASE(load_index)
        HAMMER_CASE(store_index)
        HAMMER_CASE(load_module)
        HAMMER_CASE(store_module)
        HAMMER_CASE(load_global)

        HAMMER_CASE(dup)
        HAMMER_CASE(pop)
        HAMMER_CASE(rot_2)
        HAMMER_CASE(rot_3)
        HAMMER_CASE(rot_4)

        HAMMER_CASE(add)
        HAMMER_CASE(sub)
        HAMMER_CASE(mul)
        HAMMER_CASE(div)
        HAMMER_CASE(mod)
        HAMMER_CASE(pow)
        HAMMER_CASE(lnot)
        HAMMER_CASE(bnot)
        HAMMER_CASE(upos)
        HAMMER_CASE(uneg)

        HAMMER_CASE(gt)
        HAMMER_CASE(gte)
        HAMMER_CASE(lt)
        HAMMER_CASE(lte)
        HAMMER_CASE(eq)
        HAMMER_CASE(neq)

        HAMMER_CASE(jmp)
        HAMMER_CASE(jmp_true)
        HAMMER_CASE(jmp_true_pop)
        HAMMER_CASE(jmp_false)
        HAMMER_CASE(jmp_false_pop)
        HAMMER_CASE(call)
        HAMMER_CASE(ret)
    }

#undef HAMMER_CASE

    HAMMER_UNREACHABLE("Invalid opcode.");
}

std::string disassemble_instructions(Span<const byte> code) {
    fmt::memory_buffer buf;

    const size_t pos_digits = fmt::formatted_size("{}", code.size());

    CheckedBinaryReader reader(code);
    while (reader.remaining() > 0) {
        const u32 pos = reader.pos();
        const u8 op_raw = reader.read_u8();
        if (op_raw == 0 || op_raw > static_cast<u8>(Opcode::last_opcode))
            HAMMER_ERROR("Invalid opcode number: {}.", op_raw);

        const Opcode op = static_cast<Opcode>(op_raw);

        fmt::format_to(buf, "{0: >{1}}: {2}", pos, pos_digits, to_string(op));

        switch (op) {
        case Opcode::invalid:
            HAMMER_ERROR("Invalid instruction at position {}.", pos);
            break;

        case Opcode::load_int:
            fmt::format_to(buf, " {}", reader.read_i64());
            break;

        case Opcode::load_float:
            fmt::format_to(buf, " {}", reader.read_f64());
            break;

        case Opcode::load_const:
            fmt::format_to(buf, " {}", reader.read_u32());
            break;

        case Opcode::load_param:
        case Opcode::store_param:
        case Opcode::load_local:
        case Opcode::store_local:
        case Opcode::load_member:
        case Opcode::store_member:
        case Opcode::load_module:
        case Opcode::store_module:
        case Opcode::load_global:
            fmt::format_to(buf, " {}", reader.read_u32());
            break;

        case Opcode::load_env:
        case Opcode::store_env:
            fmt::format_to(buf, " {} {}", reader.read_u32(), reader.read_u32());
            break;

        case Opcode::jmp:
        case Opcode::jmp_true:
        case Opcode::jmp_true_pop:
        case Opcode::jmp_false:
        case Opcode::jmp_false_pop:
            fmt::format_to(buf, " {}", reader.read_u32());
            break;

        case Opcode::call:
            fmt::format_to(buf, " {}", reader.read_u32());
            break;

        case Opcode::load_null:
        case Opcode::load_false:
        case Opcode::load_true:
        case Opcode::load_index:
        case Opcode::store_index:
        case Opcode::dup:
        case Opcode::pop:
        case Opcode::rot_2:
        case Opcode::rot_3:
        case Opcode::rot_4:
        case Opcode::add:
        case Opcode::sub:
        case Opcode::mul:
        case Opcode::div:
        case Opcode::mod:
        case Opcode::pow:
        case Opcode::lnot:
        case Opcode::bnot:
        case Opcode::upos:
        case Opcode::uneg:
        case Opcode::gt:
        case Opcode::gte:
        case Opcode::lt:
        case Opcode::lte:
        case Opcode::eq:
        case Opcode::neq:
        case Opcode::ret:
            break;
        }

        fmt::format_to(buf, "\n");
    }

    return to_string(buf);
}

} // namespace hammer
