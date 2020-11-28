cmake_minimum_required(VERSION 3.13)
cmake_policy(VERSION 3.13)

project(example_embed_cpp LANGUAGES CXX C)
set(CMAKE_C_STANDARD 90)
set(CMAKE_CXX_STANDARD 17)

if (WIN32) 
    add_compile_definitions(_SILENCE_ALL_CXX17_DEPRECATION_WARNINGS=1)
endif()

add_executable(${PROJECT_NAME} main.cpp)
target_link_libraries(${PROJECT_NAME} tiro::tiropp)
