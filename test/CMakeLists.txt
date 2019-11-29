add_executable(unit_tests
    main.cpp

    core/arena.cpp
    core/math.cpp
    core/overloaded.cpp
    core/rtti.cpp
    core/scope.cpp
    core/unicode.cpp

    compiler/codegen/basic_block.cpp
    compiler/codegen/variable_locations.cpp
    compiler/lexer.cpp
    compiler/parser.cpp
    compiler/source_map.cpp
    compiler/string_table.cpp
    compiler/token.cpp

    vm/arrays.cpp
    vm/collector.cpp
    vm/context.cpp
    vm/function.cpp
    vm/hash_table.cpp
    vm/heap.cpp
    vm/small_integer.cpp
    vm/string.cpp
    vm/value.cpp
)

target_compile_options(unit_tests PRIVATE ${GLOBAL_COMPILE_FLAGS})
target_link_libraries(unit_tests PRIVATE hammer_objects)

# Catch2 header location
target_include_directories(unit_tests PRIVATE "${PROJECT_SOURCE_DIR}/deps")

