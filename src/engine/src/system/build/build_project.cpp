#include "system/filesystem/toml/parser/toml_parser.h"
#include "system/package/package_system.h"
#include "system/command/command_system.h"
#include "misc/file_system/create_file.h"
#include "wrapper/compiler/compiler.h"
#include "misc/ux/color.h"
#include "build_project.h"
#include <filesystem>
#include <iostream>
#include <string>
#include <thread>
#include <atomic>
#include <algorithm>
#include <chrono>
#include <cstdlib>

using namespace cppkg;
using namespace cppkg::ux;

namespace fs = std::filesystem;

// File-scope singletons — one instance each, no shadowing
static CommandSystem build_shell_;
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
        throw std::runtime_error(
            "Dependency resolution failed; refusing to build against an unpinned revision");
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

/// Keys a table understands. Anything else is reported rather than silently ignored, which
/// is how "Compiler_Flags = [...]" used to do nothing at all.
static void warn_unknown_keys(const Toml& toml, const std::string& section,
                              const std::vector<std::string>& known) {
    if (!toml.has_section(section)) return;
    for (const auto& key : toml.keys(section)) {
        if (std::find(known.begin(), known.end(), key) != known.end()) continue;
        std::cerr << color::yellow << "[WARN] " << color::reset
                  << "cppkg.toml: unknown key \"" << key << "\" in [" << section
                  << "], ignored\n";
    }
}

static Compiler::Config get_compiler_config(const fs::path& project_dir) {
    auto toml = build_toml_.parse_toml((project_dir / "cppkg.toml").string());
    warn_unknown_keys(toml, "package",
        {"name", "version", "description", "authors", "license", "cpp_std", "compiler"});
    warn_unknown_keys(toml, "compiler",
        {"IncludePaths", "LibraryPaths", "Libraries", "compiler_flags"});

    std::string name    = toml.get("package", "name");
    // Passed through as written; Compiler::StdVersion normalises "c++20"/"20"/"latest".
    std::string cpp_std = toml.get_or("package", "cpp_std", "c++20");

    Compiler::Config config;

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

    // Include paths. array() hands back parsed elements, so a quoted entry may contain a
    // comma or a '#' without confusing the reader.
    for (const auto& p : toml.array("compiler", "IncludePaths"))
        config.include_paths.push_back(p);

    // Library search paths and library names, e.g.
    //   [compiler]
    //   LibraryPaths = ["vendor/lib"]
    //   Libraries = ["fmt"]
    for (const auto& p : toml.array("compiler", "LibraryPaths"))
        config.library_paths.push_back(p);
    for (const auto& lib : toml.array("compiler", "Libraries"))
        config.libraries.push_back(lib);

    // Fetched dependencies live in target/deps/<repo>. Put the header roots they actually
    // ship on the include path so `cppkg add fmtlib/fmt@...` makes `#include <fmt/core.h>`
    // resolve, and the usual lib/ and build/ dirs on the link path so a dependency that
    // was compiled in place can be linked with Libraries = [...].
    dependency_impl deps;
    for (const auto& dep : deps.load_dependencies(project_dir.string())) {
        fs::path dep_root = project_dir / "target" / "deps" / dep.repo;
        for (const auto& dir : { dep_root, dep_root / "include", dep_root / "src" }) {
            if (fs::is_directory(dir))
                config.include_paths.push_back(dir.string());
        }
        for (const auto& dir : { dep_root / "lib", dep_root / "build" }) {
            if (fs::is_directory(dir))
                config.library_paths.push_back(dir.string());
        }
    }

    // Extra compiler flags, appended to the defaults.
    for (const auto& flag : toml.array("compiler", "compiler_flags"))
        config.default_flags.push_back(flag);

    // Objects mirror their path below src/, so two sources that share a file name no longer
    // compile onto one another's object.
    config.source_root   = (project_dir / "src").string();
    config.project_dir   = project_dir.string();

    // CPPKG_JOBS=1 forces a serial build; unset or 0 uses one thread per hardware thread.
    if (const char* jobs = std::getenv("CPPKG_JOBS"))
        config.jobs = std::atoi(jobs);

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
    std::string cpp_std = toml.get_or("package", "cpp_std", "c++20");

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
    // CMAKE_CXX_STANDARD takes a bare version ("20"), never the manifest's "c++20".
    cmake += "set(CMAKE_CXX_STANDARD " + Compiler::StdVersion(cpp_std) + ")\n";
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
    // Canonical, so "build ." does not leak "/./" into object paths or compile_commands.json.
    fs::path project_dir = fs::weakly_canonical(path);

    get_deps(project_dir);

    std::vector<std::string> source_files = get_source_files(project_dir);

    Compiler::Config config = get_compiler_config(project_dir);
    Compiler compiler_(config);

    // Record the exact flags for clangd and other tooling. Written on every build, including
    // ones where every object is already up to date, so the database never goes stale.
    if (!compiler_.WriteCompileCommands(source_files, (project_dir / "compile_commands.json").string())) {
        std::cerr << color::yellow << "[WARN] " << color::reset
                  << "could not write compile_commands.json\n";
    }

    int result = compiler_.CompileAll(source_files);
    if (result != 0) {
        throw std::runtime_error("Build failed!");
    }

    std::cout << color::green << "Build successful: " << config.output_name << color::reset << "\n";
}

// ---------------------------------------------------------------------------
// build::run_project
// ---------------------------------------------------------------------------

void build::run_project(const std::string& path) {
    fs::path project_dir = fs::weakly_canonical(path);

    Compiler::Config config = get_compiler_config(project_dir);

#ifdef _WIN32
    std::string run_path = (project_dir / config.build_dir / (config.output_name + ".exe")).string();
#else
    std::string run_path = (project_dir / config.build_dir / config.output_name).string();
#endif

    std::cout << color::yellow << "Running: " << run_path << color::reset << "\n";

    // Start spinner after printing the path so the label is visible
    std::atomic<bool> done(false);
    std::thread t(spinner, "Running " + config.output_name, std::ref(done));

    int r = build_shell_.Run(run_path);

    done = true;
    t.join();

    if (r != 0) {
        std::cout << color::red << "Run failed!" << color::reset << "\n";
    } else {
        std::cout << color::green << "Run successful: " << config.output_name << color::reset << "\n";
    }
}