# TODO this executable should link against the dynamic or static libraries
add_executable(tiro_run main.cpp)
target_compile_options(tiro_run PRIVATE ${GLOBAL_COMPILE_FLAGS})
target_link_libraries(tiro_run PRIVATE tiro_objects)

# TODO this is gcc specific
target_link_libraries(tiro_run PRIVATE -lstdc++fs)

# A test consumer for the c api
# TODO compiler independent compile flags
add_executable(tiro_run_c c_main.c)
target_compile_options(tiro_run_c PRIVATE "-Wall" "-Werror" "-std=c89")
target_link_libraries(tiro_run_c PRIVATE tiro_dyn)
