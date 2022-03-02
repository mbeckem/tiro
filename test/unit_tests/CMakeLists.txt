add_executable(unit_tests
    main.cpp
)
tiro_set_common_options(unit_tests)
target_link_libraries(unit_tests PRIVATE tiro_objects Catch2::Catch2)
target_include_directories(unit_tests PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}")

add_subdirectory(bytecode)
add_subdirectory(compiler)
add_subdirectory(common)
add_subdirectory(support)
add_subdirectory(vm)
