#ifndef TIRO_CODEGEN_FIXUP_JUMPS_HPP
#define TIRO_CODEGEN_FIXUP_JUMPS_HPP

#include "tiro/core/defs.hpp"
#include "tiro/core/not_null.hpp"

namespace tiro {

class BasicBlock;
class InstructionStorage;

/// Fixes jump transitions between basic blocks by inserting Pop[N] instructions
/// at the appropriate places.
///
/// This is necessary because break and continue expressions (and possibly other jumps later on)
/// can be used in nested expressions and can therefore have "too many" values on their stack
/// for the target block. These values must be cleaned up in order for the code to be correct.
///
/// Consider the following (silly) example function:
///
///     func test() {
///         const foo = 1 + {
///             while (1) {
///                 var x = 99 + (3 + break);
///             }
///             2;
///         };
///         foo;
///     }
///
/// Without this algorithm, the (incorrect) function will return 5 (the 3 on the stack before the break,
/// the 2 after the while loop). The correct result is 3 (the leading 1 and the 2 after the loop).
/// The 99 and 3 must be removed from the stack with the execution of the "break".
void fixup_jumps(InstructionStorage& storage, NotNull<BasicBlock*> start);

} // namespace tiro

#endif // TIRO_CODEGEN_FIXUP_JUMPS_HPP
