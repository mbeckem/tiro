add_executable(api_tests
    main.cpp

    helpers.hpp
    matchers.hpp

    tiro_functions_test.cpp
    tiro_objects_test.cpp
    tiro_vm_test.cpp

    tiropp_compiler_test.cpp
    tiropp_error_test.cpp
    tiropp_functions_test.cpp
    tiropp_native_type_test.cpp
    tiropp_objects_test.cpp
    tiropp_vm_test.cpp
)
tiro_set_common_options(api_tests)
target_link_libraries(api_tests PRIVATE tiro::tiro fmt::fmt Catch2::Catch2)
target_include_directories(api_tests PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}")
