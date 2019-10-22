target_sources(hammer_objects
    PRIVATE
        basic_block.cpp
        codegen.cpp
        expr_codegen.cpp
        stmt_codegen.cpp
        variable_locations.cpp

    PUBLIC
        basic_block.hpp
        codegen.hpp
        expr_codegen.hpp
        stmt_codegen.hpp
        variable_locations.hpp
)
