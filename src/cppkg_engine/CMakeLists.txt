cmake_minimum_required(VERSION 3.10)
project(cppkg_engine)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Collect all engine source files
file(GLOB_RECURSE ENGINE_SOURCES
    "src/*.cpp"
    "src/*.h"
    "src/**/*.h"
    "src/**/*.cpp"
)

add_library(cppkg_engine STATIC ${ENGINE_SOURCES})

target_include_directories(cppkg_engine
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}  # For internal includes
)

# Optional: expose API for external use
target_include_directories(cppkg_engine
    INTERFACE
        ${CMAKE_CURRENT_SOURCE_DIR}
)
