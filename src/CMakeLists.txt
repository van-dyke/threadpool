cmake_minimum_required(VERSION 3.13)
project(threadpool)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CONFIGURATION_TYPES "Release")

add_executable(threadpool pool_test.cpp)

target_link_libraries(threadpool pthread)