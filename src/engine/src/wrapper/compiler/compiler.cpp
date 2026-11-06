#include "../../system/command/command_system.h"
#include "misc/ux/color.h"
#include <filesystem>
#include "compiler.h"
#include <algorithm>
#include <iostream>
#include <chrono>
#include <string>

namespace cppkg {
    CommandSystem CompilerShell;

    /// W deepseek!
    Compiler::Compiler() : config_() {
        BuildBaseCommand();
    }

    Compiler::Compiler(const Config& cfg) : config_(cfg) {
        BuildBaseCommand();
    }

    std::string Compiler::StdVersion(const std::string& cpp_std) {
        std::string ver = cpp_std;
        if (ver.size() > 3 && (ver.compare(0, 3, "c++") == 0 || ver.compare(0, 3, "C++") == 0))
            ver = ver.substr(3);
        if (ver == "latest") ver = "23";  // no portable spelling; pin to the newest we know
        if (ver.empty()) ver = "20";
        return ver;
    }

    void Compiler::BuildBaseCommand() {
        bool msvc = IsMSVC(config_.compiler_path);
        build_command_.clear();
        build_command_.push_back(config_.compiler_path);

        // Accept "c++20", "20" or "latest" from the manifest; reduce once to a bare version.
        const std::string ver = StdVersion(config_.cpp_std);

        if (msvc) {
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
            // GCC and Clang want the full spelling: -std=c++20, never -std=20.
            build_command_.push_back("-std=c++" + ver);
        }

        std::vector<std::string> seen;
        for (const auto& flag : config_.default_flags) {
            std::string translated = TranslateFlag(flag, msvc);
            if (std::find(seen.begin(), seen.end(), translated) == seen.end()) {
                build_command_.push_back(translated);
                seen.push_back(translated);
            }
        }

        for (const auto& path : config_.include_paths) {
            if (msvc) build_command_.push_back("/I" + Quote(path));
            else      build_command_.push_back("-I" + path);
        }

        if (!msvc) {
            for (const auto& path : config_.library_paths)
                build_command_.push_back("-L" + path);
            for (const auto& lib : config_.libraries)
                build_command_.push_back("-l" + lib);
        }
    }

    void Compiler::AddFlag(const std::string& flag) {
        build_command_.push_back(flag);
    }

    std::string Compiler::GetFileHash(const std::string& filename) {
        std::string command;
       #ifdef _WIN32
            command = "certutil -hashfile " + Quote(filename) + " MD5 | findstr /v \"MD5\"";
        #elif __APPLE__
            command = "md5 -q " + Quote(filename);
        #else
            command = "md5sum " + Quote(filename) + " | cut -d' ' -f1";
        #endif
        std::string result = CompilerShell.RunWithOutput(command);
        if (result.empty()) return result;
        result.erase(result.find_last_not_of(" \n\r\t") + 1);
        return result;
    }

    bool Compiler::CheckCache(const std::string& source, const std::string& output) {
        if (config_.cache_dir.empty()) return false;
        std::filesystem::create_directories(config_.cache_dir);
        std::string SourceHash = GetFileHash(source);
        std::string CacheFile = config_.cache_dir + "/" + SourceHash;
        if (std::filesystem::exists(CacheFile) && std::filesystem::exists(output)) {
            auto CacheTime  = std::filesystem::last_write_time(CacheFile);
            auto SourceTime = std::filesystem::last_write_time(source);
            if (CacheTime > SourceTime) {
                if (config_.verbose)
                    std::cout << "Cache hit for " << source << std::endl;
                return true;
            }
        }
        return false;
    }

    void Compiler::StoreCache(const std::string& source, const std::string& output) {
        if (config_.cache_dir.empty()) return;
        std::string SourceHash = GetFileHash(source);
        std::string CacheFile = config_.cache_dir + "/" + SourceHash;
        std::error_code ErrorCode;
        std::filesystem::copy_file(output, CacheFile,
            std::filesystem::copy_options::overwrite_existing, ErrorCode);
    }

    int Compiler::Compile(const std::string& source_file, const std::string& output_file) {
        if (!std::filesystem::exists(source_file)) {
            std::cerr << ux::color::red << "Error: Source file " << source_file
                      << " does not exist!" << ux::color::reset << std::endl;
            return 1;
        }

        bool msvc = IsMSVC(config_.compiler_path);
        std::string ObjectFile = output_file;

        if (ObjectFile.empty()) {
            std::string filename = source_file;
            auto slash = filename.find_last_of("/\\");
            if (slash != std::string::npos) filename = filename.substr(slash + 1);
            auto dot = filename.find_last_of('.');
            if (dot != std::string::npos) filename = filename.substr(0, dot);
            ObjectFile = config_.build_dir + "/objs/" + filename + (msvc ? ".obj" : ".o");
        }

        if (CheckCache(source_file, ObjectFile)) return 0;

        std::cout << ux::color::yellow << "[Compiling] -> " << source_file << "..." << ux::color::reset << std::endl;

        std::string CommandString;
        for (size_t i = 0; i < build_command_.size(); ++i) {
            if (i > 0) CommandString += " ";
            CommandString += build_command_[i];
        }

        if (msvc) {
            CommandString += " /c " + source_file + " /Fo" + ObjectFile;
        } else {
            CommandString += " -c " + source_file + " -o" + ObjectFile;
        }

        if (config_.verbose)
            std::cout << ux::color::cyan << "COMMAND: " << Quote(CommandString) << ux::color::reset << std::endl;

        int result = CompilerShell.RunQuiet(CommandString);
        if (result != 0) {
            // Print compiler errors by re-running without suppression
            CompilerShell.Run(CommandString);
        }

        StoreCache(source_file, ObjectFile);

        return result;
    }

    /// Compile all source files in a directory and link them into a single executable
    int Compiler::CompileAll(const std::vector<std::string>& source_files) {
        bool msvc = IsMSVC(config_.compiler_path);
        int TotalErrors = 0;
        std::vector<std::string> ObjectFiles;

        std::filesystem::create_directories(config_.build_dir);
        auto start_time = std::chrono::steady_clock::now();

        // Compile each source file into an object file
        TotalErrors = Compiler::CompileSourceDirectory(*this, config_, source_files, msvc, ObjectFiles  );

        if (ObjectFiles.empty()) {
            std::cerr << ux::color::red << "No object files to link" << ux::color::reset << std::endl;
            return TotalErrors > 0 ? TotalErrors : 1;
        }

        // Link all object files into a single executable
        std::cout << ux::color::yellow << "[LINKER] Linking..." << ux::color::reset << std::endl;

        std::string LinkCMD = msvc
            ? HandleMSVCLinker(config_, ObjectFiles)
            : HandleLinuxMacOSLinker(config_, ObjectFiles);

        if (config_.verbose)
            std::cout << ux::color::cyan << "LINK COMMAND: " << LinkCMD << ux::color::reset << std::endl;

        int link_result = CompilerShell.RunQuiet(LinkCMD);
        if (link_result != 0) {
            std::string err = CompilerShell.RunWithOutput(LinkCMD);
            std::cerr << ux::color::red << "Linking failed!\n" << err << ux::color::reset << std::endl;
            TotalErrors++;
        }

        auto end_time = std::chrono::steady_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);

        std::cout << ux::color::cyan << "\nCompilation complete: " << ux::color::reset
                  << (source_files.size() - TotalErrors) << " succeeded, "
                  << TotalErrors << " failed in " << duration.count() << "ms" << std::endl;

        return TotalErrors;
    }
}