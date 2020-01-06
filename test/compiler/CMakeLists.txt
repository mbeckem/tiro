target_sources(unit_tests
    PRIVATE
        source_map.cpp
        string_table.cpp
        test_parser.hpp
)

add_subdirectory(codegen) 
add_subdirectory(semantics)
add_subdirectory(syntax)
