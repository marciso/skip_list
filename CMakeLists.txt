cmake_minimum_required(VERSION 3.6)
project(skip_list)

set(CMAKE_CXX_STANDARD 14)

enable_testing()

include(gtest.cmake)

add_subdirectory(src)
add_subdirectory(test)
