add_subdirectory(eval)
add_subdirectory(handles)
add_subdirectory(heap)
add_subdirectory(object_support)
add_subdirectory(objects)

target_sources(unit_tests
    PRIVATE
        context_test.cpp
        hash_test.cpp
        module_loader_test.cpp
        math_test.cpp
        module_registry_test.cpp
)
