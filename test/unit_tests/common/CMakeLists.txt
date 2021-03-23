target_sources(unit_tests
    PRIVATE
        dynamic_bitset_test.cpp
        enum_flags_test.cpp
        format_stream_test.cpp
        hash_test.cpp
        id_type_test.cpp
        math_test.cpp
        overloaded_test.cpp
        safe_int_test.cpp
        scope_guards_test.cpp
        type_traits_test.cpp

        adt/function_ref_test.cpp
        adt/index_map_test.cpp
        adt/not_null_test.cpp
        adt/vec_ptr_test.cpp

        memory/arena_test.cpp
        memory/byte_order_test.cpp
        memory/ref_counted_test.cpp
        memory/tagged_ptr_test.cpp

        ranges/generator_range_test.cpp
        ranges/iter_tools_test.cpp

        text/string_table_test.cpp
        text/string_utils_test.cpp
        text/unicode_test.cpp
)
