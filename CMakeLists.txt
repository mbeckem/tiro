cmake_minimum_required(VERSION 3.13)
cmake_policy(VERSION 3.13)

set(MASTER_PROJECT OFF)
if(CMAKE_CURRENT_SOURCE_DIR STREQUAL CMAKE_SOURCE_DIR)
    set(MASTER_PROJECT ON)
    message(STATUS "This is the master project, CMake version: ${CMAKE_VERSION}")
endif()

project(tiro VERSION 0.1.0 LANGUAGES CXX C)

option(TIRO_LTO "Enable link time optimization (if supported)." OFF)
option(TIRO_COV "Code coverage analysis. Build mode should be Debug." OFF)
option(TIRO_TESTS "Build unit test target." ${MASTER_PROJECT})

# These options should only be enabled during development!
option(TIRO_WARNINGS "Build with pedantic warnings." OFF)
option(TIRO_WERROR "Enable -Werror (halt compilation on warnings)." OFF)

message(STATUS "TIRO_TESTS=${TIRO_TESTS}")
message(STATUS "TIRO_WARNINGS=${TIRO_WARNINGS}")
message(STATUS "TIRO_WERROR=${TIRO_WERROR}")
message(STATUS "TIRO_LTO=${TIRO_LTO}")
message(STATUS "TIRO_COV=${TIRO_COV}")

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set(CMAKE_CXX_VISIBILITY_PRESET hidden)
set(CMAKE_VISIBILITY_INLINES_HIDDEN 1)

if (TIRO_LTO)
    set(CMAKE_INTERPROCEDURAL_OPTIMIZATION TRUE)
endif()

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake")
include(TiroFunctions)

set(DEFAULT_BUILD_TYPE "Release")
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    message(STATUS "Setting build type to '${DEFAULT_BUILD_TYPE}' as none was specified.")
    set(CMAKE_BUILD_TYPE "${DEFAULT_BUILD_TYPE}" CACHE
        STRING "Choose the type of build." FORCE)
    set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS
        "Debug" "Release" "MinSizeRel" "RelWithDebInfo")
endif()

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
endif()

# Enable warnings based on user settings and defaults.
set(FLAGS_WARNINGS)
if(TIRO_WARNINGS)
    list(APPEND FLAGS_WARNINGS ${FLAGS_PEDANTIC_WARNINGS})
endif()
if(TIRO_WERROR)
    list(APPEND FLAGS_WARNINGS ${FLAGS_WARNINGS_ARE_ERRORS})
endif()
list(APPEND FLAGS_WARNINGS ${FLAGS_DISABLED_WARNINGS})

# These compiler flags are used by all .cpp files
set(GLOBAL_COMPILE_FLAGS
    ${FLAGS_WARNINGS}
    ${FLAGS_CPP_STANDARD}
    ${FLAGS_PLATFORM}
)

if(TIRO_COV)
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

#find_program(
#    CLANG_TIDY_EXE
#    NAMES "clang-tidy-11" "clang-tidy-10" "clang-tidy-9" "clang-tidy-8" "clang-tidy"
#    DOC "Path to clang-tidy executable"
#)
#if(NOT CLANG_TIDY_EXE)
#    message(STATUS "clang-tidy not found.")
#else()
#    message(STATUS "clang-tidy found: ${CLANG_TIDY_EXE}")
#    set(CMAKE_CXX_CLANG_TIDY "${CLANG_TIDY_EXE}")
#endif()

# Output all targets to well known directories.
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

add_subdirectory(src)
if(TIRO_TESTS)
    add_subdirectory(test)
endif()
