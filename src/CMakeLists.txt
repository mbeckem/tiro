set(SOURCE_INCLUDE_DIR "${PROJECT_SOURCE_DIR}/include")
set(BUILD_INCLUDE_DIR "${PROJECT_BINARY_DIR}/include")

tiro_add_headers(TIRO_HEADERS "${SOURCE_INCLUDE_DIR}/tiro"
    api.h
    compiler.h
    def.h
    error.h
    functions.h
    objects.h
    vm.h
)
tiro_add_headers(TIRO_HEADERS "${BUILD_INCLUDE_DIR}/tiro/" version.h)

tiro_add_headers(TIROPP_HEADERS "${SOURCE_INCLUDE_DIR}/tiropp"
    api.hpp
    compiler.hpp
    def.hpp
    error.hpp
    functions.hpp
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
execute_process(COMMAND ${VERSION_CONFIGURE_COMMAND})

add_library(tiro_objects OBJECT)
tiro_set_common_options(tiro_objects)
target_sources(tiro_objects PRIVATE ${TIRO_HEADERS})
target_include_directories(tiro_objects PUBLIC "${SOURCE_INCLUDE_DIR}" "${BUILD_INCLUDE_DIR}")
target_compile_definitions(tiro_objects PRIVATE "TIRO_BUILDING_LIBRARY")
target_link_libraries_system(tiro_objects PUBLIC
    absl::hash absl::flat_hash_map
    fmt::fmt nlohmann_json::nlohmann_json utf8::cpp
)
target_link_libraries(tiro_objects PUBLIC Threads::Threads)

if (TIRO_IWYU)
    set_property(TARGET tiro_objects PROPERTY CXX_INCLUDE_WHAT_YOU_USE ${IWYU_PATH})
endif()

add_subdirectory(api)
add_subdirectory(bytecode)
add_subdirectory(common)
add_subdirectory(compiler)
add_subdirectory(run)
add_subdirectory(vm)

if (TIRO_BUILD_SHARED)
    add_library(tiro SHARED)
    tiro_set_common_options(tiro)
    target_include_directories(tiro
        PUBLIC
            $<BUILD_INTERFACE:${SOURCE_INCLUDE_DIR}>
            $<BUILD_INTERFACE:${BUILD_INCLUDE_DIR}>
            $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
    )
    target_link_libraries(tiro PRIVATE tiro_objects)
    # Make sure that only exported symbols (i.e. tiro_*) are exported by the shared library.
    # TODO: This should be done for msvc as well.
    if(NOT EMSCRIPTEN AND ((CMAKE_CXX_COMPILER_ID STREQUAL "GNU") OR (CMAKE_CXX_COMPILER_ID STREQUAL "Clang")))
        target_link_libraries(tiro PRIVATE "-Wl,--version-script=${PROJECT_SOURCE_DIR}/support/api.version.gcc")
    endif()
else()
    add_library(tiro_static_internal STATIC)
    tiro_set_common_options(tiro_static_internal)
    target_include_directories(tiro_static_internal PUBLIC
            $<BUILD_INTERFACE:${SOURCE_INCLUDE_DIR}>
            $<BUILD_INTERFACE:${BUILD_INCLUDE_DIR}>
            $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}> # TODO unnecessary after bundling
    )
    target_link_libraries(tiro_static_internal PRIVATE tiro_objects)

    add_library(tiro INTERFACE) # TODO partial linking
    target_link_libraries(tiro INTERFACE tiro_static_internal)
endif()
add_library(tiro::tiro ALIAS tiro)

add_library(tiropp INTERFACE)
add_library(tiro::tiropp ALIAS tiropp)
target_link_libraries(tiropp INTERFACE tiro)
target_include_directories(tiropp
    INTERFACE
        $<BUILD_INTERFACE:${SOURCE_INCLUDE_DIR}>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

if (NOT TIRO_SKIP_INSTALL)
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
endif()
