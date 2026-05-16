#include "../include/dependency/dependency_service.h"
#include "../include/system/system.h"
#include "../include/build_project/build_project.h"
#include "../include/toml/toml_parser.h"
#include "../helpers/create_file.h"
#include "../helpers/color.h"
#include <filesystem>
#include <iostream>
#include <string>
#include <thread>
#include <atomic>
#include <chrono>

namespace fs = std::filesystem;

void loading_bar(const std::string& label, std::atomic<bool>& done) {
    const int bar_width = 30;
    int progress = 0;
    while (!done) {
        // Increment progress cyclically to give a moving effect
        progress = (progress + 1) % (bar_width + 1);
        int percent = (progress * 100) / bar_width;
        std::string bar = "[" + std::string(progress, '=') + std::string(bar_width - progress, ' ') + "]";
        std::cout << "\r" << Color::cyan << bar << " " << percent << "% " << label << Color::reset << std::flush;
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    // When done, show full bar
    std::string full_bar = "[" + std::string(bar_width, '=') + "]";
    std::cout << "\r" << Color::green << full_bar << " 100% " << label << Color::reset << "    \n";
}

static std::string detect_build_type(int argc, char* argv[]) {
    for (int i = 2; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--debug")   return "Debug";
        if (arg == "--release") return "Release";
    }
    return "RelWithDebInfo";
}

void Build::build_project(const std::string& path) {
    SystemService shell_; // used for running cmake commands
    auto toml = parse_toml(path + "/cppkg.toml");
    std::string name    = toml.get("package", "name");
    std::string cpp_std = toml.get("package", "cpp_std");

    if (cpp_std.substr(0, 3) == "c++")
        cpp_std = cpp_std.substr(3);

    fs::path project_dir = fs::current_path() / path;
    fs::current_path(project_dir);

    DependencyService deps;
    std::cout << Color::cyan << "Fetching dependencies..." << Color::reset << "\n";
    if (!deps.fetch_all(".")) {
        std::cout << Color::red << "Some dependencies failed to fetch" << Color::reset << "\n";
    }

    std::cout << Color::cyan << "Generating CMakeLists.txt..." << Color::reset << "\n";

    std::string cmake;
    cmake += "cmake_minimum_required(VERSION 3.10)\n";
    cmake += "project(" + name + ")\n\n";
    cmake += "set(CMAKE_CXX_STANDARD " + cpp_std + ")\n";
    cmake += "set(CMAKE_CXX_STANDARD_REQUIRED True)\n\n";
    cmake += "file(GLOB_RECURSE SOURCES \"src/*.cpp\")\n\n";

    auto dependencies = deps.load_dependencies(".");
    bool has_add_subdirectory = false;
    for (const auto& dep : dependencies) {
        cmake += "add_subdirectory(target/deps/" + dep.repo + ")\n";
        has_add_subdirectory = true;
     }
    if (has_add_subdirectory) cmake += "\n";

    cmake += "add_executable(" + name + " ${SOURCES})\n";
    cmake += "\ntarget_link_libraries(" + name + "\n";
    for (const auto& dep : dependencies) {
        cmake += "   " + dep.repo + "\n";
    }
    cmake += ")\n";

    create_file("CMakeLists.txt", project_dir.string(), cmake);

    std::atomic<bool> done(false);

    std::thread t1(loading_bar, "Configuring...", std::ref(done));
    // Detect build type (debug/release/relwithdebinfo)
    std::string build_type = detect_build_type(0, nullptr);
    int r1 = shell_.run("cmake -B target/build -DCMAKE_BUILD_TYPE=" + build_type);
    done = true;
    t1.join();

    if (r1 != 0) {
        std::cout << Color::red << "Configuration failed!" << Color::reset << "\n";
        return;
    }

    done = false;
    std::thread t2(loading_bar, "Building...", std::ref(done));
    int r2 = shell_.run("cmake --build target/build --progress");
    done = true;
    t2.join();

    if (r2 != 0) {
        std::cout << Color::red << "Build failed!" << Color::reset << "\n";
        return;
    }

    std::cout << Color::green << "Build successful: " << name << Color::reset << "\n";
}
