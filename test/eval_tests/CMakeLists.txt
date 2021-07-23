add_executable(eval_tests
    main.cpp
    eval_test.cpp
    eval_test.hpp

    containers_test.cpp
)
target_link_libraries(eval_tests PRIVATE common_flags tiro::tiro fmt::fmt Catch2::Catch2)
target_include_directories(eval_tests PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}")
