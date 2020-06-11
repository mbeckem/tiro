add_executable(tiro_tests
    main.cpp
)

target_link_libraries(tiro_tests PRIVATE common_flags)
target_link_libraries(tiro_tests PRIVATE tiro_objects)
target_link_libraries(tiro_tests PRIVATE catch)
target_include_directories(tiro_tests PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}")

add_subdirectory(bytecode)
add_subdirectory(bytecode_gen)
add_subdirectory(compiler)
add_subdirectory(core)
add_subdirectory(eval)
add_subdirectory(heap)
add_subdirectory(ir)
add_subdirectory(ir_gen)
add_subdirectory(objects)
add_subdirectory(parser)
add_subdirectory(semantics)
add_subdirectory(support)
add_subdirectory(vm)
