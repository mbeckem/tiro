target_sources(unit_tests
    PRIVATE
        source_map_test.cpp
        string_table_test.cpp
        test_parser.hpp
)

add_subdirectory(codegen) 
add_subdirectory(semantics)
add_subdirectory(syntax)
