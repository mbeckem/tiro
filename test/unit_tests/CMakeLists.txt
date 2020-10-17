add_executable(unit_tests
    main.cpp
)

target_link_libraries(unit_tests PRIVATE common_flags)
target_link_libraries(unit_tests PRIVATE tiro_objects)
target_link_libraries(unit_tests PRIVATE Catch2::Catch2)
target_include_directories(unit_tests PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}")

add_subdirectory(bytecode)
add_subdirectory(compiler)
add_subdirectory(common)
add_subdirectory(eval)
add_subdirectory(support)
add_subdirectory(vm)
