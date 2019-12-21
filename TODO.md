TODO LIST
=========

- Get rid of useless consts in vm/objects

- Nullable/Nonnullable values and handles

- Forbid casts from Null to Object types (default constructors)

- Small integers in instructions only, large into constants at module level

- Member expression for tuple literals (as value and assignment target),
  e.g. t.1 = "value" (Discuss: t[0] vs t.0 syntax)

- Tuple unpacking, i.e. var (a, b, c) = tuple

- Describe formal grammar

- Include syntax like a < b < c or a == b == c?

- MUST NOT cache the internal data pointers because the gc will move objects in the future

- Analyzer: variables must not be used until they have been initialized in the current code path

Far future
==========

- Cache closure contexts in local variables instead of chasing parent->parent.
  Would require first class contexts (for bytecode, not for user code).

- Better codegen with SSA and control flow graph

- Register machine instead of stack based vm

- Dynamic member lookup for symbols? For example "a.#var" where var is a symbol variable
