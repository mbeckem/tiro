set(SOURCE_INCLUDE_DIR "${PROJECT_SOURCE_DIR}/include")
set(BUILD_INCLUDE_DIR "${PROJECT_BINARY_DIR}/include")

configure_file("${PROJECT_SOURCE_DIR}/include/tiro/api.h.in" "${BUILD_INCLUDE_DIR}/tiro/api.h" @ONLY)

set(TIRO_HEADERS
    "${BUILD_INCLUDE_DIR}/tiro/api.h"
)

add_library(common_flags INTERFACE)
target_include_directories(common_flags INTERFACE "${PROJECT_SOURCE_DIR}/src")
#
# Compiler warnings and compiler specific flags
#
# TODO MSCV
#
if((CMAKE_CXX_COMPILER_ID MATCHES "GNU") OR (CMAKE_CXX_COMPILER_ID MATCHES "Clang"))
    if (TIRO_WARNINGS)    
        target_compile_options(common_flags INTERFACE
            -Wno-unknown-warning-option
            # Disabled because of flexible array members :/
            # -pedantic
            # -pedantic-errors
            -Wall
            -Wextra

            -Wshadow
            -Wcast-align
            -Wunused
            -Wmisleading-indentation
            -Wduplicated-cond
            -Wduplicated-branches
            -Wlogical-op
            -Wnull-dereference
            -Wdouble-promotion
            -Wformat=2
            #    -Wlifetime # Recent clang only
            -Wno-exceptions
        )

        target_compile_options(common_flags INTERFACE
            $<$<COMPILE_LANGUAGE:CXX>:
                -Wnon-virtual-dtor
                -Wno-noexcept-type
                -Wno-terminate
            >            
        )
    endif()

    if (TIRO_WERROR)
        target_compile_options(common_flags INTERFACE -Werror)
    endif()
endif()

add_library(tiro_objects OBJECT)
target_include_directories(tiro_objects PUBLIC "${SOURCE_INCLUDE_DIR}" "${BUILD_INCLUDE_DIR}")
target_compile_definitions(tiro_objects PRIVATE "TIRO_BUILDING_DLL")
target_link_libraries(tiro_objects PRIVATE common_flags)
target_link_libraries_system(tiro_objects PUBLIC 
    absl::hash absl::flat_hash_map
    asio fmt::fmt nlohmann_json::nlohmann_json utf8::cpp
)
target_link_libraries(tiro_objects PUBLIC Threads::Threads)

add_subdirectory(common)
add_subdirectory(compiler)
add_subdirectory(vm)
add_subdirectory(api)
add_subdirectory(run)



# TODO: Strip all unexported symbols from c api and export this (must resolve the internal unexported dependencies issue with cmake somehow)
add_library(tiro_static STATIC)
target_include_directories(tiro_static PUBLIC 
    $<BUILD_INTERFACE:${BUILD_INCLUDE_DIR}>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)
target_link_libraries(tiro_static PRIVATE common_flags)
target_link_libraries(tiro_static PRIVATE tiro_objects)

add_library(tiro SHARED)
target_include_directories(tiro PUBLIC 
    $<BUILD_INTERFACE:${BUILD_INCLUDE_DIR}>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)
target_link_libraries(tiro PRIVATE common_flags)
target_link_libraries(tiro PRIVATE tiro_objects)

# Make sure that only exported symbols (i.e. tiro_*) are exported by the shared library.
# TODO: This should be done for msvc as well.
if(NOT EMSCRIPTEN AND ((CMAKE_CXX_COMPILER_ID STREQUAL "GNU") OR (CMAKE_CXX_COMPILER_ID STREQUAL "Clang")))
    target_link_libraries(tiro PRIVATE "-Wl,--version-script=${PROJECT_SOURCE_DIR}/utils/api.version.gcc")
endif()

set(TARGETS_EXPORT_NAME "tiro-targets")
write_basic_package_version_file(
    "${PROJECT_BINARY_DIR}/tiro-config-version.cmake"
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMinorVersion
)
configure_package_config_file(
    "${PROJECT_SOURCE_DIR}/utils/cmake/tiro-config.cmake.in"
    "${PROJECT_BINARY_DIR}/tiro-config.cmake"
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/tiro/cmake
)

install(TARGETS tiro 
    EXPORT ${TARGETS_EXPORT_NAME}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
)
install(EXPORT ${TARGETS_EXPORT_NAME}
    FILE ${TARGETS_EXPORT_NAME}.cmake
    NAMESPACE tiro::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/tiro
)
install(FILES ${TIRO_HEADERS} DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/tiro)
install(
    FILES 
        "${PROJECT_BINARY_DIR}/tiro-config-version.cmake"
        "${PROJECT_BINARY_DIR}/tiro-config.cmake"
    DESTINATION 
        "${CMAKE_INSTALL_LIBDIR}/cmake/tiro"
)


