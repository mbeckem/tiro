# TODO: Installation paths for exported targets
set(PUBLIC_INCLUDE "${PROJECT_BINARY_DIR}/include")

configure_file("${PROJECT_SOURCE_DIR}/include/tiro/api.h.in" "${PUBLIC_INCLUDE}/tiro/api.h" @ONLY)

add_library(tiro_api_objects OBJECT
    "${PUBLIC_INCLUDE}/tiro/api.h"

    compiler.cpp
    errors.cpp
    lib.cpp
    private.hpp
    utils.cpp
    vm.cpp
)

target_include_directories(tiro_api_objects PUBLIC "${PUBLIC_INCLUDE}")
target_link_libraries(tiro_api_objects PUBLIC tiro_objects)
target_link_libraries(tiro_api_objects PRIVATE common_flags)
target_compile_definitions(tiro_api_objects PRIVATE "TIRO_BUILDING_DLL")

# TODO: apparently the public object dependency (link to tiro_objects) is not propagated...
#       this is why we link to tiro_objects again below

# TODO: Strip all unexported symbols from c api
add_library(tiro STATIC)
target_include_directories(tiro PUBLIC "${PUBLIC_INCLUDE}")
target_link_libraries(tiro PRIVATE common_flags)
target_link_libraries(tiro PRIVATE tiro_objects tiro_api_objects)

add_library(tiro_dyn SHARED)
target_include_directories(tiro_dyn PUBLIC "${PUBLIC_INCLUDE}")
target_link_libraries(tiro_dyn PRIVATE common_flags)
target_link_libraries(tiro_dyn PRIVATE tiro_objects tiro_api_objects)

# Make sure that only exported symbols (i.e. tiro_*) are exported by the shared library.
# TODO: This should be done for msvc as well.
if(NOT EMSCRIPTEN AND ((CMAKE_CXX_COMPILER_ID STREQUAL "GNU") OR (CMAKE_CXX_COMPILER_ID STREQUAL "Clang")))
    target_link_libraries(tiro_dyn PRIVATE "-Wl,--version-script=${CMAKE_CURRENT_SOURCE_DIR}/api.version.gcc")
endif()
