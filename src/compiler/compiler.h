// compiler_wrapper.h
#pragma once
#include <string>
#include <vector>
#include <filesystem>

class CompilerWrapper {
public:
    struct CompilerConfig {
        std::string compiler_path = "g++";
        std::string cpp_std = "c++20";
        std::vector<std::string> default_flags = {"-Wall", "-Wextra", "-O2"};
        std::vector<std::string> include_paths;
        std::vector<std::string> library_paths;
        std::vector<std::string> libraries;
        bool verbose = false;
        bool color_output = true;
        std::string cache_dir = ".cppkg/cache";  // This is fine here
        std::string build_dir = "build";
        std::string output_name = "cppkg_app";
        CompilerConfig() = default;
    };

    explicit CompilerWrapper(const CompilerConfig& config);
    // Constructors
    CompilerWrapper();  // default constructor

    int compile(const std::string& source_file, const std::string& output_file = "");
    int compile_all(const std::vector<std::string>& source_files);

private:
    CompilerConfig config_;
    std::vector<std::string> build_command_;

    void build_base_command();
    void add_flag(const std::string& flag);
    bool check_cache(const std::string& source, const std::string& output);
    void store_cache(const std::string& source, const std::string& output);
    std::string get_file_hash(const std::string& filename);
};