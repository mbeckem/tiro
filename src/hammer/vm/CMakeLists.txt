target_sources(hammer_objects
    PRIVATE
        collector.hpp
        context.hpp
        context.ipp
        handles.hpp
        heap.hpp

    PRIVATE
        collector.cpp
        context.cpp
        handles.cpp
        heap.cpp
)

add_subdirectory(objects)
