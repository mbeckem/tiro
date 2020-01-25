add_library(tiro_api_objects OBJECT
    api.cpp
    ${PROJECT_SOURCE_DIR}/include/tiro/api.h
)
target_link_libraries(tiro_api_objects PUBLIC tiro_objects)
target_include_directories(tiro_api_objects PUBLIC "${PROJECT_SOURCE_DIR}/include")
target_include_directories(tiro_api_objects PRIVATE "${PROJECT_SOURCE_DIR}/src")
target_compile_options(tiro_api_objects PRIVATE ${GLOBAL_COMPILE_FLAGS})
target_compile_definitions(tiro_api_objects PRIVATE "TIRO_BUILDING_DLL")

# TODO: apparently the public object dependency (link to tiro_objects) is not propagated...
#       this is why we link to tiro_objects again below

# TODO: Strip all unexported symbols from c api
add_library(tiro STATIC)
target_link_libraries(tiro PRIVATE tiro_objects tiro_api_objects)
target_include_directories(tiro PUBLIC "${PROJECT_SOURCE_DIR}/include")

add_library(tiro_dyn SHARED)
target_link_libraries(tiro_dyn PRIVATE tiro_objects tiro_api_objects)
target_include_directories(tiro_dyn PUBLIC "${PROJECT_SOURCE_DIR}/include")

if((CMAKE_CXX_COMPILER_ID MATCHES "GNU") OR (CMAKE_CXX_COMPILER_ID MATCHES "Clang"))
    target_link_libraries(tiro_dyn PRIVATE "-Wl,--version-script=${CMAKE_CURRENT_SOURCE_DIR}/api.version.gcc")
endif()
