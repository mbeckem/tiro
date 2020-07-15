# TODO: Use cmake external projects for this instead of submodules?

add_library(asio INTERFACE)
target_include_directories(asio SYSTEM INTERFACE "${CMAKE_CURRENT_LIST_DIR}/asio/asio/include")
target_compile_definitions(asio INTERFACE -DASIO_STANDALONE -DASIO_DISABLE_VISIBILITY -DASIO_NO_TYPEID)
if (WIN32)
    target_link_libraries(asio INTERFACE wsock32 ws2_32)
endif()

add_subdirectory("${CMAKE_CURRENT_LIST_DIR}/abseil-cpp" EXCLUDE_FROM_ALL)
add_subdirectory("${CMAKE_CURRENT_LIST_DIR}/catch" EXCLUDE_FROM_ALL)

add_subdirectory("${CMAKE_CURRENT_LIST_DIR}/fmt" EXCLUDE_FROM_ALL)

set(JSON_BuildTests OFF CACHE BOOL "--Override --" FORCE)
add_subdirectory("${CMAKE_CURRENT_LIST_DIR}/nlohmann_json" EXCLUDE_FROM_ALL)

set(UTF8_TESTS OFF CACHE BOOL "-- Override --" FORCE)
set(UTF8_INSTALL OFF CACHE BOOL "-- Override --" FORCE)
set(UTF8_SAMPLES OFF CACHE BOOL "-- Override --" FORCE)
add_subdirectory("${CMAKE_CURRENT_LIST_DIR}/utfcpp" EXCLUDE_FROM_ALL)

if (NOT TIRO_SKIP_THREADS)
    set(CMAKE_THREAD_PREFER_PTHREAD TRUE)
    set(THREADS_PREFER_PTHREAD_FLAG TRUE)
    find_package(Threads REQUIRED)
endif()
