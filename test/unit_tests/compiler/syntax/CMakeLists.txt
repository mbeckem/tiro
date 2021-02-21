target_sources(unit_tests
    PRIVATE
        lexer_test.cpp
        token_set_test.cpp
        parse_expr_test.cpp
        parse_item_test.cpp
        parse_stmt_test.cpp

        syntax_assert.cpp
        syntax_assert.hpp
        syntax_tree.cpp
        syntax_tree.hpp
)
