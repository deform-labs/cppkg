cmake_minimum_required(VERSION 3.10)
project(cppkg)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)

file(GLOB_RECURSE SOURCES
     "cppkg/src/*.cpp"
     "cppkg/src/*.h"
     "cppkg/src/**/*.cpp"
     "cppkg/src/**/*.h"
)


add_executable(cppkg ${SOURCES})
target_include_directories(cppkg PRIVATE src/include src)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
