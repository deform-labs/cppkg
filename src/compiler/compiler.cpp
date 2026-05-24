#include "../helpers/the_shell.h"
#include "../helpers/color.h"
#include "compiler.h"
#include <filesystem>
#include <algorithm>
#include <iostream>
#include <cstdlib>
#include <chrono>
#include <string>

static bool is_msvc(const std::string& compiler_path) {
    std::string p = compiler_path;
    for (auto& c : p) c = (char)tolower(c);
    return (
        p == "cl" ||
        p == "cl.exe" ||
        (
            p.find("cl") != std::string::npos &&
            p.find("g++") == std::string::npos &&
            p.find("clang") == std::string::npos
        )
    );
}

static std::string quote(const std::string& s) {
    return "\"" + s + "\"";
}

static std::string translate_flag(const std::string& flag, bool msvc) {
    if (!msvc) return flag;
    if (flag == "-Wall" || flag == "-Wextra") return "/W4";
    if (flag == "-O2")  return "/O2";
    if (flag == "-O0")  return "/Od";
    if (flag == "-g")   return "/Zi";
    if (flag == "-c")   return "/c";
    if (flag.substr(0, 5) == "-std=") return "/std:" + flag.substr(5);
    return flag;
}

CompilerWrapper::CompilerWrapper(const CompilerConfig& config)
    : config_(config) {
    build_base_command();
}

void CompilerWrapper::build_base_command() {
    bool msvc = is_msvc(config_.compiler_path);
    build_command_.clear();
    build_command_.push_back(quote(config_.compiler_path));

    if (msvc) {
        build_command_.push_back("/std:c++17");
        build_command_.push_back("/EHsc");
        build_command_.push_back("/nologo");
    } else {
        build_command_.push_back("-std=" + config_.cpp_std);
    }

    std::vector<std::string> seen;
    for (const auto& flag : config_.default_flags) {
        std::string translated = translate_flag(flag, msvc);
        if (std::find(seen.begin(), seen.end(), translated) == seen.end()) {
            build_command_.push_back(translated);
            seen.push_back(translated);
        }
    }

    for (const auto& path : config_.include_paths) {
        if (msvc) build_command_.push_back("/I" + quote(path));
        else      build_command_.push_back("-I" + path);
    }

    if (!msvc) {
        for (const auto& path : config_.library_paths)
            build_command_.push_back("-L" + path);
        for (const auto& lib : config_.libraries)
            build_command_.push_back("-l" + lib);
    }
}

void CompilerWrapper::add_flag(const std::string& flag) {
    build_command_.push_back(flag);
}

std::string CompilerWrapper::get_file_hash(const std::string& filename) {
    std::string command;
    #ifdef _WIN32
        command = "certutil -hashfile " + quote(filename) + " MD5 | findstr /v \"MD5\"";
    #else
        command = "md5sum " + quote(filename) + " | cut -d' ' -f1";
    #endif
    std::string result = shell_.run_with_output(command);
    if (result.empty()) return result;
    result.erase(result.find_last_not_of(" \n\r\t") + 1);
    return result;
}

bool CompilerWrapper::check_cache(const std::string& source, const std::string& output) {
    if (config_.cache_dir.empty()) return false;
    std::filesystem::create_directories(config_.cache_dir);
    std::string source_hash = get_file_hash(source);
    std::string cache_file = config_.cache_dir + "/" + source_hash;
    if (std::filesystem::exists(cache_file) && std::filesystem::exists(output)) {
        auto cache_time  = std::filesystem::last_write_time(cache_file);
        auto source_time = std::filesystem::last_write_time(source);
        if (cache_time > source_time) {
            if (config_.verbose)
                std::cout << "Cache hit for " << source << std::endl;
            return true;
        }
    }
    return false;
}

void CompilerWrapper::store_cache(const std::string& source, const std::string& output) {
    if (config_.cache_dir.empty()) return;
    std::string source_hash = get_file_hash(source);
    std::string cache_file = config_.cache_dir + "/" + source_hash;
    std::error_code ec;
    std::filesystem::copy_file(output, cache_file,
        std::filesystem::copy_options::overwrite_existing, ec);
}

