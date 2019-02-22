cmake_minimum_required(VERSION 3.13)
project(mt_example)

set(CMAKE_CXX_STANDARD 17)

set(CMAKE_CXX_FLAGS_DEBUG "")
set(CMAKE_CXX_FLAGS_RELEASE "")

include_directories(/usr/local/include)

set(CMAKE_CXX_FLAGS "-pthread -Wall -g")

add_executable(mt_example main.cpp)
