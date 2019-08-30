# TODO this executable should link against the dynamic or static libraries
add_executable(exec main.cpp)
target_compile_options(exec PRIVATE ${GLOBAL_COMPILE_FLAGS})
target_link_libraries(exec PRIVATE hammer_objects)

# TODO this is gcc specific
target_link_libraries(exec PRIVATE -lstdc++fs)
