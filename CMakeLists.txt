cmake_minimum_required(VERSION 3.13)
cmake_policy(VERSION 3.13)

set(MASTER_PROJECT OFF)
if(CMAKE_CURRENT_SOURCE_DIR STREQUAL CMAKE_SOURCE_DIR)
    set(MASTER_PROJECT ON)
    message(STATUS "This is the master project, CMake version: ${CMAKE_VERSION}")
endif()

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/support/cmake")
include(TiroFunctions)
include(CMakePackageConfigHelpers)

project(tiro LANGUAGES CXX C)
tiro_apply_version(0 1 0 "-dev")

include(GNUInstallDirs)

option(TIRO_DEV "Enable common development settings." OFF)
option(TIRO_LTO "Enable link time optimization (if supported)." OFF)
option(TIRO_COV "Code coverage analysis. Build mode should be Debug." OFF)
option(TIRO_IWYU "Runs include-what-you-use when building the library" OFF)
option(TIRO_SAN "Enable compiling with sanitizer flags (GCC only)." OFF)
option(TIRO_TESTS "Build unit test target." ${TIRO_DEV})
option(TIRO_EXAMPLES "Build example projects." ${TIRO_DEV})
option(TIRO_DOCS "Enable documentation build target (use target doc to build)." OFF)
option(TIRO_RTTI "Enable or disable building with runtime type information." OFF)

# These options should only be enabled during development!
option(TIRO_WARNINGS "Build with pedantic warnings." ${TIRO_DEV})
option(TIRO_WERROR "Enable -Werror (halt compilation on warnings)." ${TIRO_DEV})
option(TIRO_COLORS "Always produce ANSI-colored output (GNU/Clang only)." ${TIRO_DEV})

# For emscripten build ..
option(TIRO_SKIP_THREADS "Build without looking for a threading library." OFF)
option(TIRO_SKIP_INSTALL "Build without installation targets" OFF)

message(STATUS "TIRO_DEV=${TIRO_DEV}")
message(STATUS "TIRO_TESTS=${TIRO_TESTS}")
message(STATUS "TIRO_EXAMPLES=${TIRO_EXAMPLES}")
message(STATUS "TIRO_WARNINGS=${TIRO_WARNINGS}")
message(STATUS "TIRO_WERROR=${TIRO_WERROR}")
message(STATUS "TIRO_SAN=${TIRO_SAN}")
message(STATUS "TIRO_LTO=${TIRO_LTO}")
message(STATUS "TIRO_COV=${TIRO_COV}")
message(STATUS "TIRO_IWYU=${TIRO_IWYU}")
message(STATUS "TIRO_SKIP_THREADS=${TIRO_SKIP_THREADS}")

set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set(CMAKE_C_VISIBILITY_PRESET hidden)
set(CMAKE_CXX_VISIBILITY_PRESET hidden)
set(CMAKE_VISIBILITY_INLINES_HIDDEN 1)
set(CMAKE_C_STANDARD 90)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_EXTENSIONS ON) # Needed for flexible array members in the vm

set(DEFAULT_BUILD_TYPE "Release")
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    message(STATUS "Setting build type to '${DEFAULT_BUILD_TYPE}' as none was specified.")
    set(CMAKE_BUILD_TYPE "${DEFAULT_BUILD_TYPE}" CACHE
        STRING "Choose the type of build." FORCE)
    set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS
        "Debug" "Release" "MinSizeRel" "RelWithDebInfo")
endif()

if (TIRO_IWYU)
    find_program(IWYU_PATH NAMES include-what-you-use iwyu REQUIRED)
endif()

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

if (TIRO_SAN)
    if (CMAKE_CXX_COMPILER_ID MATCHES "GNU")
        add_compile_options("-fsanitize=address" "-fsanitize=undefined")
        add_link_options("-fsanitize=address" "-fsanitize=undefined")
    else()
        message(ERROR "Unsupported compiler for sanitizers.")
    endif()
endif()

if (TIRO_COLORS)
    if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
       add_compile_options(-fdiagnostics-color=always)
    elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
       add_compile_options(-fcolor-diagnostics)
    endif ()
endif ()

# Common flags that impact code generation.
# It is important to set this globally so it gets picked up by dependencies as well, especially
# when using LTO.
if ((CMAKE_CXX_COMPILER_ID MATCHES "GNU") OR (CMAKE_CXX_COMPILER_ID MATCHES "Clang"))
    if (TIRO_LTO)
        add_compile_options(-flto -fno-fat-lto-objects)
        add_link_options(-flto=auto)
    endif()

    if ((CMAKE_CXX_COMPILER_ID MATCHES "Clang") AND WIN32)
        # TODO: There appears to be a msvc stdlib issue with rtti?
        # add_compile_definitions(_HAS_STATIC_RTTI=0 )
    elseif(NOT TIRO_RTTI)
        add_compile_options($<$<COMPILE_LANGUAGE:CXX>:-fno-rtti>)
    endif()
else()
    if (TIRO_LTO)
        set(CMAKE_INTERPROCEDURAL_OPTIMIZATION TRUE)
    endif()
endif()

if (WIN32)
    add_compile_definitions(_SILENCE_ALL_CXX17_DEPRECATION_WARNINGS=1)
endif()

# The dependencies file is included rather than added via add_subdirectory because
# otherwise, imported targets would not be visible in the other subdirectories.
message(STATUS "### BEGIN DEPENDENCIES ###")
include(./Dependencies.cmake)
message(STATUS "### END DEPENDENCIES ###")

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
if (MASTER_PROJECT)
    set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
    set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
endif()

add_subdirectory(src)
if(TIRO_TESTS)
    add_subdirectory(test)
endif()
if (TIRO_EXAMPLES)
    add_subdirectory(examples/embed_cpp)
endif()
if (TIRO_DOCS)
    add_subdirectory(docs/api)
endif()
