#include "../../system/command/command_system.h"
#include "misc/ux/color.h"
#include <filesystem>
#include "compiler.h"
#include <algorithm>
#include <iostream>
#include <chrono>
#include <cstdio>
#include <fstream>
#include <iterator>
#include <mutex>
#include <sstream>
#include <string>

namespace cppkg {
    CommandSystem CompilerShell;

    namespace {
        /// Compile threads share stdout, so each progress line is printed as a unit.
        std::mutex g_log_mutex;

        /// FNV-1a: stable across runs, unlike std::hash which implementations may salt.
        std::string HashString(const std::string& text) {
            unsigned long long hash = 1469598103934665603ULL;
            for (unsigned char c : text) {
                hash ^= c;
                hash *= 1099511628211ULL;
            }
            char buffer[17];
            std::snprintf(buffer, sizeof(buffer), "%016llx", hash);
            return std::string(buffer);
        }
    }

    void Compiler::LogLine(const std::string& line) {
        std::lock_guard<std::mutex> guard(g_log_mutex);
        std::cout << line << std::endl;
    }

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

    /// Objects mirror their path below src/: src/net/util.cpp becomes objs/net/util.o, so two
    /// sources that happen to share a file name can no longer compile onto the same object.
    std::string Compiler::ObjectFileFor(const std::string& source_file) {
        std::filesystem::path source(source_file);
        std::string relative = source.filename().string();

        if (!config_.source_root.empty()) {
            std::error_code ec;
            std::filesystem::path root = std::filesystem::weakly_canonical(config_.source_root, ec);
            if (!ec) {
                std::filesystem::path full = std::filesystem::weakly_canonical(source, ec);
                if (!ec) {
                    std::filesystem::path rel = std::filesystem::relative(full, root, ec);
                    if (!ec && !rel.empty() && rel.native().compare(0, 2, "..") != 0)
                        relative = rel.generic_string();
                }
            }
        }

        std::filesystem::path object(relative);
        object.replace_extension(IsMSVC(config_.compiler_path) ? ".obj" : ".o");
        return (std::filesystem::path(config_.build_dir) / "objs" / object).string();
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
            // Quoted: an include root may contain spaces ("include dir", "Program Files").
            if (msvc) build_command_.push_back("/I" + Quote(path));
            else      build_command_.push_back("-I" + Quote(path));
        }

