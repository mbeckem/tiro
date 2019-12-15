# TODO this executable should link against the dynamic or static libraries
add_executable(run main.cpp)
target_compile_options(run PRIVATE ${GLOBAL_COMPILE_FLAGS})
target_link_libraries(run PRIVATE hammer_objects)

# TODO this is gcc specific
target_link_libraries(run PRIVATE -lstdc++fs)

# A test consumer for the c api
# TODO compiler independent compile flags
add_executable(run_c c_main.c)
target_compile_options(run_c PRIVATE "-Wall" "-Werror" "-std=c89")
target_link_libraries(run_c PRIVATE hammer_dyn)
