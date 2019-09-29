add_subdirectory(fmt-5.3.0 EXCLUDE_FROM_ALL)

set(UTF8_TESTS OFF CACHE BOOL "-- Override --")
set(UTF8_INSTALL OFF CACHE BOOL "-- Override --")
set(UTF8_SAMPLES OFF CACHE BOOL "-- Override --")
add_subdirectory(utfcpp-3.1 EXCLUDE_FROM_ALL)

set(Boost_USE_STATIC_LIBS       ON)
set(Boost_USE_MULTITHREADED     ON)
set(Boost_USE_STATIC_RUNTIME    OFF)
find_package(Boost 1.65 REQUIRED)