        if (!msvc) {
            for (const auto& path : config_.library_paths)
                build_command_.push_back("-L" + Quote(path));
            for (const auto& lib : config_.libraries)
                build_command_.push_back("-l" + lib);
        }
    }

    void Compiler::AddFlag(const std::string& flag) {
        build_command_.push_back(flag);
    }

    std::string Compiler::CommandSignature() const {
        std::string joined;
        for (const auto& part : build_command_) {
            joined += part;
            joined += '\n';
        }
        return HashString(joined);
    }

    void Compiler::WriteSignature(const std::string& object) const {
        std::ofstream signature(object + ".sig");
        signature << CommandSignature() << "\n";
    }

    namespace {
        /// Minimal JSON string escaping for compile_commands.json.
        std::string JsonEscape(const std::string& text) {
            std::string out;
            out.reserve(text.size() + 8);
            for (char c : text) {
                switch (c) {
                    case '"':  out += "\\\""; break;
                    case '\\': out += "\\\\"; break;
                    case '\n': out += "\\n";  break;
                    case '\r': out += "\\r";  break;
                    case '\t': out += "\\t";  break;
                    default:
                        if (static_cast<unsigned char>(c) < 0x20) {
                            char escaped[7];
                            std::snprintf(escaped, sizeof(escaped), "\\u%04x", static_cast<unsigned char>(c));
                            out += escaped;
                        } else {
                            out += c;
                        }
                }
            }
            return out;
        }
    }

    /// The exact command Compile() runs. Absolute paths keep it valid from any directory, which
    /// is what clangd needs when it replays an entry from compile_commands.json.
    std::string Compiler::CompileCommandFor(const std::string& source_file, const std::string& output_file) {
        const std::string source = std::filesystem::absolute(source_file).generic_string();
        const std::string object = output_file.empty()
            ? std::filesystem::absolute(ObjectFileFor(source_file)).generic_string()
            : output_file;

        std::string command;
        for (size_t i = 0; i < build_command_.size(); ++i) {
            if (i > 0) command += " ";
            command += build_command_[i];
        }

        if (IsMSVC(config_.compiler_path)) {
            command += " /c " + Quote(source) + " /Fo" + Quote(object);
        } else {
            // -MMD writes the header dependency list next to the object, which is what lets
            // IsUpToDate notice a changed header instead of trusting the source mtime alone.
            command += " -c " + Quote(source) + " -o " + Quote(object)
                     + " -MMD -MF " + Quote(object + ".d");
        }
        return command;
    }

    bool Compiler::WriteCompileCommands(const std::vector<std::string>& source_files, const std::string& path) {
        const std::filesystem::path directory = config_.project_dir.empty()
            ? std::filesystem::current_path()
            : std::filesystem::weakly_canonical(config_.project_dir);

        std::string json = "[\n";
        for (size_t i = 0; i < source_files.size(); ++i) {
            const std::string file   = std::filesystem::absolute(source_files[i]).generic_string();
            const std::string object = std::filesystem::absolute(ObjectFileFor(source_files[i])).generic_string();

            json += "  {\n";
            json += "    \"directory\": \"" + JsonEscape(directory.generic_string()) + "\",\n";
            json += "    \"file\": \"" + JsonEscape(file) + "\",\n";
            json += "    \"output\": \"" + JsonEscape(object) + "\",\n";
            json += "    \"command\": \"" + JsonEscape(CompileCommandFor(source_files[i])) + "\"\n";
            json += (i + 1 < source_files.size()) ? "  },\n" : "  }\n";
        }
        json += "]\n";

        std::ofstream out(path);
        if (!out) return false;
        out << json;
        return static_cast<bool>(out);
    }

    /// A dependency file holds one rule, "<object>: <source> <header> ...", continued across
    /// lines with a trailing backslash. Returns true when any listed file is newer or gone.
    bool Compiler::DependsOnNewer(const std::string& depfile, std::filesystem::file_time_type object_time) {
        std::ifstream dep_stream(depfile);
        if (!dep_stream) return false;
        std::string text((std::istreambuf_iterator<char>(dep_stream)), std::istreambuf_iterator<char>());

        std::string joined;
        for (size_t i = 0; i < text.size(); ++i) {
            if (text[i] == '\\' && i + 1 < text.size() &&
                (text[i + 1] == '\n' || text[i + 1] == '\r')) {
                ++i;
                if (text[i] == '\r' && i + 1 < text.size() && text[i + 1] == '\n') ++i;
                joined += ' ';
                continue;
            }
            joined += text[i];
        }

        const size_t colon = joined.find(':');
        if (colon == std::string::npos) return false;

        std::istringstream dependencies(joined.substr(colon + 1));
        std::string path;
        while (dependencies >> path) {
            std::error_code ec;
            if (!std::filesystem::exists(path, ec)) return true;
            if (std::filesystem::last_write_time(path, ec) > object_time) return true;
        }
        return false;
    }

    bool Compiler::IsUpToDate(const std::string& source, const std::string& object) {
        std::error_code ec;
        if (!std::filesystem::exists(object, ec)) return false;

        // A command-line change means the object was built differently; never reuse it.
        std::string recorded;
        {
            std::ifstream signature(object + ".sig");
            signature >> recorded;
        }
        if (recorded != CommandSignature()) return false;

        const auto object_time = std::filesystem::last_write_time(object, ec);
        if (ec) return false;
        if (std::filesystem::last_write_time(source, ec) > object_time) return false;

        const std::string depfile = object + ".d";
        if (std::filesystem::exists(depfile, ec))
            return !DependsOnNewer(depfile, object_time);

        // No depfile means headers are invisible. MSVC builds do not emit one, so they keep
        // the source-only check; elsewhere a missing depfile means the object predates this
        // scheme and should be rebuilt once.
        return IsMSVC(config_.compiler_path);
    }

    int Compiler::Compile(const std::string& source_file, const std::string& output_file) {
        if (!std::filesystem::exists(source_file)) {
            std::cerr << ux::color::red << "Error: Source file " << source_file
                      << " does not exist!" << ux::color::reset << std::endl;
            return 1;
        }

        std::string ObjectFile = output_file.empty() ? ObjectFileFor(source_file) : output_file;
        std::filesystem::create_directories(std::filesystem::path(ObjectFile).parent_path());

        if (IsUpToDate(source_file, ObjectFile)) {
            if (config_.verbose) {
                std::ostringstream line;
                line << ux::color::cyan << "Cache hit for " << source_file << ux::color::reset;
                LogLine(line.str());
            }
            return 0;
        }

        {
            std::ostringstream line;
            line << ux::color::yellow << "[Compiling] -> " << source_file << "..." << ux::color::reset;
            LogLine(line.str());
        }

        const std::string CommandString = CompileCommandFor(source_file, ObjectFile);

        if (config_.verbose) {
            std::ostringstream line;
            line << ux::color::cyan << "COMMAND: " << Quote(CommandString) << ux::color::reset;
            LogLine(line.str());
        }

        int result = CompilerShell.RunQuiet(CommandString);
        if (result != 0) {
            // Print compiler errors by re-running without suppression
            CompilerShell.Run(CommandString);
        } else {
            WriteSignature(ObjectFile);
        }

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
        TotalErrors = Compiler::CompileSourceDirectory(*this, config_, source_files, ObjectFiles);

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
