#ifndef TIRO_BYTECODE_OP_HPP
#define TIRO_BYTECODE_OP_HPP

#include "tiro/bytecode/fwd.hpp"
#include "tiro/core/defs.hpp"
#include "tiro/core/format.hpp"
#include "tiro/core/id_type.hpp"

namespace tiro {

TIRO_DEFINE_ID(BytecodeRegister, u32)
TIRO_DEFINE_ID(BytecodeParam, u32)
TIRO_DEFINE_ID(BytecodeMemberId, u32)
TIRO_DEFINE_ID(BytecodeOffset, u32)

/* [[[cog
    from codegen.unions import define
    from codegen.bytecode import BytecodeOp
    define(BytecodeOp)
]]] */
/// Represents the type of an instruction.
enum class BytecodeOp : u8 {
    /// Load null into the target.
    ///
    /// Arguments:
    ///   - target (local, u32)
    LoadNull = 1,

    /// Load false into the target.
    ///
    /// Arguments:
    ///   - target (local, u32)
    LoadFalse,

    /// Load true into the target.
    ///
    /// Arguments:
    ///   - target (local, u32)
    LoadTrue,

    /// Load the given integer constant into the target.
    ///
    /// Arguments:
    ///   - constant (constant, i64)
    ///   - target (local, u32)
    LoadInt,

    /// Load the given floating point constant into the target.
    ///
    /// Arguments:
    ///   - constant (constant, f64)
    ///   - target (local, u32)
    LoadFloat,

    /// Load the given parameter into the target.
    ///
    /// Arguments:
    ///   - source (param, u32)
    ///   - target (local, u32)
    LoadParam,

    /// Store the given local into the the parameter.
    ///
    /// Arguments:
    ///   - source (local, u32)
    ///   - target (param, u32)
    StoreParam,

    /// Load the module variable source into target.
    ///
    /// Arguments:
    ///   - source (module, u32)
    ///   - target (local, u32)
    LoadModule,

    /// Store the source local into the target module variable.
    ///
    /// Arguments:
    ///   - source (local, u32)
    ///   - target (module, u32)
    StoreModule,

    /// Load `object.name` into target.
    ///
    /// Arguments:
    ///   - object (local, u32)
    ///   - name (module, u32)
    ///   - target (local, u32)
    LoadMember,

    /// Store source into `object.name`.
    ///
    /// Arguments:
    ///   - source (local, u32)
    ///   - object (local, u32)
    ///   - name (module, u32)
    StoreMember,

    /// Load `tuple.index` into target.
    ///
    /// Arguments:
    ///   - tuple (local, u32)
    ///   - index (constant, u32)
    ///   - target (local, u32)
    LoadTupleMember,

    /// Store source into `tuple.index`.
    ///
    /// Arguments:
    ///   - source (local, u32)
    ///   - tuple (local, u32)
    ///   - index (constant, u32)
    StoreTupleMember,

    /// Load `array[index]` into target.
    ///
    /// Arguments:
    ///   - array (local, u32)
    ///   - index (local, u32)
    ///   - target (local, u32)
    LoadIndex,

    /// Store source into `array[index]`.
    ///
    /// Arguments:
    ///   - source (local, u32)
    ///   - array (local, u32)
    ///   - index (local, u32)
    StoreIndex,

    /// Load the function's closure environment into the target.
    ///
    /// Arguments:
    ///   - target (local, u32)
    LoadClosure,

    /// Load a value from a closure environment. `level` is the number parent links to follow
    /// to reach the desired target environment (0 is `env` itself). `index` is the index of the value
    /// in the target environment.
    ///
    /// Arguments:
    ///   - env (local, u32)
    ///   - level (constant, u32)
    ///   - index (constant, u32)
    ///   - target (local, u32)
    LoadEnv,

    /// Store a value into a closure environment. Analog to LoadEnv.
    ///
    /// Arguments:
    ///   - source (local, u32)
    ///   - env (local, u32)
    ///   - level (constant, u32)
    ///   - index (constant, u32)
    StoreEnv,

    /// Store lhs + rhs into target.
    ///
    /// Arguments:
    ///   - lhs (local, u32)
    ///   - rhs (local, u32)
    ///   - target (local, u32)
    Add,

    /// Store lhs - rhs into target.
    ///
    /// Arguments:
    ///   - lhs (local, u32)
    ///   - rhs (local, u32)
    ///   - target (local, u32)
    Sub,

    /// Store lhs * rhs into target.
    ///
    /// Arguments:
    ///   - lhs (local, u32)
    ///   - rhs (local, u32)
    ///   - target (local, u32)
    Mul,

