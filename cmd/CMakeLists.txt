# TODO this executable should link against the dynamic or static libraries
add_executable(run main.cpp)
target_compile_options(run PRIVATE ${GLOBAL_COMPILE_FLAGS})
target_link_libraries(run PRIVATE hammer_objects)

# TODO this is gcc specific
target_link_libraries(run PRIVATE -lstdc++fs)
