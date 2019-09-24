add_subdirectory(fmt-5.3.0 EXCLUDE_FROM_ALL)

set(UTF8_TESTS OFF)
set(UTF8_INSTALL OFF)
set(UTF8_SAMPLES OFF)
add_subdirectory(utfcpp-3.1 EXCLUDE_FROM_ALL)

set(Boost_USE_STATIC_LIBS       ON)
set(Boost_USE_MULTITHREADED     ON)
set(Boost_USE_STATIC_RUNTIME    OFF)
find_package(Boost 1.65 REQUIRED)
