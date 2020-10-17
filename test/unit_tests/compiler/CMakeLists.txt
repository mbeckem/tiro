add_subdirectory(bytecode_gen)
add_subdirectory(ir)
add_subdirectory(ir_gen)
add_subdirectory(parser)
add_subdirectory(semantics)

target_sources(unit_tests
    PRIVATE
        source_map_test.cpp
)

