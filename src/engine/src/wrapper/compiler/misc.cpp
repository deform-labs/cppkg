#include "compiler.h"
#include "misc/ux/color.h"  // or wherever it lives
#include <iostream>

/// compile the source code of the package
int cppkg::Compiler::CompileSourceDirectory(Compiler& wrapper, const Compiler::Config& config,
                              const std::vector<std::string>& source_files, bool msvc,
                              std::vector<std::string>& obj_files) {
    int total_errors = 0;
    int compiled = 0;

    for (const auto& source : source_files) {
        if (config.verbose) {
            std::cout << cppkg::ux::color::cyan << "[" << (compiled + 1) << "/" << source_files.size() << "]"
                      << " Processing " << source << "..." << cppkg::ux::color::reset << std::endl;
        }

        std::string filename = source;
        auto slash = filename.find_last_of("/\\");
        if (slash != std::string::npos) filename = filename.substr(slash + 1);
        auto dot = filename.find_last_of('.');
        if (dot != std::string::npos) filename = filename.substr(0, dot);
        std::filesystem::create_directories(config.build_dir + "/objs");

        std::string obj = config.build_dir + "/objs/" + filename + (msvc ? ".obj" : ".o");
        int result = wrapper.Compile(source, obj);

        if (result != 0) {
            total_errors++;
            if (config.verbose)
                std::cerr << cppkg::ux::color::red << "Failed to compile -> " << source << cppkg::ux::color::reset << std::endl;
        } else {
            obj_files.push_back(obj);
        }
        compiled++;
    }
    return total_errors;
}

/// wrap a string in quotes :)
std::string cppkg::Compiler::Quote(const std::string& s) {
    return "\"" + s + "\"";
}

/// check if the user is using msvc or just has msvc in the path
bool cppkg::Compiler::IsMSVC(const std::string& compiler_path) {
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

/// translate a flag to its msvc equivalent if necessary
std::string cppkg::Compiler::TranslateFlag(const std::string& flag, bool msvc) {
    if (!msvc) return flag;
    if (flag == "-Wall" || flag == "-Wextra") return "/W4";
    if (flag == "-O2")  return "/O2";
    if (flag == "-O0")  return "/Od";
    if (flag == "-g")   return "/Zi";
    if (flag == "-c")   return "/c";
    if (flag.substr(0, 5) == "-std=") return "/std:" + flag.substr(5);
    return flag;
}

/// handle the linker for windows users
std::string cppkg::Compiler::HandleMSVCLinker(const cppkg::Compiler::Config& config, const std::vector<std::string>& obj_files) {
    std::string out = config.build_dir + "/" + (config.output_name.empty() ? "program" : config.output_name) + ".exe";
    std::string link_cmd = "link /nologo /OUT:" + Quote(out);
    for (const auto& path : config.library_paths)
        link_cmd += " /LIBPATH:" + Quote(path);
    for (const auto& lib : config.libraries)
        link_cmd += " " + lib + ".lib";
    for (const auto& obj : obj_files)
        link_cmd += " " + Quote(obj);
    return link_cmd;
}

/// handle the linker for other platforms such as linux & macos
std::string cppkg::Compiler::HandleLinuxMacOSLinker(const cppkg::Compiler::Config& config, const std::vector<std::string>& obj_files) {
    std::string out = config.build_dir + "/" + (config.output_name.empty() ? "program" : config.output_name);
    std::string link_cmd = Quote(config.compiler_path);
    for (const auto& obj : obj_files)
        link_cmd += " " + Quote(obj);
    for (const auto& path : config.library_paths)
        link_cmd += " -L" + Quote(path);
    for (const auto& lib : config.libraries)
        link_cmd += " -l" + lib;
    link_cmd += " -o " + Quote(out);
    return link_cmd;
}