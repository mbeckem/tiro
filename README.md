# Tiro

## Introduction

Tiro is a young dynamic programming language with a runtime designed to be easily embeddable into other applications.  
This project is still under active development.

### Design Goals and Rationale

- Teachablity. Tiro should have syntax and semantics that are easily understood by programmers already experience
  in a language from the C/C++/Java-family.  
  Inexperienced programmers should be able to learn this language quickly.   
  There should be as little confusing special cases or "footguns" as possible.
- Expressiveness. The language should not require excessive amounts of boilerplate code.
- Readability. Code written in Tiro should not devolve into an unreadable mess.
- Variables have dynamic types for quick prototyping.
- Support for more rigid constructs like classes and modules to structure larger code bases.
- Async execution model by default (with cheap coroutines / lightweight threads) to support common scripting tasks
  in domains like network services or games.
- Acceptable performance at runtime. The implementation should not be slower (and ideally faster) than that of comparable languages.
- The option to implement components in native code gives developers the ability to interface with the system,  
  to reuse existing native libraries and to implement high performance parts of their application in a compiled language.
- Easyly embeddable and extensible runtime library with a simple C-API and a small size footprint to support as many platforms as possible.  
  The runtime should be a "good citizen" in its embedding context: global state is forbidden and runtime instances are isolated from each other.

### Non-Goals
- Maximum performance is not a requirement.
- Highly experimental or unproven language constructs will not be implemented.  
  This is not a research project - the language's features are rather conservative.


## Getting started

1. Prerequisites:
    - A C++17 compiler
    - CMake >= 3.13
    - Boost library headers, version 1.65 or newer (Boost deps are on their way out :))

2. From inside the project directory, run:

        mkdir build && cd build        
        cmake .. -DTIRO_TESTS=1     # -DCMAKE_BUILD_TYPE=Debug -DTIRO_WARNINGS -DTIRO_WERROR for development
        make                          # Use -jN for an appropriate value of N for parallel builds
        ./test/unit_tests             # Run tests (optional)

## Dependencies

Included with the project:

* fmtlib (deps/fmt-$VERSION)  
  Website:        <http://fmtlib.net/latest/index.html>  
  Documentation:  <http://fmtlib.net/latest/api.html>

* Utfcpp (deps/utfcpp-$VERSION)
  Website:        <https://github.com/nemtrif/utfcpp>

* Catch2 (deps/catch.hpp)  
  Website:        <https://github.com/catchorg/Catch2>  
  Documentation:  <https://github.com/catchorg/Catch2/blob/master/docs/Readme.md>

System dependencies:

* Boost  
  Website:        <https://www.boost.org>

