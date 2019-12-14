add_subdirectory("${CMAKE_CURRENT_LIST_DIR}/fmt-5.3.0" EXCLUDE_FROM_ALL)

set(UTF8_TESTS OFF CACHE BOOL "-- Override --")
set(UTF8_INSTALL OFF CACHE BOOL "-- Override --")
set(UTF8_SAMPLES OFF CACHE BOOL "-- Override --")
add_subdirectory("${CMAKE_CURRENT_LIST_DIR}/utfcpp-3.1" EXCLUDE_FROM_ALL)

# TODO use standalone asio (if asio at all) to get rid of link library "system"
set(Boost_USE_STATIC_LIBS       ON)
set(Boost_USE_MULTITHREADED     ON)
set(Boost_USE_STATIC_RUNTIME    OFF)
find_package(Boost 1.67 REQUIRED COMPONENTS system)

set(CMAKE_THREAD_PREFER_PTHREAD TRUE)
set(THREADS_PREFER_PTHREAD_FLAG TRUE)
find_package(Threads REQUIRED)