    /// Store lhs / rhs into target.
    ///
    /// Arguments:
    ///   - lhs (local, u32)
    ///   - rhs (local, u32)
    ///   - target (local, u32)
    Div,

    /// Store lhs % rhs into target.
    ///
    /// Arguments:
    ///   - lhs (local, u32)
    ///   - rhs (local, u32)
    ///   - target (local, u32)
    Mod,

    /// Store pow(lhs, rhs) into target.
    ///
    /// Arguments:
    ///   - lhs (local, u32)
    ///   - rhs (local, u32)
    ///   - target (local, u32)
    Pow,

    /// Store +value into target.
    ///
    /// Arguments:
    ///   - value (local, u32)
    ///   - target (local, u32)
    UAdd,

    /// Store -value into target.
    ///
    /// Arguments:
    ///   - value (local, u32)
    ///   - target (local, u32)
    UNeg,

    /// Store lhs << rhs into target.
    ///
    /// Arguments:
    ///   - lhs (local, u32)
    ///   - rhs (local, u32)
    ///   - target (local, u32)
    LSh,

    /// Store lhs >> rhs into target.
    ///
    /// Arguments:
    ///   - lhs (local, u32)
    ///   - rhs (local, u32)
    ///   - target (local, u32)
    RSh,

    /// Store lhs & rhs into target.
    ///
    /// Arguments:
    ///   - lhs (local, u32)
    ///   - rhs (local, u32)
    ///   - target (local, u32)
    BAnd,

    /// Store lhs | rhs into target.
    ///
    /// Arguments:
    ///   - lhs (local, u32)
    ///   - rhs (local, u32)
    ///   - target (local, u32)
    BOr,

    /// Store lhs ^ rhs into target.
    ///
    /// Arguments:
    ///   - lhs (local, u32)
    ///   - rhs (local, u32)
    ///   - target (local, u32)
    BXor,

    /// Store ~value into target.
    ///
    /// Arguments:
    ///   - value (local, u32)
    ///   - target (local, u32)
    BNot,

    /// Store lhs > rhs into target.
    ///
    /// Arguments:
    ///   - lhs (local, u32)
    ///   - rhs (local, u32)
    ///   - target (local, u32)
    Gt,

    /// Store lhs >= rhs into target.
    ///
    /// Arguments:
    ///   - lhs (local, u32)
    ///   - rhs (local, u32)
    ///   - target (local, u32)
    Gte,

    /// Store lhs < rhs into target.
    ///
    /// Arguments:
    ///   - lhs (local, u32)
    ///   - rhs (local, u32)
    ///   - target (local, u32)
    Lt,

    /// Store lhs <= rhs into target.
    ///
    /// Arguments:
    ///   - lhs (local, u32)
    ///   - rhs (local, u32)
    ///   - target (local, u32)
    Lte,

    /// Store lhs == rhs into target.
    ///
    /// Arguments:
    ///   - lhs (local, u32)
    ///   - rhs (local, u32)
    ///   - target (local, u32)
    Eq,

    /// Store lhs != rhs into target.
    ///
    /// Arguments:
    ///   - lhs (local, u32)
    ///   - rhs (local, u32)
    ///   - target (local, u32)
    NEq,

    /// Store !value into target.
    ///
    /// Arguments:
    ///   - value (local, u32)
    ///   - target (local, u32)
    LNot,

    /// Construct an array with the count topmost values
    /// from the stack and store it into target.
    ///
    /// Arguments:
    ///   - count (constant, u32)
    ///   - target (local, u32)
    Array,

    /// Construct a tuple with the count topmost values
    /// from the stack and store it into target.
    ///
    /// Arguments:
    ///   - count (constant, u32)
    ///   - target (local, u32)
    Tuple,

    /// Construct a set with the count topmost values
    /// from the stack and store it into target.
    ///
    /// Arguments:
    ///   - count (constant, u32)
    ///   - target (local, u32)
    Set,

    /// Construct a map with the count topmost keys and values
    /// from the stack and store it into target.
    /// The count must be even.
    /// Arguments at even indices become keys, arguments at odd indices become
    /// values of the new map.
    ///
    /// Arguments:
    ///   - count (constant, u32)
    ///   - target (local, u32)
    Map,

    /// Construct an environment with the given parent and size and
    /// store it into target.
    ///
    /// Arguments:
    ///   - parent (local, u32)
    ///   - size (constant, u32)
    ///   - target (local, u32)
    Env,

