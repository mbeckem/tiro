# TODO this executable should link against the dynamic or static libraries
add_executable(tiro_run main.cpp)
target_link_libraries(tiro_run PRIVATE common_flags)
target_link_libraries(tiro_run PRIVATE tiro_objects)

# stdc++fs not needed for apple clang
if((CMAKE_CXX_COMPILER_ID MATCHES "GNU") OR ((CMAKE_CXX_COMPILER_ID STREQUAL "Clang") AND NOT CMAKE_CXX_SIMULATE_ID STREQUAL "MSVC"))
    target_link_libraries(tiro_run PRIVATE stdc++fs)
endif()

# A test consumer for the c api
# TODO compiler independent compile flags
add_executable(tiro_run_c c_main.c)
target_link_libraries(tiro_run_c PRIVATE common_flags)
target_link_libraries(tiro_run_c PRIVATE tiro)
