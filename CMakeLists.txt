cmake_minimum_required(VERSION 3.20)
project(Projet LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)
FetchContent_Declare(
        googletest
        URL https://github.com/google/googletest/archive/03597a01ee50ed33e9dfd640b249b4be3799d395.zip
        DOWNLOAD_EXTRACT_TIMESTAMP TRUE
)
FetchContent_MakeAvailable(googletest)

add_compile_options(-Wall -Wextra -Wpedantic -Wconversion -Wsign-conversion -Werror)

add_library(projet_lib src/library.cpp)
target_include_directories(projet_lib PUBLIC include)

add_executable(projet main.cpp)
target_link_libraries(projet PRIVATE projet_lib)

add_executable(converter apps/converter.cpp)
target_link_libraries(converter PRIVATE projet_lib)

enable_testing()
add_executable(tests tests/test.cpp)
target_link_libraries(tests PRIVATE projet_lib GTest::gtest_main)
include(GoogleTest)
gtest_discover_tests(tests)