#include "../command_system/command_system.h"
#include "../misc/ux/color.h"
#include <filesystem>
#include "compiler.h"
#include <algorithm>
#include <iostream>
#include <chrono>
#include <string>

using namespace cppkg;
command_system compiler_shell_;

/// W deepseek!
compiler::compiler() : config_() {
    build_base_command();
}

compiler::compiler(const config& cfg) : config_(cfg) {
    build_base_command();
}

void compiler::build_base_command() {
    bool msvc = is_msvc(config_.compiler_path);
    build_command_.clear();
    build_command_.push_back(config_.compiler_path);

    if (msvc) {
        // MSVC /std: flag mapping
        // Input may be "17", "20", "23", "c++17", "c++20", "c++23", "c++latest"
        std::string raw = config_.cpp_std;
        // Strip leading "c++" if present
        std::string ver = (raw.size() > 3 && raw.substr(0, 3) == "c++") ? raw.substr(3) : raw;
        std::string msvc_std;
        if (ver == "14") msvc_std = "/std:c++14";
        else if (ver == "17") msvc_std = "/std:c++17";
        else if (ver == "20") msvc_std = "/std:c++20";
        else if (ver == "23" || ver == "latest") msvc_std = "/std:c++latest";
        else msvc_std = "/std:c++17"; // safe fallback
        build_command_.push_back(msvc_std);
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

void compiler::add_flag(const std::string& flag) {
    build_command_.push_back(flag);
}

std::string compiler::get_file_hash(const std::string& filename) {
    std::string command;
   #ifdef _WIN32
        command = "certutil -hashfile " + quote(filename) + " MD5 | findstr /v \"MD5\"";
    #elif __APPLE__
        command = "md5 -q " + quote(filename);
    #else
        command = "md5sum " + quote(filename) + " | cut -d' ' -f1";
    #endif
    std::string result = compiler_shell_.run_with_output(command);
    if (result.empty()) return result;
    result.erase(result.find_last_not_of(" \n\r\t") + 1);
    return result;
}

bool compiler::check_cache(const std::string& source, const std::string& output) {
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

void compiler::store_cache(const std::string& source, const std::string& output) {
    if (config_.cache_dir.empty()) return;
    std::string source_hash = get_file_hash(source);
    std::string cache_file = config_.cache_dir + "/" + source_hash;
    std::error_code ec;
    std::filesystem::copy_file(output, cache_file,
        std::filesystem::copy_options::overwrite_existing, ec);
}

int compiler::compile(const std::string& source_file, const std::string& output_file) {
    if (!std::filesystem::exists(source_file)) {
        std::cerr << ux::color::red << "Error: Source file " << source_file
                  << " does not exist!" << ux::color::reset << std::endl;
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
        obj_file = config_.build_dir + "/objs/" + filename + (msvc ? ".obj" : ".o");
    }

    if (check_cache(source_file, obj_file)) return 0;

    std::cout << ux::color::yellow << "[Compiling] -> " << source_file << "..." << ux::color::reset << std::endl;

    std::string command_str;
    for (size_t i = 0; i < build_command_.size(); ++i) {
        if (i > 0) command_str += " ";
        command_str += build_command_[i];
    }

    if (msvc) {
        command_str += " /c " + source_file + " /Fo" + obj_file;
    } else {
        command_str += " -c " + source_file + " -o" + obj_file;
    }

    if (config_.verbose)
        std::cout << ux::color::cyan << "COMMAND: " << quote(command_str) << ux::color::reset << std::endl;

    int result = compiler_shell_.run_quiet(command_str);
    if (result != 0) {
        // Print compiler errors by re-running without suppression
        compiler_shell_.run(command_str);
    }

    store_cache(source_file, obj_file);

    return result;
}

/// gotta compile em all (⌐■_■)
int compiler::compile_all(const std::vector<std::string>& source_files) {
    bool msvc = is_msvc(config_.compiler_path);
    int total_errors = 0;
    std::vector<std::string> obj_files;

    std::filesystem::create_directories(config_.build_dir);
    auto start_time = std::chrono::steady_clock::now();

    // compile phase undertale ahh
    total_errors = cppkg::compiler::compile_sourcedir(*this, config_, source_files, msvc, obj_files);

    if (obj_files.empty()) {
        std::cerr << ux::color::red << "No object files to link" << ux::color::reset << std::endl;
        return total_errors > 0 ? total_errors : 1;
    }

    // link phase undertale ahh
    std::cout << ux::color::yellow << "[LINKER] Linking..." << ux::color::reset << std::endl;

    std::string link_cmd = msvc
        ? handle_msvc_linker(config_, obj_files)
        : handle_linux_macos_linker(config_, obj_files);

    if (config_.verbose)
        std::cout << ux::color::cyan << "LINK COMMAND: " << link_cmd << ux::color::reset << std::endl;

    int link_result = compiler_shell_.run_quiet(link_cmd);
    if (link_result != 0) {
        std::string err = compiler_shell_.run_with_output(link_cmd);
        std::cerr << ux::color::red << "Linking failed!\n" << err << ux::color::reset << std::endl;
        total_errors++;
    }

    auto end_time = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);

    std::cout << ux::color::cyan << "\nCompilation complete: " << ux::color::reset
              << (source_files.size() - total_errors) << " succeeded, "
              << total_errors << " failed in " << duration.count() << "ms" << std::endl;

    return total_errors;
}