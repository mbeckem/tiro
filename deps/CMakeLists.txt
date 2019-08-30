add_subdirectory(fmt-5.3.0 EXCLUDE_FROM_ALL)

set(Boost_USE_STATIC_LIBS       ON)
set(Boost_USE_MULTITHREADED     ON)
set(Boost_USE_STATIC_RUNTIME    OFF)
find_package(Boost 1.70 REQUIRED) # TODO too recent
