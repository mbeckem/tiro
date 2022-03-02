add_executable(tiro_run
    main.cpp
)
tiro_set_common_options(tiro_run)
target_link_libraries(tiro_run PRIVATE tiropp cxxopts fmt::fmt)
target_compile_options(tiro_run PRIVATE "-frtti")

# stdc++fs not needed for apple clang
if((CMAKE_CXX_COMPILER_ID MATCHES "GNU") OR ((CMAKE_CXX_COMPILER_ID STREQUAL "Clang") AND NOT CMAKE_CXX_SIMULATE_ID STREQUAL "MSVC"))
    target_link_libraries(tiro_run PRIVATE stdc++fs)
endif()
