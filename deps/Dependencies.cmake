add_subdirectory("${CMAKE_CURRENT_LIST_DIR}/fmt-5.3.0" EXCLUDE_FROM_ALL)

set(UTF8_TESTS OFF CACHE BOOL "-- Override --")
set(UTF8_INSTALL OFF CACHE BOOL "-- Override --")
set(UTF8_SAMPLES OFF CACHE BOOL "-- Override --")
add_subdirectory("${CMAKE_CURRENT_LIST_DIR}/utfcpp-3.1" EXCLUDE_FROM_ALL)

set(Boost_USE_STATIC_LIBS       ON)
set(Boost_USE_MULTITHREADED     ON)
set(Boost_USE_STATIC_RUNTIME    OFF)
find_package(Boost 1.67 REQUIRED)

set(CMAKE_THREAD_PREFER_PTHREAD TRUE)
set(THREADS_PREFER_PTHREAD_FLAG TRUE)
find_package(Threads REQUIRED)

add_library(asio INTERFACE)
target_include_directories(asio SYSTEM INTERFACE "${CMAKE_CURRENT_LIST_DIR}/asio-1.13.0/include")
target_compile_definitions(asio INTERFACE -DASIO_STANDALONE -DASIO_DISABLE_VISIBILITY -DASIO_NO_TYPEID)

add_library(catch INTERFACE)
target_include_directories(catch SYSTEM INTERFACE "${CMAKE_CURRENT_LIST_DIR}/catch")
target_compile_definitions(catch INTERFACE -DCATCH_CONFIG_NO_POSIX_SIGNALS)
