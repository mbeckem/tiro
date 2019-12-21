target_sources(unit_tests
    PRIVATE
        lexer.cpp
        parser.cpp
        source_map.cpp
        string_table.cpp
        token.cpp
)

add_subdirectory(codegen)
