add_executable(tiro_tests
    main.cpp
    test_parser.hpp
    test_rng.hpp
)

target_link_libraries(tiro_tests PRIVATE common_flags)
target_link_libraries(tiro_tests PRIVATE tiro_objects)
target_link_libraries(tiro_tests PRIVATE catch)

add_subdirectory(bytecode)
add_subdirectory(bytecode_gen)
add_subdirectory(codegen)
add_subdirectory(compiler)
add_subdirectory(core)
add_subdirectory(heap)
add_subdirectory(mir)
add_subdirectory(mir_gen)
add_subdirectory(objects)
add_subdirectory(semantics)
add_subdirectory(syntax)
add_subdirectory(vm)
