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
    invalid = 0,

    load_null,  // Push null
    load_false, // Push false
    load_true,  // Push true
    load_int,   // (i : i64), push constant 64 bit integer i
    load_float, // (f : f64), push constant 64 bit float f

    load_const,   // (i : u32), push constant at index i
    load_param,   // (i : u32), push parameter at index i
    store_param,  // (i : u32), pop a and set parameter at index i to a
    load_local,   // (i : u32), push local variable at index i
    store_local,  // (i : u32), pop a and set local variable at index i to a
    load_env,     // (n : u32, i : u32), push captured variable at level n and index i
    store_env,    // (n : u32, i : u32), pop a and set captured variable at level n and index i to a
    load_member,  // (i : u32) Pop obj. Push obj."constants[i]"
    store_member, // (i : u32) Pop obj, v. Set obj."contants[i]" = v
    load_index,   // Pop a, i. Push a[i].
    store_index,  // Pop a, i, v. Set a[i] = v.
    load_module,  // (i : u32), push module variable at index i
    store_module, // (i : u32), pop a and set module variable at index i to a
    load_global,  // (i : u32), push global variable called "constants[i]"

    dup,   // Push top
    pop,   // Pop top
    rot_2, // Pop a, b. Push b, a.
    rot_3, // Pop a, b, c. Push c, a, b.
    rot_4, // Pop a, b, c, d. Push d, a, b, c.

    add,  // Pop a, b. Push a + b
    sub,  // Pop a, b. Push a - b
    mul,  // Pop a, b. Push a * b
    div,  // Pop a, b. Push a / b
    mod,  // Pop a, b. Push a % b
    pow,  // Pop a, b. Push pow(a, b)
    lnot, // Pop a. Push !a.
    bnot, // Pop a. Push ~a.
    upos, // Pop a, Push +a.
    uneg, // Pop a, Push -a.

    // TODO xor missing, shifts missing

    gt,  // Pop a, b. Push a > b.
    gte, // Pop a, b. Push a >= b.
    lt,  // Pop a, b. Push a < b.
    lte, // Pop a, b. Push a <= b.
    eq,  // Pop a, b. Push a == b.
    neq, // Pop a, b. Push a != b.

    jmp,           // (o: u32), jump to offset o
    jmp_true,      // (o: u32), jump to offset o if top is true
    jmp_true_pop,  // (o: u32), jump to offset o if top is true, pop in any case
    jmp_false,     // (o: u32), jump to offset o if top is false
    jmp_false_pop, // (o: u32), jump to offset o if top is false, pop in any case
    call,          // (n: u32), pop func, arg1, ..., argn and call func(arg1, ..., argn)
    ret,           // Pop v and return v to the caller

    // TODO: varargs call, keyword arguments, tail calls.

    // TODO function "is_valid_opcode(u8)"
    last_opcode = ret,
};

std::string_view to_string(Opcode i);

std::string disassemble_instructions(Span<const byte> code);

} // namespace hammer

#endif // HAMMER_COMPILER_OPCODES_HPP
