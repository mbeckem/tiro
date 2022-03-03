# Building tiro

## Requirements

-   A C++17 compiler
-   CMake 3.13 or later

Tiro supports the following compilers:

-   gcc (tested on g++ 8.3)
-   clang (tested on clang++ 8.0)

Other recent compilers may work as well (C++17 is required at the very least).

## Build system

Tiro uses [CMake](https://cmake.org/) as its build system. CMake is a cross platform tool that can generate appropriate build files for a wide range of platforms (such as Unix Makefiles or Visual Studio Solutions). The general pattern when using CMake is:

    $ mkdir build && cd build   # Create a build directory for out-of-source builds
    $ cmake ..                  # Generate native build scripts
    $ make                      # Build the project

Specify the well known `CMAKE_BUILD_TYPE=<Debug|Release|MinSizeRel|RelWithDebInfo>` option to control optimization settings. The default is `Release`.
Use `-jN` for some sensible value of `N` when using make to use more than once processor, e.g. `make -j8` for 8 parallel build jobs.

After a successful build, executables can be found in `bin` directory. Libraries are placed in the `lib` directory.

The following additional options can be supplied to CMake at configuration time:

-   `TIRO_DEV=<ON|OFF>`  
     Build in development mode. This enables warnings and tests by default. It also disables the current build time in version.h in order to prevent needless recompilation.
-   `TIRO_TESTS=<ON|OFF>`  
     Build the unit test target. Defaults to `ON` if tiro is being built in development mode.
-   `TIRO_EXAMPLES=<ON|OFF>`  
     Build the examples projects. Defaults to `ON` if tiro is being built in development mode.
-   `TIRO_WARNINGS=<ON|OFF>`  
     Build with pedantic warnings enabled. Should be enabled during development mode. Defaults to `OFF` if Tiro is not being built in development mode.
-   `TIRO_WERROR=<ON|OFF>`  
     Makes any warnings fatal. Should be disabled for release builds. Defaults to `OFF` if Tiro is not being built in development mode.
-   `TIRO_SAN=<ON|OFF>`  
     Build with sanitizers enabled (e.g. `-fsanitize=undefined`). Only works on GNU compilers for now.
-   `TIRO_LTO=<ON|OFF>`  
     Enables link time optimization for compilers that support it. Results in smaller and more efficient binaries
    at the cost of slower builds. Should be enabled for optimized builds. Defaults to `OFF`.

## Examples

### Release build

    $ mkdir build && cd build
    $ cmake .. -DCMAKE_BUILD_TYPE=Release -DTIRO_TESTS=1 -DTIRO_WARNINGS=1 -DTIRO_LTO=1
    $ cmake --build . -j $(nproc)
    $ ./bin/unit_tests && ./bin/eval_tests && ./bin/api_tests

### Debug/development configuration

Use the following command (or similar) to configure your development setup:

    $ cmake .. -DCMAKE_BUILD_TYPE=Debug -DTIRO_DEV=1

## Using Docker

You can use Docker to build tiro, thus eliminated any setup costs. From the project root, run:

    $ docker build . -f utils/build-container/Dockerfile -t build-tiro
    $ docker run --rm -it build-tiro /bin/bash

The built project will be available in `/workspace/build/`. Note that you can provide the build options listed above using the `--build-arg` argument to `docker build`.
See the contents of `utils/build-container/Dockerfile` for details.
