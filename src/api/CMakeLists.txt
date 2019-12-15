add_library(hammer_api_objects OBJECT
    api.cpp
    ${PROJECT_SOURCE_DIR}/include/hammer/api.h
)
target_link_libraries(hammer_api_objects PUBLIC hammer_objects)
target_include_directories(hammer_api_objects PUBLIC "${PROJECT_SOURCE_DIR}/include")
target_include_directories(hammer_api_objects PRIVATE "${PROJECT_SOURCE_DIR}/src")
target_compile_options(hammer_api_objects PRIVATE ${GLOBAL_COMPILE_FLAGS})

# TODO: apparently the public object dependency (link to hammer_objects) is not propagated...
#       this is why we link to hammer_objects again below

# TODO: Strip all unexported symbols from c api
add_library(hammer STATIC)
target_link_libraries(hammer PRIVATE hammer_objects hammer_api_objects)
target_include_directories(hammer PUBLIC "${PROJECT_SOURCE_DIR}/include")

add_library(hammer_dyn SHARED)
target_link_libraries(hammer_dyn PRIVATE hammer_objects hammer_api_objects)
target_include_directories(hammer_dyn PUBLIC "${PROJECT_SOURCE_DIR}/include")
