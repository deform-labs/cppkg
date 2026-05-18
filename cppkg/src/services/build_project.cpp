#include "../include/dependency/dependency_service.h"
#include "../include/build_project/build_project.h"
#include "../include/toml/toml_parser.h"
#include "../include/turtle/turtle.h"
#include "../helpers/create_file.h"
#include "../helpers/color.h"
#include <filesystem>
#include <iostream>
#include <string>
#include <thread>
#include <atomic>
#include <chrono>

namespace fs = std::filesystem;

/// Spinner animation lmao W UX
void spinner(const std::string& label, std::atomic<bool>& done) {
    const char frames[] = { '|', '/', '-', '\\' };
    int i = 0;
    while (!done) {
        std::cout << "\r" << Color::cyan << frames[i++ % 4] << " " << label << Color::reset << std::flush;
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    std::cout << "\r" << Color::green << "[OK] " << label << Color::reset << "    \n";
};

/// Detect the build type
static std::string detect_build_type(int argc, char* argv[]) {
    for (int i = 2; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--debug")   return "Debug";
        if (arg == "--release") return "Release";
    }
    return "RelWithDebInfo";
}

/// Build the project
void Build::build_project(const std::string& path) {
    turtle shell_; // i think that a turtle also has a shell... Dunno what language it uses tho
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

    std::thread t1(spinner, "Configuring: ", std::ref(done));
    std::string build_type = detect_build_type(0, nullptr);
    int r1 = shell_.run("cmake -B target/build -DCMAKE_BUILD_TYPE=" + build_type);
    done = true;
    t1.join();

    if (r1 != 0) {
        std::cout << Color::red << "Configuration failed!" << Color::reset << "\n";
        return;
    }

    done = false;
    std::thread t2(spinner, "Building: ", std::ref(done));
    int r2 = shell_.run("cmake --build target/build");
    done = true;
    t2.join();

    if (r2 != 0) {
        std::cout << Color::red << "Build failed!" << Color::reset << "\n";
        return;
    }

    std::cout << Color::green << "Build successful: " << name << Color::reset << "\n";
}

/// well well well. What do we have here, an user in a hurry i see.
void Build::run_project(const std::string& path) {
    turtle shell_; ///another shell? I thought turtles only had 1!
    auto toml = parse_toml(path + "/cppkg.toml");
    std::string name = toml.get("package", "name");

    std::atomic<bool> done(false);

    std::thread t(spinner, "Running: ", std::ref(done));
    int r = shell_.run("target/build/" + name);
    done = true;
    t.join();

    if (r != 0) {
        std::cout << Color::red << "Run failed!" << Color::reset << "\n";
    } else {
        std::cout << Color::green << "Run successful: " << name << Color::reset << "\n";
    }
}
