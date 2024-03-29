# IDEAS

-   Compiler: Analyzer: variables must not be used until they have been initialized in the current code path
    There must be tests for this, it should already be implemented like this.

-   Compiler: Improve null safety in the AST. Use `NotNull<T>` for structurally required children and introduce `Error` node types
    as placeholders where no child could be parsed. Note that most children of nodes should be required (most are not right now), the
    incremental nature of the parsing process can be modeled with separate, stateful builder classes.

-   Compiler: Introduce a type similar to `NotNull<T>` for id types and other types that have an invalid state. This type would be
    similar to std::optional<T>, but it would the contained instance against invalid patterns in `T`.

-   VM: Generate object layouts and gc walk functions using cog.

-   VM: Modcount (or equivalent) for containers to prevent concurrent modifications.
    Alternative: define container behavior when modifying while iterating

-   VM: Iterator protocol (and framework for other protocols).

-   Compiler: Make functions (in mir translation phase) compile and optimize separately. This would enable parallel compilation.
    The function translation would not be able to reference the module's state and the parent function's closure environments (if any).
    Instead, it would simply export required module variables (as node IDs) and required upvalues (also as node IDs). These
    would have to be patched in later once the dependencies have been compiled as well.

-   Compiler:

    -   Maybe: operator ?
    -   Maybe: a chaining operator to pipe function results into each other (Syntax? "->" or "|>" or something else entirely)?
    -   Compiler/VM: Dynamic member lookup for symbols? For example "a.#var" where var is a symbol variable

-   VM/Compiler: Small integers in instructions only, large into constants at module level?

-   Use builder syntax for native function object creation
