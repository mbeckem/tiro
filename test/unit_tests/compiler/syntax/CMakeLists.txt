target_sources(unit_tests
    PRIVATE
        build_syntax_tree_test.cpp
        lexer_test.cpp
        token_set_test.cpp
        parse_errors_test.cpp
        parse_expr_test.cpp
        parse_item_test.cpp
        parse_stmt_test.cpp

        syntax_assert.cpp
        syntax_assert.hpp
        simple_syntax_tree.cpp
        simple_syntax_tree.hpp
)
