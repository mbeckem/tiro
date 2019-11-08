TODO LIST
=========

- Get rid of useless consts in vm/objects

- Automated tests for code compilation / execution now that we can run it

- Forbid casts from Null to Object types (default constructors)

- Should remove noexcept in most circumstances as assertions may throw (?) [VM]

- Small integers in instructions only, large into constants at module level

- Member expression for tuple literals (as value and assignment target)

- Tuple unpacking, i.e. var (a, b, c) = tuple

- Describe formal grammar

- complete VM

- Dynamic member lookup for symbols? For example "a.#var" 

Far future
==========

- Cache closure contexts in local variables instead of chasing parent->parent.
  Would require first class contexts (for bytecode, not for user code).

- Better codegen with SSA and control flow graph

- Register machine instead of stack based vm
