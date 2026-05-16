cmake_minimum_required(VERSION 3.10)
project(cppkg)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)

file(GLOB_RECURSE SOURCES "src/*.cpp")

add_executable(cppkg ${SOURCES})

target_link_libraries(cppkg
)
