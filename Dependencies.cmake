include(FetchContent)

FetchContent_Declare(
    abseil_cpp
    GIT_REPOSITORY  https://github.com/abseil/abseil-cpp.git
    GIT_TAG         04bde89e5cb33bf4a714a5496fac715481fc4831
    GIT_PROGRESS    TRUE
    #GIT_SHALLOW     TRUE # (does not work for specific commit hashes on github)
)

FetchContent_Declare(
    catch2
    GIT_REPOSITORY  https://github.com/catchorg/Catch2.git
    GIT_TAG         v2.13.9
    GIT_PROGRESS    TRUE
    GIT_SHALLOW     TRUE
)

FetchContent_Declare(
    fmt
    GIT_REPOSITORY  https://github.com/fmtlib/fmt.git
    GIT_TAG         8.0.1
    GIT_PROGRESS    TRUE
    GIT_SHALLOW     TRUE
)

FetchContent_Declare(
    nlohmann_json
    GIT_REPOSITORY  https://github.com/nlohmann/json.git
    GIT_TAG         v3.9.1
    GIT_PROGRESS    TRUE
    GIT_SHALLOW     TRUE
)

FetchContent_Declare(
    utfcpp
    GIT_REPOSITORY  https://github.com/nemtrif/utfcpp.git
    GIT_TAG         v3.2
    GIT_PROGRESS    TRUE
    GIT_SHALLOW     TRUE
)

FetchContent_Declare(
    cxxopts
    GIT_REPOSITORY  https://github.com/jarro2783/cxxopts.git
    GIT_TAG         v2.2.1
    GIT_PROGRESS    TRUE
    GIT_SHALLOW     TRUE
)

if (NOT TIRO_SKIP_THREADS)
    set(CMAKE_THREAD_PREFER_PTHREAD TRUE)
    set(THREADS_PREFER_PTHREAD_FLAG TRUE)
    find_package(Threads REQUIRED)
endif()

# set(FETCHCONTENT_QUIET FALSE)
# set(FETCHCONTENT_FULLY_DISCONNECTED TRUE)
# set(FETCHCONTENT_UPDATES_DISCONNECTED TRUE)

# When building as a static library, we must enable the install targets
# of runtime dependencies. This means that the subdirectories must
# also be added without EXCLUDE_FROM_ALL, making builds slower.
set(INSTALL_DEPENDENCIES OFF)
if (NOT ${TIRO_BUILD_SHARED})
    set(INSTALL_DEPENDENCIES ON)
endif()

function(_add_dependency_subdir source_dir dest_dir)
    set(_mode "EXCLUDE_FROM_ALL")
    if (INSTALL_DEPENDENCIES)
        set(_mode "")
    endif()

    add_subdirectory("${source_dir}" "${dest_dir}" ${_mode})
endfunction()

FetchContent_GetProperties(abseil_cpp)
if(NOT abseil_cpp_POPULATED)
    message(STATUS "### Dependency: abseil_cpp")
    set(ABSL_PROPAGATE_CXX_STD ON CACHE BOOL "-- Override --" FORCE)
    set(ABSL_ENABLE_INSTALL ${INSTALL_DEPENDENCIES} CACHE BOOL "-- Override --" FORCE)
    FetchContent_Populate(abseil_cpp)
    _add_dependency_subdir(${abseil_cpp_SOURCE_DIR} ${abseil_cpp_BINARY_DIR})
endif()

FetchContent_GetProperties(catch2)
if(NOT catch2_POPULATED)
    message(STATUS "### Dependency: catch2")
    FetchContent_Populate(catch2)
    add_subdirectory(${catch2_SOURCE_DIR} ${catch2_BINARY_DIR} EXCLUDE_FROM_ALL)
endif()

FetchContent_GetProperties(fmt)
if(NOT fmt_POPULATED)
    message(STATUS "### Dependency: fmt")
    FetchContent_Populate(fmt)
    set(FMT_INSTALL ${INSTALL_DEPENDENCIES} CACHE BOOL "-- Override --" FORCE)
    _add_dependency_subdir(${fmt_SOURCE_DIR} ${fmt_BINARY_DIR})
endif()

FetchContent_GetProperties(nlohmann_json)
if(NOT nlohmann_json_POPULATED)
    message(STATUS "### Dependency: nlohmann_json")
    FetchContent_Populate(nlohmann_json)
    set(JSON_BuildTests OFF CACHE BOOL "--Override --" FORCE)
    _add_dependency_subdir(${nlohmann_json_SOURCE_DIR} ${nlohmann_json_BINARY_DIR})
endif()

FetchContent_GetProperties(utfcpp)
if(NOT utfcpp_POPULATED)
    message(STATUS "### Dependency: utfcpp")
    FetchContent_Populate(utfcpp)
    set(UTF8_TESTS OFF CACHE BOOL "-- Override --" FORCE)
    set(UTF8_INSTALL ${INSTALL_DEPENDENCIES} CACHE BOOL "-- Override --" FORCE)
    set(UTF8_SAMPLES OFF CACHE BOOL "-- Override --" FORCE)
    _add_dependency_subdir(${utfcpp_SOURCE_DIR} ${utfcpp_BINARY_DIR})
endif()

FetchContent_GetProperties(cxxopts)
if(NOT cxxopts_POPULATED)
    message(STATUS "### Dependency: cxxopts")
    set(CXXOPTS_BUILD_EXAMPLES OFF CACHE BOOL "-- Override --" FORCE)
    set(CXXOPTS_BUILD_TESTS OFF CACHE BOOL "-- Override --" FORCE)
    FetchContent_Populate(cxxopts)
    add_subdirectory(${cxxopts_SOURCE_DIR} ${cxxopts_BINARY_DIR} EXCLUDE_FROM_ALL)
endif()
