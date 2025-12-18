cmake_minimum_required(VERSION 3.16)
project(editer_tests LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(TEST_SOURCES
    test_main.cpp
    test_core.cpp
)

add_executable(editer_tests ${TEST_SOURCES})
target_include_directories(editer_tests PRIVATE ${CMAKE_CURRENT_LIST_DIR}/../include)
target_compile_features(editer_tests PRIVATE cxx_std_20)

enable_testing()
add_test(NAME EditerTests COMMAND editer_tests)

