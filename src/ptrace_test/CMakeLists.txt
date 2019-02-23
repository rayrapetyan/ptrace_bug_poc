cmake_minimum_required(VERSION 3.6)
project(ptrace_test)

set(CMAKE_VERBOSE_MAKEFILE "ON")

set(CMAKE_CXX_FLAGS_DEBUG "")
set(CMAKE_CXX_FLAGS_RELEASE "")

set(CMAKE_CXX_STANDARD 17)

include_directories(/usr/local/include)

set(CMAKE_CXX_FLAGS "-pthread -Wall -g")

add_executable(ptrace_test
        debugger.cpp
        helpers.cpp
        main.cpp
        traced_proc.cpp
        traced_thread.cpp
        )

target_link_libraries(ptrace_test procstat)
