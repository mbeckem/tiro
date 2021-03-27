target_sources(tiro_objects
    PRIVATE
        assert.cpp
        assert.hpp
        defs.cpp
        defs.hpp
        dynamic_bitset.hpp
        enum_flags.hpp
        fix.hpp
        format.cpp
        format.hpp
        fwd.hpp
        hash.hpp
        math.hpp
        overloaded.hpp
        safe_int.hpp
        scope_guards.hpp
        type_traits.hpp

        adt/function_ref.hpp
        adt/not_null.hpp
        adt/span.hpp
        adt/vec_ptr.hpp

        entities/entity_id.hpp
        entities/entity_storage_accessors.hpp
        entities/entity_storage_view.hpp
        entities/entity_storage.hpp

        memory/arena.cpp
        memory/arena.hpp
        memory/binary.hpp
        memory/byte_order.hpp
        memory/ref_counted.cpp
        memory/ref_counted.hpp
        memory/tagged_ptr.hpp

        ranges/iter_tools.hpp
        ranges/generator_range.hpp

        text/code_point_range.hpp
        text/string_table.cpp
        text/string_table.hpp
        text/string_utils.cpp
        text/string_utils.hpp
        text/unicode_data.cpp
        text/unicode.cpp
        text/unicode.hpp
)
