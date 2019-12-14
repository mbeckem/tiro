target_sources(hammer_objects
    PRIVATE
        context.hpp
        context.ipp
        fwd.hpp
        hash.hpp
        interpreter.hpp
        load.hpp
        math.hpp
        types.hpp

    PRIVATE
        context.cpp
        hash.cpp
        interpreter.cpp
        load.cpp
        math.cpp
        types.cpp
)

add_subdirectory(builtin)
add_subdirectory(heap)
add_subdirectory(objects)
