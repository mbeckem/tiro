cmake_minimum_required(VERSION 3.13)
cmake_policy(VERSION 3.13)

set(MASTER_PROJECT OFF)
if(CMAKE_CURRENT_SOURCE_DIR STREQUAL CMAKE_SOURCE_DIR)
    set(MASTER_PROJECT ON)
    message(STATUS "This is the master project, CMake version: ${CMAKE_VERSION}")
endif()

project(hammer VERSION 0.1.0 LANGUAGES CXX C)

option(HAMMER_LTO "Enable link time optimization (if supported)." OFF)
option(HAMMER_COV "Code coverage analysis. Build mode should be Debug." OFF)
option(HAMMER_TESTS "Build unit test target." ${MASTER_PROJECT})

# These options should only be enabled during development!
option(HAMMER_WARNINGS "Build with pedantic warnings." OFF)
option(HAMMER_WERROR "Enable -Werror (halt compilation on warnings)." OFF)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set(CMAKE_CXX_VISIBILITY_PRESET hidden)
set(CMAKE_VISIBILITY_INLINES_HIDDEN 1)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake")
include(HammerFunctions)

#
# Compiler warnings and compiler specific flags
#
# TODO MSCV
#
if((CMAKE_CXX_COMPILER_ID MATCHES "GNU") OR (CMAKE_CXX_COMPILER_ID MATCHES "Clang"))
    set(FLAGS_PEDANTIC_WARNINGS
       # Disabled because of flexible array members :/
       # -pedantic
       # -pedantic-errors
        -Wall
        -Wextra

        -Wshadow
        -Wnon-virtual-dtor
        -Wcast-align
        -Wunused
        -Wsign-conversion
        -Wmisleading-indentation
        -Wduplicated-cond
        -Wduplicated-branches
        -Wlogical-op
        -Wnull-dereference
        -Wdouble-promotion
        -Wformat=2
    #    -Wlifetime # Recent clang only
    )
    set(FLAGS_DISABLED_WARNINGS
        -Wno-unknown-warning-option
        -Wno-noexcept-type
        -Wno-terminate
        -Wno-exceptions
    )
    set(FLAGS_WARNINGS_ARE_ERRORS -Werror)
    set(FLAGS_CPP_STANDARD -std=gnu++17) # Need extensions right now for flexible array members
    set(FLAGS_PLATFORM -fno-rtti)
    set(FLAGS_LTO -flto -fno-fat-lto-objects)
endif()

# Enable warnings based on user settings and defaults.
set(FLAGS_WARNINGS)
if(HAMMER_WARNINGS)
    list(APPEND FLAGS_WARNINGS ${FLAGS_PEDANTIC_WARNINGS})
endif()
if(HAMMER_WERROR)
    list(APPEND FLAGS_WARNINGS ${FLAGS_WARNINGS_ARE_ERRORS})
endif()
list(APPEND FLAGS_WARNINGS ${FLAGS_DISABLED_WARNINGS})

# These compiler flags are used by all .cpp files
set(GLOBAL_COMPILE_FLAGS
    ${FLAGS_WARNINGS}
    ${FLAGS_CPP_STANDARD}
    ${FLAGS_PLATFORM}
)
if(HAMMER_LTO)
    list(APPEND GLOBAL_COMPILE_FLAGS ${FLAGS_LTO})
endif()

if(HAMMER_COV)
    if (CMAKE_CXX_COMPILER_ID MATCHES "GNU")
        include(CodeCoverage)

        set(COVERAGE_LCOV_EXCLUDES '${PROJECT_SOURCE_DIR}/deps/*' '${PROJECT_SOURCE_DIR}/test/*' '/usr/*')
        message("Exclude: ${COVERAGE_LCOV_EXCLUDES}")

        APPEND_COVERAGE_COMPILER_FLAGS()
        SETUP_TARGET_FOR_COVERAGE_LCOV(
            NAME unit_tests_coverage
            EXECUTABLE unit_tests
            DEPENDENCIES unit_tests
        )
    else()
        message(ERROR "Unsupported compiler for code coverage analysis.")
    endif()
endif()

# The dependencies file is included rather than added via add_subdirectory because
# otherwise, imported targets would not be visible in the other subdirectories.
include(deps/Dependencies.cmake)

# Output all targets to well known directories.
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

add_subdirectory(src)
if(HAMMER_TESTS)
    add_subdirectory(test)
endif()
