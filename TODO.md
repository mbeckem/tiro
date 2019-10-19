TODO LIST
=========

- Important: Closures must be local variables (first class context) instead of push/pop logic

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
