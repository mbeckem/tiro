#ifndef HAMMER_COMPILER_OPCODES_HPP
#define HAMMER_COMPILER_OPCODES_HPP

#include "hammer/core/defs.hpp"
#include "hammer/core/span.hpp"

#include <string>
#include <string_view>
#include <vector>

namespace hammer {

/*
 * Instructions for the virtual stack machine.
 * Instructions pop values off the stack and/or push values onto the stack.
 * 
 * If an instruction requires multiple arguments, then those arguments must be 
 * pushed in their documented order.
 * 
 * For example, the sequece of instructions
 *      
 *      load_int 10
 *      load_int 5
 *      div
 * 
 * will compute 10 / 5.
 * 
 * In the following documentation, "top" refers to the current value on top of the stack.
 */
enum class Opcode : u8 {
    Invalid = 0,

    LoadNull,  // Push null
    LoadFalse, // Push false
    LoadTrue,  // Push true
    LoadInt,   // (i : i64), push constant 64 bit integer i
    LoadFloat, // (f : f64), push constant 64 bit float f

    LoadConst,  // (i : u32), push constant at index i
    LoadParam,  // (i : u32), push parameter at index i
    StoreParam, // (i : u32), pop a and set parameter at index i to a
    LoadLocal,  // (i : u32), push local variable at index i
    StoreLocal, // (i : u32), pop a and set local variable at index i to a
    LoadEnv, // (n : u32, i : u32), push captured variable at level n and index i
    StoreEnv, // (n : u32, i : u32), pop a and set captured variable at level n and index i to a
    LoadMember,  // (i : u32) Pop obj. Push obj."constants[i]"
    StoreMember, // (i : u32) Pop obj, v. Set obj."contants[i]" = v
    LoadIndex,   // Pop a, i. Push a[i].
    StoreIndex,  // Pop a, i, v. Set a[i] = v.
    LoadModule,  // (i : u32), push module variable at index i
    StoreModule, // (i : u32), pop a and set module variable at index i to a
    LoadGlobal,  // (i : u32), push global variable called "constants[i]"

    Dup,  // Push top
    Pop,  // Pop top
    Rot2, // Pop a, b. Push b, a.
    Rot3, // Pop a, b, c. Push c, a, b.
    Rot4, // Pop a, b, c, d. Push d, a, b, c.

    Add,  // Pop a, b. Push a + b
    Sub,  // Pop a, b. Push a - b
    Mul,  // Pop a, b. Push a * b
    Div,  // Pop a, b. Push a / b
    Mod,  // Pop a, b. Push a % b
    Pow,  // Pop a, b. Push pow(a, b)
    LNot, // Pop a. Push !a.
    BNot, // Pop a. Push ~a.
    UPos, // Pop a, Push +a.
    UNeg, // Pop a, Push -a.

    LSh,  // Pop a, b. Push a << b.
    RSh,  // Pop a, b. Push a >> b.
    BAnd, // Pop a, b. Push a & b.
    BOr,  // Pop a, b. Push a | b.
    BXor, // Pop a, b. Push a ^ b.

    // TODO xor missing, shifts missing

    Gt,  // Pop a, b. Push a > b.
    Gte, // Pop a, b. Push a >= b.
    Lt,  // Pop a, b. Push a < b.
    Lte, // Pop a, b. Push a <= b.
    Eq,  // Pop a, b. Push a == b.
    NEq, // Pop a, b. Push a != b.

    Jmp,         // (o: u32), jump to offset o
    JmpTrue,     // (o: u32), jump to offset o if top is true
    JmpTruePop,  // (o: u32), jump to offset o if top is true, pop in any case
    JmpFalse,    // (o: u32), jump to offset o if top is false
    JmpFalsePop, // (o: u32), jump to offset o if top is false, pop in any case
    Call, // (n: u32), pop func, arg1, ..., argn and call func(arg1, ..., argn)
    Ret,  // Pop v and return v to the caller

    // TODO: varargs call, keyword arguments, tail calls.

    // TODO function "is_valid_opcode(u8)"
    LastOpcode = Ret,
};

std::string_view to_string(Opcode i);

std::string disassemble_instructions(Span<const byte> code);

} // namespace hammer

#endif // HAMMER_COMPILER_OPCODES_HPP
