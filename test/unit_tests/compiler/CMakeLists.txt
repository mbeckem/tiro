add_subdirectory(ast_gen)
add_subdirectory(bytecode_gen)
add_subdirectory(ir)
add_subdirectory(ir_gen)
add_subdirectory(ir_passes)
add_subdirectory(parser)
add_subdirectory(semantics)
add_subdirectory(syntax)

target_sources(unit_tests
    PRIVATE
        source_map_test.cpp
)

