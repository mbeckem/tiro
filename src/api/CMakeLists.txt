# TODO: Strip all unexported symbols from c api
add_library(hammer STATIC
    api.cpp
     ${PROJECT_SOURCE_DIR}/include/hammer/api.h
)
target_link_libraries(hammer PRIVATE hammer_objects)
target_include_directories(hammer SYSTEM PUBLIC "${PROJECT_SOURCE_DIR}/deps")
target_include_directories(hammer PUBLIC "${PROJECT_SOURCE_DIR}/lib" "${PROJECT_SOURCE_DIR}/include")
target_compile_options(hammer PRIVATE ${GLOBAL_COMPILE_FLAGS})