    /// Construct a closure with the given function template and environment and
    /// store it into target.
    ///
    /// Arguments:
    ///   - template (local, u32)
    ///   - env (local, u32)
    ///   - target (local, u32)
    Closure,

    /// Construct a new string formatter and store it into target.
    ///
    /// Arguments:
    ///   - target (local, u32)
    Formatter,

    /// Format a value and append it to the formatter.
    ///
    /// Arguments:
    ///   - value (local, u32)
    ///   - formatter (local, u32)
    AppendFormat,

    /// Store the formatted string into target.
    ///
    /// Arguments:
    ///   - formatter (local, u32)
    ///   - target (local, u32)
    FormatResult,

    /// Copy source to target.
    ///
    /// Arguments:
    ///   - source (local, u32)
    ///   - target (local, u32)
    Copy,

    /// Swap the values of the two locals.
    ///
    /// Arguments:
    ///   - a (local, u32)
    ///   - b (local, u32)
    Swap,

    /// Push value on the stack.
    ///
    /// Arguments:
    ///   - value (local, u32)
    Push,

    /// Pop the top (written by most recent push) from the stack.
    Pop,

    /// Pop the top (written by most recent push) from the stack and store it into target.
    ///
    /// Arguments:
    ///   - target (local, u32)
    PopTo,

    /// Unconditional jump to the given offset.
    ///
    /// Arguments:
    ///   - offset (offset, u32)
    Jmp,

    /// Jump to the given offset if the condition evaluates to true,
    /// otherwise continue with the next instruction.
    ///
    /// Arguments:
    ///   - condition (local, u32)
    ///   - offset (offset, u32)
    JmpTrue,

    /// Jump to the given offset if the condition evaluates to false,
    /// otherwise continue with the next instruction.
    ///
    /// Arguments:
    ///   - condition (local, u32)
    ///   - offset (offset, u32)
    JmpFalse,

    /// Jump to the given offset if the condition evaluates to null,
    /// otherwise continue with the next instruction.
    ///
    /// Arguments:
    ///   - condition (local, u32)
    ///   - offset (offset, u32)
    JmpNull,

    /// Jump to the given offset if the condition does not evaluate to null,
    /// otherwise continue with the next instruction.
    ///
    /// Arguments:
    ///   - condition (local, u32)
    ///   - offset (offset, u32)
    JmpNotNull,

    /// Call the given function the topmost count arguments on the stack.
    /// After the call, a single return value will be left on the stack.
    ///
    /// Arguments:
    ///   - function (local, u32)
    ///   - count (constant, u32)
    Call,

    /// Load the method called name from the given object.
    ///
    /// The appropriate this pointer (possibly null) will be stored into `this`.
    /// The method handle will be stored into `method`. The this pointer will be null
    /// for functions that do not accept a this parameter (e.g. bound methods, function
    /// attributes).
    ///
    /// This instruction is designed to be used in combination with CallMethod.
    ///
    /// Arguments:
    ///   - object (local, u32)
    ///   - name (module, u32)
    ///   - this (local, u32)
    ///   - method (local, u32)
    LoadMethod,

    /// Call the given method on an object with `count` additional arguments on the stack.
    /// The caller must push the `this` value received by LoadMethod followed by `count` arguments (for
    /// a total of `count + 1` push instructions).
    ///
    /// The arguments `this` and `method` must be the results
    /// of a previously executed LoadMethod instruction.
    ///
    /// After the call, a single return value will be left on the stack.
    ///
    /// Arguments:
    ///   - method (local, u32)
    ///   - count (constant, u32)
    CallMethod,

    /// Returns the value to the calling function.
    ///
    /// Arguments:
    ///   - value (local, u32)
    Return,

    /// Signals an assertion error and aborts the pogram.
    /// `expr` should contain the string representation of the failed assertion.
    /// `message` can hold a user defined error message string or null.
    ///
    /// Arguments:
    ///   - expr (local, u32)
    ///   - message (local, u32)
    AssertFail,
};

std::string_view to_string(BytecodeOp type);
// [[[end]]]

/// Returns true if the given value is in the range of valid opcode values.
bool valid_opcode(u8 raw_op);

/// Returns true if instructions with that opcode can reference a jump target by offset.
bool references_offset(BytecodeOp op);

/// Returns true if instructions with that opcode reference module members.
bool references_module(BytecodeOp op);

} // namespace tiro

TIRO_ENABLE_FREE_TO_STRING(tiro::BytecodeOp)

#endif // TIRO_BYTECODE_OP_HPP
