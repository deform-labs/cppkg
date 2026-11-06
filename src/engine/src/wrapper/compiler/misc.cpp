#include "compiler.h"
#include "misc/ux/color.h"  // or wherever it lives
#include <atomic>
#include <iostream>
#include <sstream>
#include <thread>

/// compile the source code of the package
int cppkg::Compiler::CompileSourceDirectory(Compiler& wrapper, const Compiler::Config& config,
                              const std::vector<std::string>& source_files,
                              std::vector<std::string>& obj_files) {
    const size_t total = source_files.size();
    if (total == 0) return 0;

    // One worker per hardware thread unless the caller asked for a specific count.
    const unsigned hardware = std::thread::hardware_concurrency();
    size_t workers = config.jobs > 0 ? static_cast<size_t>(config.jobs)
                                     : static_cast<size_t>(hardware ? hardware : 1);
    if (workers > total) workers = total;

    std::atomic<size_t> next{0};
    std::atomic<int> errors{0};

    std::vector<std::string> objects(total);
    std::vector<char> built(total, 0);

    auto worker = [&]() {
        for (;;) {
            const size_t index = next.fetch_add(1);
            if (index >= total) return;
            const std::string& source = source_files[index];

            if (config.verbose) {
                std::ostringstream line;
                line << cppkg::ux::color::cyan << "[" << (index + 1) << "/" << total << "]"
                     << " Processing " << source << "..." << cppkg::ux::color::reset;
                LogLine(line.str());
            }

            const std::string object = wrapper.ObjectFileFor(source);
            std::error_code ec;
            std::filesystem::create_directories(std::filesystem::path(object).parent_path(), ec);

            const int result = wrapper.Compile(source, object);

            if (result != 0) {
                errors.fetch_add(1);
                if (config.verbose) {
                    std::ostringstream line;
                    line << cppkg::ux::color::red << "Failed to compile -> " << source
                         << cppkg::ux::color::reset;
                    LogLine(line.str());
                }
            } else {
                objects[index] = object;
                built[index] = 1;
            }
        }
    };

    std::vector<std::thread> pool;
    pool.reserve(workers);
    for (size_t i = 0; i < workers; ++i) pool.emplace_back(worker);
    for (auto& thread : pool) thread.join();

    // Build the object list in source order so the link command stays deterministic.
    for (size_t i = 0; i < total; ++i)
        if (built[i]) obj_files.push_back(objects[i]);

    return errors.load();
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
