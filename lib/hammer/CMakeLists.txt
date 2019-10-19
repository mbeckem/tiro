add_library(hammer_objects OBJECT)
target_link_libraries_system(hammer_objects PUBLIC fmt::fmt utf8::cpp)
target_include_directories(hammer_objects SYSTEM PUBLIC "${PROJECT_SOURCE_DIR}/deps")
target_include_directories(hammer_objects PUBLIC "${PROJECT_SOURCE_DIR}/lib")
target_compile_options(hammer_objects PRIVATE ${GLOBAL_COMPILE_FLAGS})

# TODO: Strip all unexported symbols from c api
add_library(hammer STATIC)
target_link_libraries(hammer PRIVATE hammer_objects)
target_include_directories(hammer SYSTEM PUBLIC "${PROJECT_SOURCE_DIR}/deps")
target_include_directories(hammer PUBLIC "${PROJECT_SOURCE_DIR}/lib" "${PROJECT_SOURCE_DIR}/include")
target_compile_options(hammer PRIVATE ${GLOBAL_COMPILE_FLAGS})

add_subdirectory(ast)
add_subdirectory(c_api)
add_subdirectory(compiler)
add_subdirectory(core)
add_subdirectory(vm)
