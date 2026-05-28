cmake_minimum_required(VERSION 3.10)
project(cppkg_api)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Collect public API headers
file(GLOB_RECURSE API_HEADERS "*.h")

add_library(cppkg_api INTERFACE)

target_include_directories(cppkg_api
    INTERFACE
        ${CMAKE_CURRENT_SOURCE_DIR}
)

# API depends on engine
target_link_libraries(cppkg_api
    INTERFACE
        cppkg_engine
)
