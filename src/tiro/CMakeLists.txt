add_library(tiro_objects OBJECT)
target_include_directories(tiro_objects PUBLIC "${PROJECT_SOURCE_DIR}/src")
target_compile_options(tiro_objects PRIVATE ${GLOBAL_COMPILE_FLAGS})

target_link_libraries_system(tiro_objects PUBLIC fmt::fmt utf8::cpp)
target_link_libraries(tiro_objects PUBLIC Boost::boost)
target_link_libraries(tiro_objects PUBLIC asio)
target_link_libraries(tiro_objects PUBLIC Threads::Threads)
target_include_directories(tiro_objects SYSTEM PUBLIC "${PROJECT_SOURCE_DIR}/deps")

add_subdirectory(compiler)
add_subdirectory(core)
add_subdirectory(vm)

