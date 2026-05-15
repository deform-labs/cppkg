#include "build/build.h"
#include <cstdlib>
#include <fstream>

void build_project(const std::string& name, const std::string& cpp_std) {
    // generate CMakeLists.txt
    std::ofstream cmake("CMakeLists.txt");
    cmake << "cmake_minimum_required(VERSION 3.10)\n";
    cmake << "project(" << name << ")\n\n";
    cmake << "set(CMAKE_CXX_STANDARD " << cpp_std << ")\n";
    cmake << "set(CMAKE_CXX_STANDARD_REQUIRED True)\n\n";
    cmake << "file(GLOB_RECURSE SOURCES \"src/*.cpp\")\n\n";
    cmake << "add_executable(" << name << " ${SOURCES})\n";

    // run cmake
    std::system("cmake -B build");
    std::system("cmake --build build");
}