int CompilerWrapper::compile(const std::string& source_file, const std::string& output_file) {
    if (!std::filesystem::exists(source_file)) {
        std::cerr << Color::red << "Error: Source file " << source_file
                  << " does not exist!" << Color::reset << std::endl;
        return 1;
    }

    bool msvc = is_msvc(config_.compiler_path);
    std::string obj_file = output_file;

    if (obj_file.empty()) {
        std::string filename = source_file;
        auto slash = filename.find_last_of("/\\");
        if (slash != std::string::npos) filename = filename.substr(slash + 1);
        auto dot = filename.find_last_of('.');
        if (dot != std::string::npos) filename = filename.substr(0, dot);
        obj_file = config_.build_dir + "/" + filename + (msvc ? ".obj" : ".o");
    }

    if (check_cache(source_file, obj_file)) return 0;

    std::cout << Color::yellow << "[Compiling] -> " << source_file << "..." << Color::reset << std::endl;

    std::string command_str;
    for (const auto& part : build_command_)
        command_str += " " + part;

    if (msvc) {
        command_str += " /c " + source_file + " /Fo" + obj_file;
    } else {
        command_str += " -c " + source_file + " -o " + obj_file;
    }

    if (config_.verbose)
        std::cout << Color::cyan << "COMMAND: " << quote(command_str) << Color::reset << std::endl;

    int result = shell_.run_quiet(command_str);
    if (result == 0 && !config_.cache_dir.empty())
        store_cache(source_file, obj_file);

    return result;
}

/// windows linker
static std::string handle_msvc_linker(const CompilerWrapper::CompilerConfig& config, const std::vector<std::string>& obj_files) {
    std::string out = config.build_dir + "/" + (config.output_name.empty() ? "program" : config.output_name) + ".exe";
    std::string link_cmd = "link /nologo /OUT:" + quote(out);
    for (const auto& lib : config.libraries)
        link_cmd += " " + lib + ".lib";
    for (const auto& obj : obj_files)
        link_cmd += " " + quote(obj);
    return link_cmd;
}

/// linux/macos linker
static std::string handle_linux_macos_linker(const CompilerWrapper::CompilerConfig& config, const std::vector<std::string>& obj_files) {
    std::string out = config.build_dir + "/" + (config.output_name.empty() ? "program" : config.output_name);
    std::string link_cmd = quote(config.compiler_path);
    for (const auto& obj : obj_files)
        link_cmd += " " + quote(obj);
    for (const auto& path : config.library_paths)
        link_cmd += " -L" + quote(path);
    for (const auto& lib : config.libraries)
        link_cmd += " -l" + lib;
    link_cmd += " -o " + quote(out);
    return link_cmd;
}

/// compile all source files and return obj list + error count
static int compile_sourcedir(CompilerWrapper& wrapper, const CompilerWrapper::CompilerConfig& config,
                              const std::vector<std::string>& source_files, bool msvc,
                              std::vector<std::string>& obj_files) {
    int total_errors = 0;
    int compiled = 0;

    for (const auto& source : source_files) {
        if (config.verbose) {
            std::cout << Color::cyan << "[" << (compiled + 1) << "/" << source_files.size() << "]"
                      << " Processing " << source << "..." << Color::reset << std::endl;
        }

        std::string filename = source;
        auto slash = filename.find_last_of("/\\");
        if (slash != std::string::npos) filename = filename.substr(slash + 1);
        auto dot = filename.find_last_of('.');
        if (dot != std::string::npos) filename = filename.substr(0, dot);

        std::string obj = config.build_dir + "/" + filename + (msvc ? ".obj" : ".o");
        int result = wrapper.compile(source, obj);

        if (result != 0) {
            total_errors++;
            if (config.verbose)
                std::cerr << Color::red << "Failed to compile -> " << source << Color::reset << std::endl;
        } else {
            obj_files.push_back(obj);
        }
        compiled++;
    }
    return total_errors;
}

/// gotta compile em all (⌐■_■)
int CompilerWrapper::compile_all(const std::vector<std::string>& source_files) {
    bool msvc = is_msvc(config_.compiler_path);
    int total_errors = 0;
    std::vector<std::string> obj_files;

    std::filesystem::create_directories(config_.build_dir);
    auto start_time = std::chrono::steady_clock::now();

    // compile phase undertale ahh
    total_errors = compile_sourcedir(*this, config_, source_files, msvc, obj_files);

    if (obj_files.empty()) {
        std::cerr << Color::red << "No object files to link" << Color::reset << std::endl;
        return total_errors > 0 ? total_errors : 1;
    }

    // link phase undertale ahh
    std::cout << Color::yellow << "[LINKER] Linking..." << Color::reset << std::endl;

    std::string link_cmd = msvc
        ? handle_msvc_linker(config_, obj_files)
        : handle_linux_macos_linker(config_, obj_files);

    if (config_.verbose)
        std::cout << Color::cyan << "LINK COMMAND: " << link_cmd << Color::reset << std::endl;

    int link_result = shell_.run_quiet(link_cmd);
    if (link_result != 0) {
        std::string err = shell_.run_with_output(link_cmd);
        std::cerr << Color::red << "Linking failed!\n" << err << Color::reset << std::endl;
        total_errors++;
    }

    auto end_time = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);

    std::cout << Color::cyan << "\nCompilation complete: " << Color::reset
              << (source_files.size() - total_errors) << " succeeded, "
              << total_errors << " failed in " << duration.count() << "ms" << std::endl;

    return total_errors;
}