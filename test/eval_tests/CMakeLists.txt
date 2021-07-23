add_executable(eval_tests
    main.cpp
    eval_test.cpp
    eval_test.hpp

    containers_test.cpp
    control_flow_test.cpp
    example_test.cpp
    functions_test.cpp
    modules_test.cpp
    objects_test.cpp
    operators_test.cpp
    order_test.cpp
    results_test.cpp
    std_lib_test.cpp
    strings_test.cpp
    variables_test.cpp
)
target_link_libraries(eval_tests PRIVATE common_flags tiro::tiro fmt::fmt Catch2::Catch2)
target_include_directories(eval_tests PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}")
