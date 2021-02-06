# Internal Representation

The compiler's internal representation (IR) is based on Single Static Assignment (SSA) instructions contained in basic blocks.
Basic blocks are linked to each other, and a graph of basic blocks makes up a function's control flow graph (CFG).
A function is the unit of compilation in the middle end of the compiler, i.e. all functions are compiled independently from each other.

## Functions

The `Function` class contains metadata about a tiro function (name, type, parameters, etc.) and also owns the
control flow graph and all instructions that make up that function.
Entities within a function are referred to by their Id (e.g. `BlockId` or `InstId`) instead of raw pointers.

Ids are only valid within their function and must not be mixed with Ids from another function without context.
For example, two functions may use the `InstId` 1 to mean completely different things.

## Blocks

Blocks form the function's CFG.
They are linked through predecessor and successor links (represented as `BlockId`s).
Normal control flow only happens on the edge between two blocks, the `Terminator` (e.g. a branch).
Blocks can contain any number of instructions (class `Inst`).
If control enters a block, all its instructions are executed from start to finish (modulo exceptions, see below), in the order they are listed (basic block property).

There are two special blocks in every function:

-   The entry block has no predecessor and never contains any instructions.
    It is purely virtual to ensure that all "normal" blocks have a common root, which makes algorithms like
    the computation of dominator trees easier to implement.
    The entry block points to the first "normal" basic block in the function and to all "handler" blocks, which
    are used to implement exception handlers.

-   The exit block has no successors and also never contains any instructions.
    All basic blocks in the function's CFG have the exit block as their direct or indirect successor, making
    it the root of the reverse CFG.

## Instructions

Instructions model basic operations, e.g. additions, function calls or memory reads and writes.
Instructions may produce a value, which can be used as an operand of other instructions later on.
Instructions use single static assignment (SSA) form:

-   Every instruction is defined at most once. Instructions are defined by listing their Id in a basic block.
-   Every instruction is defined before its first use as an operand in another instruction.

## Exceptions

Basic blocks only model "normal" (i.e. non exceptional) control flow like loops or branches.
The IR does not model exceptions through normal edges between blocks because in tiro, almost every instruction may
result in an exception (for example, every addition may throw on integer overflow).
Using normal edges to model exceptions, like e.g. LLVM's IR does, would therefore result in most basic blocks only containing a single instruction,
making their existence as a concept almost useless.

Instead, tiro's IR follows a similar approach as Julia (https://docs.julialang.org/en/v1/devdocs/ssair/):

-   Every block has `handler` block, possibly empty.
    When an exception occurs while evaluating the instructions in that block, control jumps to the beginning of the exception `handler`.
    Thus, there is an implicit control flow edge from every instruction to the `handler`.

-   Handler blocks are additional entry points into the function which are called by the runtime when an exception occurs.
    Because many blocks may share the same handler, the handler can only make minimal assumptions about the state of the function.
    For example, it currently cannot refer to any SSA instructions from the "normal" control flow which could possibly be in scope.

-   If code inside a handler needs to read or write a variable, it uses a `ObserveAssign` instruction, which is similar to a Phi node.
    The compiler will ensure that every variable assignment also emits a `PublishAssign` instruction, which may then be captured by `ObserveAssign` instructions in one of the exception handlers.
    Unused `PublishAssign` instructions will be optimized out.
