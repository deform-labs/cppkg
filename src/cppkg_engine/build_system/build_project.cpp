#include "../file_system/toml/parser/toml_parser.h"
#include "../package_system/package_system.h"
#include "../command_system/command_system.h"
#include "../misc/file_system/create_file.h"
#include "../compiler_wrapper/compiler.h"
#include "../misc/ux/color.h"
#include "build_project.h"
#include <filesystem>
#include <iostream>
#include <string>
#include <thread>
#include <atomic>
#include <chrono>

using namespace cppkg;
using namespace cppkg::ux;

command_system build_shell_;
toml_parser build_toml_;
namespace fs = std::filesystem;

/// Spinner animation lmao W UX
void spinner(const std::string& label, std::atomic<bool>& done) {
    const char frames[] = { '|', '/', '-', '\\' };
    int i = 0;
    while (!done) {
        std::cout << "\r" << color::cyan << frames[i++ % 4] << " " << label << color::reset << std::flush;
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    std::cout << "\r" << color::green << "[OK] " << label << color::reset << "    \n";
};

/// building by brick or by plaster?
static std::string detect_build_type(int argc, char* argv[]) {
    for (int i = 2; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--debug")   return "Debug";
        if (arg == "--release") return "Release";
    }
    return "RelWithDebInfo";
}

/// i have to for compatibility and for users whole just prefer cmake :|
void create_lists(fs::path project_dir, std::string name, std::string cpp_std, dependency_impl deps) {
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
}



void get_deps() {
    dependency_impl deps;
    std::cout << color::cyan << "Fetching dependencies..." << color::reset << "\n";
    if (!deps.fetch_all(".")) {
        std::cout << color::red << "Some dependencies failed to fetch" << color::reset << "\n";
    }
}

std::vector<std::string> get_source_files() {
    std::vector<std::string> source_files;
    for (const auto& entry : fs::recursive_directory_iterator("src")) {
        if (entry.path().extension() == ".cpp") {
            source_files.push_back(entry.path().string());
        }
    }

    if (source_files.empty()) {
        throw std::runtime_error("No source files found in src/");
    }

    return source_files;
}

CompilerWrapper::CompilerConfig get_compiler_config(const std::string& path) {
    auto toml = build_toml_.parse_toml(path + "/cppkg.toml");
    std::string name = toml.get("package", "name");
    std::string cpp_std = toml.get("package", "cpp_std");

    if (cpp_std.substr(0, 3) == "c++")
        cpp_std = cpp_std.substr(3);

    // finally it can NOT use cmake
    CompilerWrapper::CompilerConfig config;

    // compiler avenue 69
    if (toml.has("package", "compiler")) {
        config.compiler_path = toml.get("package", "compiler");
    } else {
        #ifdef _WIN32
            config.compiler_path = "cl";
        #else
            config.compiler_path = "g++";
        #endif
    }

    // include road 104
    if (toml.has("compiler", "IncludePaths")) {
        auto paths = toml.parse_array(toml.get("compiler", "IncludePaths"));
        for (const auto& Array_path : paths)
            config.include_paths.push_back(Array_path);
    }

    // flags
    if (toml.has("compiler", "compiler_flags")) {
        auto flags = toml.parse_array(toml.get("compiler", "Compiler_Flags"));
        for (const auto& flag : flags)
            config.default_flags.push_back(flag);
    }

    config.output_name = name;
    config.cpp_std = cpp_std;
    config.color_output = true;

    return config;
}

/// building it brick by brick
void Build::build_project(const std::string& path) {
    fs::path project_dir = fs::current_path() / path;
    fs::current_path(project_dir);

    // fetch dog fetch!
    get_deps();

    // source 2 pls valve i need this
    std::vector<std::string> source_files = get_source_files();

    // finally it can NOT use cmake
    CompilerWrapper::CompilerConfig config = get_compiler_config(path);

    CompilerWrapper compiler(config);

    // compile that ass
    int result = compiler.compile_all(source_files);
    if (result != 0) {
        throw std::runtime_error("Build failed!");
    }

    std::cout << color::green << "Build successful: " << config.output_name << color::reset << "\n";
}


/// well well well. What do we have here, an user in a hurry i see.
void Build::run_project(const std::string& path) {
    auto toml = build_toml_.parse_toml(path + "/cppkg.toml");
    std::string name = toml.get("package", "name");

    std::atomic<bool> done(false);

    std::thread t(spinner, "Running: ", std::ref(done));
    int r = build_shell_.run("target/build/" + name);
    done = true;
    t.join();

    if (r != 0) {
        std::cout << color::red << "Run failed!" << color::reset << "\n";
    } else {
        std::cout << color::green << "Run successful: " << name << color::reset << "\n";
    }
}
