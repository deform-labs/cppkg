cmake_minimum_required(VERSION 3.10)
project(cppkg_root)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_subdirectory(src/cppkg_engine)
add_subdirectory(src/api)
add_subdirectory(src/cli)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
