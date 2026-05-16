#include "../include/dependency/dependency_service.h"
#include "../include/build_project/build_project.h"
#include "../include/toml/toml_parser.h"
#include "../helpers/create_file.h"
#include "../helpers/color.h"
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

/// Determine build type from optional flags
static std::string detect_build_type(int argc, char* argv[]) {
    // Flags: --debug -> Debug, --release -> Release, default -> RelWithDebInfo
    for (int i = 2; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--debug")   return "Debug";
        if (arg == "--release") return "Release";
    }
    return "RelWithDebInfo";
}

void Build::build_project(const std::string& path) {
    auto toml = parse_toml(path + "/cppkg.toml");
    std::string name    = toml.get("package", "name");
    std::string cpp_std = toml.get("package", "cpp_std");

    if (cpp_std.substr(0, 3) == "c++")
        cpp_std = cpp_std.substr(3);

    fs::path project_dir = fs::current_path() / path;
    fs::current_path(project_dir);

    // ---- Step 1: Fetch dependencies ----
    DependencyService deps;
    std::cout << Color::cyan << "Fetching dependencies..." << Color::reset << "\n";
    if (!deps.fetch_all(".")) {
        std::cout << Color::red << "Some dependencies failed to fetch" << Color::reset << "\n";
    }

    // ---- Step 2: Generate CMakeLists.txt ----
    std::cout << Color::cyan << "Generating CMakeLists.txt..." << Color::reset << "\n";

    std::string cmake = R"(cmake_minimum_required(VERSION 3.10)
project()" + name + R"()

set(CMAKE_CXX_STANDARD )" + cpp_std + R"()
set(CMAKE_CXX_STANDARD_REQUIRED True)

file(GLOB_RECURSE SOURCES "src/*.cpp")

)";

    // Add dependency subdirectories
    auto dependencies = deps.load_dependencies(".");
    bool has_add_subdirectory = false;
    for (const auto& dep : dependencies) {
        cmake += "add_subdirectory(deps/" + dep.repo + ")\n";
        has_add_subdirectory = true;
    }
    if (has_add_subdirectory) cmake += "\n";

    // Link libraries (using target name same as repo name)
    cmake += "add_executable(" + name + " ${SOURCES})\n";
    cmake += "\ntarget_link_libraries(" + name + "\n";
    for (const auto& dep : dependencies) {
        cmake += "    " + dep.repo + "\n";
    }
    cmake += ")\n";

    create_file("CMakeLists.txt", project_dir.string(), cmake);

    // ---- Step 3: Build ----
    std::string build_type = "RelWithDebInfo";

    std::atomic<bool> done(false);

    std::thread t1(spinner, "Configuring...", std::ref(done));
    std::string configure_cmd = "cmake -B build/target -G \"ninja\" -DCMAKE_BUILD_TYPE=" + build_type + " > nul 2>&1";
    std::system(configure_cmd.c_str());
    done = true;
    t1.join();

    done = false;
    std::thread t2(spinner, "Building...", std::ref(done));
    std::string build_cmd = "cmake --build build/target > nul 2>&1";
    std::system(build_cmd.c_str());
    done = true;
    t2.join();

    std::cout << Color::green << "Build successful: " << name << Color::reset << "\n";
}
