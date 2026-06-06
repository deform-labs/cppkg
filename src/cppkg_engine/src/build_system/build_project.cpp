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

namespace fs = std::filesystem;

// File-scope singletons — one instance each, no shadowing
static command_system build_shell_;
static toml_parser    build_toml_;

// ---------------------------------------------------------------------------
// Spinner
// ---------------------------------------------------------------------------

void spinner(const std::string& label, std::atomic<bool>& done) {
    const char frames[] = { '|', '/', '-', '\\' };
    int i = 0;
    while (!done) {
        std::cout << "\r" << color::cyan << frames[i++ % 4] << " " << label << color::reset << std::flush;
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    // Clear the spinner line, then print the done message
    std::cout << "\r                                    \r"
              << color::green << "[OK] " << label << color::reset << "\n";
}

// ---------------------------------------------------------------------------
// Internal helpers (all take an explicit project path)
// ---------------------------------------------------------------------------

static void get_deps(const fs::path& project_dir) {
    dependency_impl deps;
    std::cout << color::cyan << "Fetching dependencies..." << color::reset << "\n";
    if (!deps.fetch_all(project_dir.string())) {
        std::cout << color::red << "Some dependencies failed to fetch" << color::reset << "\n";
    }
}

static std::vector<std::string> get_source_files(const fs::path& project_dir) {
    std::vector<std::string> source_files;
    for (const auto& entry : fs::recursive_directory_iterator(project_dir / "src")) {
        if (entry.path().extension() == ".cpp") {
            source_files.push_back(entry.path().string());
        }
    }
    if (source_files.empty()) {
        throw std::runtime_error("No source files found in src/");
    }
    return source_files;
}

static compiler::config get_compiler_config(const fs::path& project_dir) {
    auto toml = build_toml_.parse_toml((project_dir / "cppkg.toml").string());
    std::string name    = toml.get("package", "name");
    std::string cpp_std = toml.get("package", "cpp_std");

    // Strip leading "c++" prefix if present ("c++17" -> "17")
    if (cpp_std.size() > 3 && cpp_std.substr(0, 3) == "c++")
        cpp_std = cpp_std.substr(3);

    compiler::config config;

    // Compiler executable
    if (toml.has("package", "compiler")) {
        config.compiler_path = toml.get("package", "compiler");
    } else {
#ifdef _WIN32
        config.compiler_path = "cl";
#else
        config.compiler_path = "g++";
#endif
    }

    // Include paths
    if (toml.has("compiler", "IncludePaths")) {
        auto paths = toml.parse_array(toml.get("compiler", "IncludePaths"));
        for (const auto& p : paths)
            config.include_paths.push_back(p);
    }

    // Compiler flags — fixed: key name was inconsistent ("compiler_flags" vs "Compiler_Flags")
    if (toml.has("compiler", "compiler_flags")) {
        auto flags = toml.parse_array(toml.get("compiler", "compiler_flags"));
        for (const auto& flag : flags)
            config.default_flags.push_back(flag);
    }

    config.output_name   = name;
    config.cpp_std       = cpp_std;
    config.color_output  = true;

    return config;
}

// ---------------------------------------------------------------------------
// build::create_lists
// ---------------------------------------------------------------------------

void build::create_lists(fs::path project_dir) {
    if (fs::exists(project_dir / "CMakeLists.txt")) return;
    if (!fs::exists(project_dir / "cppkg.toml")) {
        std::cerr << color::red << "[ERROR] " << color::reset << "cppkg.toml not found\n";
        return;
    }

    auto toml = build_toml_.parse_toml((project_dir / "cppkg.toml").string());
    std::string name    = toml.get("package", "name");
    std::string cpp_std = toml.get("package", "cpp_std");

    // Collect source files explicitly to avoid GLOB_RECURSE stale-cache issues
    std::vector<std::string> sources;
    if (fs::exists(project_dir / "src")) {
        for (const auto& entry : fs::recursive_directory_iterator(project_dir / "src")) {
            if (entry.path().extension() == ".cpp") {
                // Store relative path for portability
                sources.push_back(
                    fs::relative(entry.path(), project_dir).generic_string()
                );
            }
        }
    }

    dependency_impl deps;
    auto dependencies = deps.load_dependencies(project_dir.string());

    std::string cmake;
    cmake += "cmake_minimum_required(VERSION 3.10)\n";
    cmake += "project(" + name + ")\n\n";
    cmake += "set(CMAKE_CXX_STANDARD " + cpp_std + ")\n";
    cmake += "set(CMAKE_CXX_STANDARD_REQUIRED True)\n\n";

    // Explicit source list instead of GLOB_RECURSE
    cmake += "set(SOURCES\n";
    for (const auto& src : sources)
        cmake += "    " + src + "\n";
    cmake += ")\n\n";

    for (const auto& dep : dependencies)
        cmake += "add_subdirectory(target/deps/" + dep.repo + ")\n";
    if (!dependencies.empty()) cmake += "\n";

    cmake += "add_executable(" + name + " ${SOURCES})\n";
    cmake += "\ntarget_link_libraries(" + name + "\n";
    for (const auto& dep : dependencies)
        cmake += "    " + dep.repo + "\n";
    cmake += ")\n";

    create_file("CMakeLists.txt", project_dir.string(), cmake);
    std::cout << color::cyan << "CMakeLists.txt created in " << project_dir.string() << color::reset << "\n";
}

// ---------------------------------------------------------------------------
// build::build_project
// ---------------------------------------------------------------------------

void build::build_project(const std::string& path) {
    fs::path project_dir = fs::absolute(path);

    get_deps(project_dir);

    std::vector<std::string> source_files = get_source_files(project_dir);

    compiler::config config = get_compiler_config(project_dir);
    compiler compiler_(config);

    int result = compiler_.compile_all(source_files);
    if (result != 0) {
        throw std::runtime_error("Build failed!");
    }

    std::cout << color::green << "Build successful: " << config.output_name << color::reset << "\n";
}

// ---------------------------------------------------------------------------
// build::run_project
// ---------------------------------------------------------------------------

void build::run_project(const std::string& path) {
    fs::path project_dir = fs::absolute(path);

    compiler::config config = get_compiler_config(project_dir);

#ifdef _WIN32
    std::string run_path = (project_dir / config.build_dir / (config.output_name + ".exe")).string();
#else
    std::string run_path = (project_dir / config.build_dir / config.output_name).string();
#endif

    std::cout << color::yellow << "Running: " << run_path << color::reset << "\n";

    // Start spinner after printing the path so the label is visible
    std::atomic<bool> done(false);
    std::thread t(spinner, "Running " + config.output_name, std::ref(done));

    int r = build_shell_.run(run_path);

    done = true;
    t.join();

    if (r != 0) {
        std::cout << color::red << "Run failed!" << color::reset << "\n";
    } else {
        std::cout << color::green << "Run successful: " << config.output_name << color::reset << "\n";
    }
}