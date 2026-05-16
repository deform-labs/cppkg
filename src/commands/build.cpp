#include "../include/toml/toml_parser.h"
#include "../include/build/build.h"
#include "helpers/create_file.h"
#include "helpers/color.h"
#include <filesystem>
#include <iostream>
#include <cstdlib>
#include <thread>
#include <atomic>
#include <chrono>

namespace fs = std::filesystem;

void spinner(const std::string& label, std::atomic<bool>& done) {
    const char frames[] = { '|', '/', '-', '\\' };
    int i = 0;
    while (!done) {
        std::cout << "\r" << Color::cyan << frames[i++ % 4] << " " << label << Color::reset << std::flush;
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    std::cout << "\r" << Color::green << "[OK] " << label << Color::reset << "    \n";
}

void Build::build_project(const std::string& path) {
    auto toml = parse_toml(path + "/cppkg.toml");
    std::string name    = toml.get("package", "name");
    std::string cpp_std = toml.get("package", "cpp_std");

    if (cpp_std.substr(0, 3) == "c++")
        cpp_std = cpp_std.substr(3);

    fs::path project_dir = fs::current_path() / path;
    std::cout << Color::cyan << "Building project: " << name << Color::reset << "\n";

    create_file("CMakeLists.txt", project_dir.string(),
        "cmake_minimum_required(VERSION 3.10)\n"
        "project(" + name + ")\n\n"
        "set(CMAKE_CXX_STANDARD " + cpp_std + ")\n"
        "set(CMAKE_CXX_STANDARD_REQUIRED True)\n\n"
        "file(GLOB_RECURSE SOURCES \"src/*.cpp\")\n\n"
        "add_executable(" + name + " ${SOURCES})\n"
    );

    fs::current_path(project_dir);

    std::atomic<bool> done(false);

    std::thread t1(spinner, "Configuring...", std::ref(done));
    std::system("cmake -B build -G \"ninja\" > nul 2>&1");
    done = true;
    t1.join();

    done = false;
    std::thread t2(spinner, "Building...", std::ref(done));
    std::system("cmake --build build > nul 2>&1");
    done = true;
    t2.join();

    std::cout << Color::green << "Build successful: " << name << Color::reset << "\n";
}
