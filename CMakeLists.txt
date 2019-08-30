cmake_minimum_required(VERSION 3.13)

set(MASTER_PROJECT OFF)
if(CMAKE_CURRENT_SOURCE_DIR STREQUAL CMAKE_SOURCE_DIR)
    set(MASTER_PROJECT ON)
    message(STATUS "This is the master project, CMake version: ${CMAKE_VERSION}")
endif()

project(hammer VERSION 0.1.0)

# These options should be enabled during development!
option(HAMMER_WARNINGS "Build with pedantic warnings." OFF)
option(HAMMER_WERROR "Enable -Werror (halt compilation on warnings)." OFF)
option(HAMMER_TESTS "Build unit test target." ${MASTER_PROJECT})

option(HAMMER_LTO "Enable link time optimization (if supported)." OFF)

set(CMAKE_POSITION_INDEPENDENT_CODE ON)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake")
include(HammerFunctions)

#
# Compiler warnings and compiler specific flags
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
    set(FLAGS_PLATFORM -fno-rtti -fvisibility=hidden)
    set(FLAGS_LTO -flto -fno-fat-lto-objects)
endif()

#
# Enable warnings based on user settings and defaults
#
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

add_subdirectory(deps)
add_subdirectory(lib)
add_subdirectory(cmd)

if(HAMMER_TESTS)
    add_subdirectory(test)
endif()