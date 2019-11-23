target_sources(hammer_objects
    PRIVATE
        collector.hpp
        context.hpp
        context.ipp
        fwd.hpp
        handles.hpp
        hash.hpp
        heap.hpp
        interpreter.hpp
        load.hpp
        math.hpp
        types.hpp

    PRIVATE
        collector.cpp
        context.cpp
        handles.cpp
        hash.cpp
        heap.cpp
        interpreter.cpp
        load.cpp
        math.cpp
        types.cpp
)

add_subdirectory(builtin)
add_subdirectory(objects)
