TODO LIST
=========

- IMPORTANT: Finish allocation of local variables (~ register allocation). Currently, every local is assigned
  its own slot, which is insanely wasteful for the common case where local lifetimes do not overlap.

- Compiler:
  - Operator ?. (Null chaining)
  - Operator ?? or similar (Null coalescence)
  - Maybe: operator ?.
  - Maybe: a chaining operator to pipe function results into each other (Syntax? "->" or "|>" or something else entirely)?

- Compiler: Chained comparisons (like in python: https://docs.python.org/3/reference/expressions.html#comparisons)?  
            All comparisons have the same precedence and can be chained together:
            A op B op C is the same as (A op B) && (B op C) with the exception that every expr is only evaluated once.

- VM: Get rid of useless consts in vm/objects

- VM: Nullable/Nonnullable values and handles

- VM: Forbid casts from Null to Object types (default constructors)

- VM/Compiler: Small integers in instructions only, large into constants at module level

- Compiler: Describe formal grammar

- VM: Better garbage collector, see `design/gc_design.md`

- VM: MUST NOT cache the internal data pointers because the gc will move objects in the future

- Compiler: Analyzer: variables must not be used until they have been initialized in the current code path
            There must be tests for this, it should already be implemented like this.

- Compiler: Investigate non-standard container libraries to reduce binay size

- Compiler: The ast should represent syntax elements only - use different classes for semantic information.

- Compiler: Should a function automatically return its last value when no explicit return is given? Like in normal blocks.


Far future
==========

- Compiler: Better codegen with SSA and control flow graph

- Compiler: Make functions (in mir translation phase) compile and optimize separately. This would enable parallel compilation.
  The function translation would not be able to reference the module's state and the parent function's closure environments (if any).
  Instead, it would simply export required module variables (as node IDs) and required upvalues (also as node IDs). These
  would have to be patched in later once the dependencies have been compiled as well.

- Compiler/VM: Register machine instead of stack based vm

- Compiler/VM: Dynamic member lookup for symbols? For example "a.#var" where var is a symbol variable
