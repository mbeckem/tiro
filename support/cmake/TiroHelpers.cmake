# Link against a list of targets and make sure that headers are included with SYSTEM mode,
# to supress errors or warnings in system libraries.
function(target_link_libraries_system target visibility)
    set(libs ${ARGN})
    foreach(lib ${libs})
        get_target_property(lib_include_dirs ${lib} INTERFACE_INCLUDE_DIRECTORIES)
        target_include_directories(${target} SYSTEM ${visibility} ${lib_include_dirs})
        target_link_libraries(${target} ${visibility} ${lib})
    endforeach(lib)
endfunction(target_link_libraries_system)

# Set common options for targets developed in this project.
function(tiro_set_common_options target)
    target_include_directories(${target} PRIVATE "${PROJECT_SOURCE_DIR}/src")

    # Compiler warnings and compiler specific flags
    # TODO MSCV
    if((CMAKE_CXX_COMPILER_ID MATCHES "GNU") OR (CMAKE_CXX_COMPILER_ID MATCHES "Clang"))
        if (TIRO_WARNINGS)
            target_compile_options(${target} PRIVATE
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

            target_compile_options(${target} PRIVATE
                $<$<COMPILE_LANGUAGE:CXX>:
                    -Wnon-virtual-dtor
                    -Wno-noexcept-type
                    -Wno-terminate

                    # Really annoying on some clang versions
                    -Wno-range-loop-analysis
                >
            )
        endif()

        if (TIRO_WERROR)
            target_compile_options(${target} PRIVATE -Werror)
        endif()
    endif()
endfunction()

function(tiro_add_headers VAR BASE)
    set(headers ${${VAR}})
    foreach (header ${ARGN})
        list(APPEND headers "${BASE}/${header}")
    endforeach()
    set(${VAR} ${headers} PARENT_SCOPE)
endfunction()

macro(tiro_apply_version MAJOR MINOR PATCH SUFFIX)
    set(PROJECT_VERSION "${MAJOR}.${MINOR}.${PATCH}${SUFFIX}")
    set(PROJECT_VERSION_MAJOR ${MAJOR})
    set(PROJECT_VERSION_MINOR ${MINOR})
    set(PROJECT_VERSION_PATCH ${PATCH})
endmacro()
