add_library(tiro_api_objects OBJECT
    api.cpp
    ${PROJECT_SOURCE_DIR}/include/tiro/api.h
)

target_link_libraries(tiro_api_objects PUBLIC tiro_objects)
target_link_libraries(tiro_api_objects PRIVATE common_flags)
target_compile_definitions(tiro_api_objects PRIVATE "TIRO_BUILDING_DLL")

# TODO: apparently the public object dependency (link to tiro_objects) is not propagated...
#       this is why we link to tiro_objects again below

# TODO: Strip all unexported symbols from c api
add_library(tiro STATIC)
target_link_libraries(tiro PRIVATE common_flags)
target_link_libraries(tiro PRIVATE tiro_objects tiro_api_objects)

add_library(tiro_dyn SHARED)
target_link_libraries(tiro_dyn PRIVATE common_flags)
target_link_libraries(tiro_dyn PRIVATE tiro_objects tiro_api_objects)

if((CMAKE_CXX_COMPILER_ID MATCHES "GNU") OR (CMAKE_CXX_COMPILER_ID MATCHES "Clang"))
    target_link_libraries(tiro_dyn PRIVATE "-Wl,--version-script=${CMAKE_CURRENT_SOURCE_DIR}/api.version.gcc")
endif()
