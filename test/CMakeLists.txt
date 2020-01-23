add_executable(tiro_tests
    main.cpp
    test_parser.hpp
    test_rng.hpp
)

target_compile_options(tiro_tests PRIVATE ${GLOBAL_COMPILE_FLAGS})
target_link_libraries(tiro_tests PRIVATE tiro_objects)
target_link_libraries(tiro_tests PRIVATE catch)

add_subdirectory(codegen)
add_subdirectory(compiler)
add_subdirectory(core)
add_subdirectory(heap)
add_subdirectory(objects)
add_subdirectory(semantics)
add_subdirectory(syntax)
add_subdirectory(vm)
