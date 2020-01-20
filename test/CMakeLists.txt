add_executable(unit_tests
    main.cpp
)

target_compile_options(unit_tests PRIVATE ${GLOBAL_COMPILE_FLAGS})
target_link_libraries(unit_tests PRIVATE tiro_objects)
target_link_libraries(unit_tests PRIVATE catch)

add_subdirectory(compiler)
add_subdirectory(core)
add_subdirectory(vm)
