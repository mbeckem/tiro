add_library(common_flags INTERFACE)
target_include_directories(common_flags INTERFACE "${PROJECT_SOURCE_DIR}/src")

# Compiler warnings and compiler specific flags
# TODO MSCV
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

set(SOURCE_INCLUDE_DIR "${PROJECT_SOURCE_DIR}/include")
set(BUILD_INCLUDE_DIR "${PROJECT_BINARY_DIR}/include")

tiro_add_headers(TIRO_HEADERS "${SOURCE_INCLUDE_DIR}/tiro"
    api.h
    compiler.h
    def.h
    error.h
    objects.h
    vm.h
)
tiro_add_headers(TIRO_HEADERS "${BUILD_INCLUDE_DIR}/tiro/" version.h)

tiro_add_headers(TIROPP_HEADERS "${SOURCE_INCLUDE_DIR}/tiropp"
    api.hpp
    compiler.hpp 
    def.hpp
    error.hpp
    fwd.hpp
    native_type.hpp
    objects.hpp
    version.hpp
    vm.hpp

    detail/handle_check.hpp
    detail/resource_holder.hpp
)

set(VERSION_SCRIPT "${PROJECT_BINARY_DIR}/GenerateVersion.cmake")
configure_file("${PROJECT_SOURCE_DIR}/support/cmake/GenerateVersion.cmake.in" "${VERSION_SCRIPT}" @ONLY)
set(VERSION_CONFIGURE_COMMAND "${CMAKE_COMMAND}" "-P" "${VERSION_SCRIPT}")

# Initial version.h content, so it is available after cmake configure
execute_process(COMMAND ${VERSION_CONFIGURE_COMMAND})

# This target runs every time, but the version script ensures that the header is only modified 
# if the content actually changed.
add_custom_target(tiro_version_header
    COMMAND ${VERSION_CONFIGURE_COMMAND}
    BYPRODUCTS "${BUILD_INCLUDE_DIR}/tiro/version.h"
    COMMENT "Updating version.h"
)

add_library(tiro_objects OBJECT)
add_dependencies(tiro_objects tiro_version_header)
target_sources(tiro_objects PRIVATE ${TIRO_HEADERS})
target_include_directories(tiro_objects PUBLIC "${SOURCE_INCLUDE_DIR}" "${BUILD_INCLUDE_DIR}")
target_compile_definitions(tiro_objects PRIVATE "TIRO_BUILDING_DLL")
target_link_libraries(tiro_objects PRIVATE common_flags)
target_link_libraries_system(tiro_objects PUBLIC 
    absl::hash absl::flat_hash_map
    fmt::fmt nlohmann_json::nlohmann_json utf8::cpp
)
target_link_libraries(tiro_objects PUBLIC Threads::Threads)

add_subdirectory(common)
add_subdirectory(compiler)
add_subdirectory(vm)
add_subdirectory(api)
add_subdirectory(run)

# TODO: Strip all unexported symbols from c api and export this (must resolve the internal unexported dependencies issue with cmake somehow)
add_library(tiro_static STATIC)
target_include_directories(tiro_static
    PUBLIC 
        $<BUILD_INTERFACE:${SOURCE_INCLUDE_DIR}>
        $<BUILD_INTERFACE:${BUILD_INCLUDE_DIR}>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)
target_link_libraries(tiro_static PRIVATE common_flags)
target_link_libraries(tiro_static PRIVATE tiro_objects)

# TODO: Silly duplication
add_library(tiro SHARED)
add_library(tiro::tiro ALIAS tiro)
target_include_directories(tiro
    PUBLIC 
        $<BUILD_INTERFACE:${SOURCE_INCLUDE_DIR}>
        $<BUILD_INTERFACE:${BUILD_INCLUDE_DIR}>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)
target_link_libraries(tiro PRIVATE common_flags)
target_link_libraries(tiro PRIVATE tiro_objects)

# Make sure that only exported symbols (i.e. tiro_*) are exported by the shared library.
# TODO: This should be done for msvc as well.
if(NOT EMSCRIPTEN AND ((CMAKE_CXX_COMPILER_ID STREQUAL "GNU") OR (CMAKE_CXX_COMPILER_ID STREQUAL "Clang")))
    target_link_libraries(tiro PRIVATE "-Wl,--version-script=${PROJECT_SOURCE_DIR}/support/api.version.gcc")
endif()

add_library(tiropp INTERFACE)
add_library(tiro::tiropp ALIAS tiropp)
target_link_libraries(tiropp INTERFACE tiro::tiro)
target_include_directories(tiropp
    INTERFACE
        $<BUILD_INTERFACE:${SOURCE_INCLUDE_DIR}>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

# Setup exported files for find_package support.
set(TARGETS_EXPORT_NAME "tiro-targets")
write_basic_package_version_file(
    "${PROJECT_BINARY_DIR}/tiro-config-version.cmake"
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMinorVersion
)
configure_package_config_file(
    "${PROJECT_SOURCE_DIR}/support/cmake/tiro-config.cmake.in"
    "${PROJECT_BINARY_DIR}/tiro-config.cmake"
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/tiro/cmake
)

install(TARGETS tiro tiropp
    EXPORT ${TARGETS_EXPORT_NAME}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
)
install(EXPORT ${TARGETS_EXPORT_NAME}
    FILE ${TARGETS_EXPORT_NAME}.cmake
    NAMESPACE tiro::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/tiro
)
install(FILES ${TIRO_HEADERS} DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/tiro)
install(FILES ${TIROPP_HEADERS} DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/tiropp)
install(
    FILES 
        "${PROJECT_BINARY_DIR}/tiro-config-version.cmake"
        "${PROJECT_BINARY_DIR}/tiro-config.cmake"
    DESTINATION 
        "${CMAKE_INSTALL_LIBDIR}/cmake/tiro"
)
