cmake_minimum_required(VERSION 3.10)
project(cppkg_cli)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Collect CLI source files
file(GLOB_RECURSE CLI_SOURCES "*.cpp")

add_executable(cppkg ${CLI_SOURCES})

# Link dependencies
target_link_libraries(cppkg
    PRIVATE
        cppkg_engine
        cppkg_api
)

# Include paths
target_include_directories(cppkg
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_SOURCE_DIR}/src/api
        ${CMAKE_SOURCE_DIR}/src/cppkg_engine
)
