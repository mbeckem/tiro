add_subdirectory(ast)
add_subdirectory(bytecode_gen)
add_subdirectory(ir)
add_subdirectory(ir_gen)
add_subdirectory(ir_passes)
add_subdirectory(parser)
add_subdirectory(semantics)

target_sources(tiro_objects
    PRIVATE
        fwd.hpp

        compiler.cpp
        diagnostics.cpp
        diagnostics.hpp
        reset_value.hpp
        source_map.cpp
        source_map.hpp
        source_reference.cpp
        source_reference.hpp
        utils.cpp
        utils.hpp
)
