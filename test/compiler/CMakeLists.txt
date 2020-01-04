target_sources(unit_tests
    PRIVATE
        lexer.cpp
        parser.cpp
        simplifier.cpp
        source_map.cpp
        string_table.cpp
        test_parser.hpp
        token.cpp
)

add_subdirectory(codegen) 
