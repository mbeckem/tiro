#ifndef TIRO_COMPILER_OPCODES_HPP
#define TIRO_COMPILER_OPCODES_HPP

#include "tiro/core/defs.hpp"
#include "tiro/core/span.hpp"

#include <string>
#include <string_view>
#include <vector>

namespace tiro::compiler {

/// Instructions for the virtual stack machine.
/// Instructions pop values off the stack and/or push values onto the stack.
///
/// If an instruction requires multiple arguments, then those arguments must be
/// pushed in their documented order.
///
/// For example, the sequece of instructions
///
///      load_int 10
///      load_int 5
///      div
///
/// will compute 10 / 5.
///
/// In the following documentation, "top" refers to the current value on top of the stack.
// TODO: Revise order of operands. Implement strict left to right expression order?
enum class Opcode : u8 {
    Invalid = 0,

    LoadNull,  // Push null
    LoadFalse, // Push false
    LoadTrue,  // Push true
    LoadInt,   // (i : i64), push constant 64 bit integer i
    LoadFloat, // (f : f64), push constant 64 bit float f

    LoadParam,   // (i : u32), push parameter at index i
    StoreParam,  // (i : u32), pop a and set parameter at index i to a
    LoadLocal,   // (i : u32), push local variable at index i
    StoreLocal,  // (i : u32), pop a and set local variable at index i to a
    LoadClosure, // Push the closure context of the current function.
    LoadContext, // (n : u32, i : u32), pop context, push captured variable at level n and index i
    StoreContext, // (n : u32, i : u32), pop v, context, and set captured variable at level n and index i to v

    LoadMember,       // (i : u32) Pop obj. Push obj."module[i]"
    StoreMember,      // (i : u32) Pop obj, v. Set obj."module[i]" = v
    LoadTupleMember,  // (i : u32) Pop obj. Push "obj.i"
    StoreTupleMember, // (i : u32) Pop obj, v. Set "obj.i" = v
    LoadIndex,        // Pop a, i. Push a[i].
    StoreIndex,       // Pop a, i, v. Set a[i] = v.
    LoadModule, // (i : u32), push module variable at index i  -- TOOD const variant?
    StoreModule, // (i : u32), pop a and set module variable at index i to a
    LoadGlobal, // (i : u32), push global variable called "module[i]" // TODO Needed?

    Dup,  // Push top
    Pop,  // Pop top
    PopN, // (n: u32) Pop n values.
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

    Gt,  // Pop a, b. Push a > b.
    Gte, // Pop a, b. Push a >= b.
    Lt,  // Pop a, b. Push a < b.
    Lte, // Pop a, b. Push a <= b.
    Eq,  // Pop a, b. Push a == b.
    NEq, // Pop a, b. Push a != b.

    MkArray,   // (n: u32). Pop (v1, ..., vn), make an array and push it.
    MkTuple,   // (n: u32). Pop (v1, ..., vn), make a tuple and push it.
    MkSet,     // (n: u32). Pop (v1, ..., vn), make a set and push it.
    MkMap,     // (n: u32). Pop (k1, v1, ..., kn, vn), make a map and push it.
    MkContext, // (n: u32). Pop parent, push a closure context with that parent and room for n variables.
    MkClosure, // Pops function template, closure context, build a new function object and push it.

    MkBuilder,     // Create a string builder.
    BuilderAppend, // Pop builder, value and append value to builder. Push builder.
    BuilderString, // Pop builder, return its content as a string.

    Jmp,         // (o: u32), jump to offset o
    JmpTrue,     // (o: u32), jump to offset o if top is true
    JmpTruePop,  // (o: u32), jump to offset o if top is true, pop in any case
    JmpFalse,    // (o: u32), jump to offset o if top is false
    JmpFalsePop, // (o: u32), jump to offset o if top is false, pop in any case
    Call, // (n: u32), pop func, arg1, ..., argn and push func(arg1, ..., argn)
    Ret,  // Pop v and return v to the caller

    // (i : u32) Pop obj. Pushes the method of obj with name "module[i]", followed
    // by either obj or null. Obj is pushed if the method is unbound (i.e. a normal method),
    // null is pushed if the function does not take a "this" parameter. Use with CallMethod.
    LoadMethod,

    // (n: u32). Calls a method loaded via LoadMethod with n positional arguments, just like Call.
    // The "this" parameter, if needed, was provided by LoadMethod (i.e. there are n + 1 real arguments).
    CallMethod,

    AssertFail, // pop expr_str, message and aborts (or throws...)

    // TODO: varargs call, keyword arguments, tail calls.

    // This value must always be kept in sync with the highest opcode value.
    // All values between "Invalid" (exlusive) and this one (inclusive) are treated
    // as valid.
    LastOpcode = AssertFail,
};

/// Returns the string representation of the given opcode.
std::string_view to_string(Opcode op);

/// Returns true if the given byte represents a valid opcode.
bool valid_opcode(u8 op);

/// Disassembles the given sequence of encoded instructions, for debugging.
std::string disassemble_instructions(Span<const byte> code);

} // namespace tiro::compiler

#endif // TIRO_COMPILER_OPCODES_HPP
