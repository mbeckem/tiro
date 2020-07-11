add_subdirectory(handles)
add_subdirectory(heap)
add_subdirectory(objects)

target_sources(tiro_tests
    PRIVATE
        context_test.cpp
        load_test.cpp
        math_test.cpp
)
