add_library(common_flags INTERFACE)
target_include_directories(common_flags INTERFACE "${PROJECT_SOURCE_DIR}/include" "${PROJECT_SOURCE_DIR}/src")
target_include_directories(common_flags SYSTEM INTERFACE "${PROJECT_SOURCE_DIR}/deps")

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

    if ((CMAKE_CXX_COMPILER_ID MATCHES "Clang") AND WIN32)
        # TODO: There appears to be a msvc stdlib issue with rtti?
        # "-D_HAS_STATIC_RTTI=0"
    else()
        target_compile_options(common_flags INTERFACE $<$<COMPILE_LANGUAGE:CXX>:-fno-rtti>)
    endif()
endif()


add_library(tiro_objects OBJECT)
target_link_libraries(tiro_objects PRIVATE common_flags)

target_link_libraries_system(tiro_objects PUBLIC fmt::fmt utf8::cpp absl::hash absl::flat_hash_map)
target_link_libraries(tiro_objects PUBLIC asio json Threads::Threads)


add_subdirectory(common)
add_subdirectory(compiler)
add_subdirectory(vm)
add_subdirectory(api)
add_subdirectory(run)

