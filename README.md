# Tiro [![Build](https://github.com/mbeckem/tiro/workflows/Test%20Build/badge.svg)](https://github.com/mbeckem/tiro/actions?query=workflow%3A%22Test+Build%22)

## Introduction

Tiro is a young dynamic programming language with a runtime designed to be easily embeddable into other applications.
This project is still under active development and far from being complete!

A snippet of tiro looks like this:

```
import std;

func main() {
    const object = "World";
    std.print("Hello ${object}!");
}
```

Or like this:

```
func fibonacci(i) {
    if (i <= 1) {
        return i;
    }

    var a = 0;
    var b = 1;
    while (i >= 2) {
        (a, b) = (b, a + b);
        i -= 1;
    }
    return b;
}
```

Or like this:

```
import std;

func fizzbuzz() {
    for (var i = 1; i <= 100; i += 1) {
        const message = if (i % 15 == 0) {
            "FizzBuzz";
        } else if (i % 3 == 0) {
            "Fizz";
        } else if (i % 5 == 0) {
            "Buzz";
        } else {
            "$i";
        };

        std.print(message);
    }
}
```

## Code examples

Have a look at the [examples directory](./examples) and the [unit tests](./test/unit_tests/eval).

## Quick start

1.  Prerequisites:

    -   A C++17 compiler
    -   CMake >= 3.13

2.  From inside the project directory, run:

        $ mkdir build && cd build
        $ cmake .. -DTIRO_TESTS=1     # -DCMAKE_BUILD_TYPE=Debug -DTIRO_WARNINGS -DTIRO_WERROR for development
        $ cmake --build . -j $(nproc) # Use -j N for parallel builds
        $ ./bin/unit_tests && ./bin/api_tests
        $ ./bin/tiro_run ../examples/fizzbuzz/fizzbuzz.tro --dump-ast --disassemble --invoke fizzbuzz

For more detailed build instructions, read [building.md](./docs/building.md).

## The Language

### Design Goals and Rationale

-   Teachablity. Tiro should have syntax and semantics that are easily understood by programmers already experience
    in a language from the C/C++/Java-family.  
    Inexperienced programmers should be able to learn this language quickly. There should be as little confusing special cases or "footguns" as possible.
-   Expressiveness. The language should not require excessive amounts of boilerplate code.
-   Readability. Code written in Tiro should not devolve into an unreadable mess.
-   Variables have dynamic types for quick prototyping.
-   Support for more rigid constructs like classes and modules to structure larger code bases.
-   Async execution model by default (with cheap coroutines / lightweight threads) to support common scripting tasks in  
    domains like network services or games.
-   Acceptable performance at runtime. The implementation should not be slower than that of comparable languages.
-   The option to implement components in native code gives developers the ability to interface with the system,  
    to reuse existing native libraries and to implement high performance parts of their application in a compiled language.
-   Easyly embeddable and extensible runtime library with a simple C-API and a small size footprint to support as many platforms as possible.  
    The runtime should be a "good citizen" in its embedding context: global state is forbidden and runtime instances are isolated from each other.

#### Non-Goals

-   Maximum performance is not a requirement.
-   Highly experimental or unproven language constructs will not be implemented.
    This is not a research project - the language's feature set is rather conservative.

### Comparison with similar languages

**TODO**

### Grammar

The grammar is a work in progress and is documented [here](./docs/grammar.md).

## Repository overview

<!-- tree -d --charset utf8 -L 1 -n --noreport -->

### Project repository

```
.
├── design              -- Design documents
├── examples            -- Code examples
├── include             -- Public C API
├── scratch             -- Test code (TODO: Remove)
├── src                 -- Implementation of cli tools, API, compiler and vm
├── test                -- Automated tests
└── utils               -- Utility files (like codegen scripts, unicode database, cmake support scripts)
```

### Source code folder overview

```
src
├── api                 -- Implementation of the public interface (i.e. include/tiro)
├── bytecode            -- Defines bytecode objects (output of the compiler, input of the VM)
├── common              -- Reuseable types, macros and functions used by the compiler or the vm
├── compiler            -- Implementation of the compiler
│   ├── ast             -- Defines all AST types and associated helpers (e.g. tree traversal)
│   ├── bytecode_gen    -- Transforms the internal representation into executable bytecode
│   ├── ir              -- Defines the internal representations. Contains analysis and optimization.
│   ├── ir_gen          -- Transforms the AST into the internal representation.
│   ├── parser          -- Lexing and parsing of tiro source code
│   └── semantics       -- Semantic analysis passes during compilation
├── run                 -- Implementation of cli tools (currently uses the internal interface)
└── vm                  -- Implementation of the virtual machine
    ├── handles         -- Handle types for referencing objects on the managed heap
    ├── heap            -- Garbage collected heap implementation
    ├── modules         -- Importable default modules (e.g. "import std")
    └── objects         -- VM Object types (e.g. Arrays, HashTable, Strings, ...)
```

## Dependencies

These dependencies are fetched during the build and will be included (statically linked) when tiro is built as a library.

-   Abseil  
    Website: <https://abseil.io/>  
    License: Apache-2.0 License

-   fmtlib  
    Website: <http://fmtlib.net/latest/index.html>  
    License: [Link](https://github.com/fmtlib/fmt/blob/master/LICENSE.rst)

-   Utfcpp  
    Website: <https://github.com/nemtrif/utfcpp>  
    License: Boost Software License 1.0

-   nlohmann/json  
    Website: <https://github.com/nlohmann/json>  
    License: MIT License

### Test Dependencies

Not included in the final executable library:

-   Catch2  
    Website: <https://github.com/catchorg/Catch2>  
    License: Boost Software License 1.0
