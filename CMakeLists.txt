cmake_minimum_required(VERSION 3.14)
project(tape_sort VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)

include(FetchContent)
FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG release-1.12.1
)

set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

enable_testing()


add_executable(tape_sort
    src/Config.cpp
    src/main.cpp
    src/FileTape.cpp
    src/TapeSorter.cpp
)

target_include_directories(tape_sort PRIVATE ${CMAKE_SOURCE_DIR}/include)

add_executable(tape_sort_tests
    src/Config.cpp
    src/FileTape.cpp
    src/TapeSorter.cpp
    tests/test_tape.cpp
)

target_include_directories(tape_sort_tests PRIVATE ${CMAKE_SOURCE_DIR}/include)
target_link_libraries(tape_sort_tests
    GTest::gtest
    GTest::gtest_main
)

include(GoogleTest)
gtest_discover_tests(tape_sort_tests)