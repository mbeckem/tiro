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
