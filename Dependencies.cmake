include(FetchContent)

FetchContent_Declare(
    abseil_cpp
    GIT_REPOSITORY  https://github.com/abseil/abseil-cpp.git
    GIT_TAG         38db52adb2eabc0969195b33b30763e0a1285ef9
    GIT_PROGRESS    TRUE
    #GIT_SHALLOW     TRUE # (does not work for specific commit hashes on github)
)

FetchContent_Declare(
    catch2
    GIT_REPOSITORY  https://github.com/catchorg/Catch2.git
    GIT_TAG         v2.13.0
    GIT_PROGRESS    TRUE
    GIT_SHALLOW     TRUE
)

FetchContent_Declare(
    fmt
    GIT_REPOSITORY  https://github.com/fmtlib/fmt.git
    GIT_TAG         7.0.1
    GIT_PROGRESS    TRUE
    GIT_SHALLOW     TRUE
)

FetchContent_Declare(
    nlohmann_json
    GIT_REPOSITORY  https://github.com/nlohmann/json.git
    GIT_TAG         v3.8.0
    GIT_PROGRESS    TRUE
    GIT_SHALLOW     TRUE
)

FetchContent_Declare(
    utfcpp
    GIT_REPOSITORY  https://github.com/nemtrif/utfcpp.git
    GIT_TAG         v3.1.1
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

FetchContent_GetProperties(abseil_cpp)
if(NOT abseil_cpp_POPULATED)
    message(STATUS "### Dependency: abseil_cpp")
    FetchContent_Populate(abseil_cpp)
    add_subdirectory(${abseil_cpp_SOURCE_DIR} ${abseil_cpp_BINARY_DIR} EXCLUDE_FROM_ALL)
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
    add_subdirectory(${fmt_SOURCE_DIR} ${fmt_BINARY_DIR} EXCLUDE_FROM_ALL)
endif()

FetchContent_GetProperties(nlohmann_json)
if(NOT nlohmann_json_POPULATED)
    message(STATUS "### Dependency: nlohmann_json")
    FetchContent_Populate(nlohmann_json)
    set(JSON_BuildTests OFF CACHE BOOL "--Override --" FORCE)
    add_subdirectory(${nlohmann_json_SOURCE_DIR} ${nlohmann_json_BINARY_DIR} EXCLUDE_FROM_ALL)
endif()

FetchContent_GetProperties(utfcpp)
if(NOT utfcpp_POPULATED)
    message(STATUS "### Dependency: utfcpp")
    FetchContent_Populate(utfcpp)
    set(UTF8_TESTS OFF CACHE BOOL "-- Override --" FORCE)
    set(UTF8_INSTALL OFF CACHE BOOL "-- Override --" FORCE)
    set(UTF8_SAMPLES OFF CACHE BOOL "-- Override --" FORCE)
    add_subdirectory(${utfcpp_SOURCE_DIR} ${utfcpp_BINARY_DIR} EXCLUDE_FROM_ALL)
endif()
