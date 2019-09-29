# Hammer

## Introduction

TODO

## Getting started

1. Prerequisites:
    - A C++17 compiler
    - CMake >= 3.13
    - Boost library headers, version 1.65 or newer (Boost deps are on their way out :))

2. From inside the project directory, run:

        mkdir build && cd build        
        cmake .. -DHAMMER_TESTS=1     # -DCMAKE_BUILD_TYPE=Debug -DHAMMER_WARNINGS -DHAMMER_WERROR for development
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

