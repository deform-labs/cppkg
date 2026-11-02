// compiler_wrapper.h
#ifndef COMPILER_WRAPPER_H
    #define COMPILER_WRAPPER_H
    #include <string>
    #include <vector>
    #include <filesystem>

    namespace cppkg {
        class Compiler {
            public:
                struct Config {
                    std::string compiler_path = "g++";
                    std::string cpp_std = "c++20";

                    std::vector<std::string> default_flags = {"-Wall", "-Wextra", "-O2"};
                    std::vector<std::string> include_paths;
                    std::vector<std::string> library_paths;
                    std::vector<std::string> libraries;

                    bool verbose = false;
                    bool color_output = true;

                    std::string cache_dir = ".cppkg/cache";  // This is fine here
                    std::string build_dir = "target";
                    std::string output_name = "cppkg_app";

                    Config() = default;
                };

                explicit Compiler(const Config& config);
                // Constructors
                Compiler();  // default constructor

                int Compile(const std::string& source_file, const std::string& output_file = "");
                int CompileAll(const std::vector<std::string>& source_files);

            private:
                Config config;
                std::vector<std::string> build_command_;

                void BuildBaseCommand();
                void AddFlag(const std::string& flag);
                bool CheckCache(const std::string& source, const std::string& output);
                void StoreCache(const std::string& source, const std::string& output);
                std::string GetFileHash(const std::string& filename);


                /// misc

                bool IsMSVC(const std::string& CompilerPath);
                std::string TranslateFlag(const std::string& flag, bool msvc);
                std::string Quote(const std::string& s);
                std::string HandleMSVCLinker(const Config& config, const std::vector<std::string>& obj_files);
                std::string HandleLinuxMacOSLinker(const Config& config, const std::vector<std::string>& obj_files);
                int CompileSourceDirectory(Compiler& wrapper, const Config& config, const std::vector<std::string>& source_files, bool msvc, std::vector<std::string>& obj_files);
        };
    }
#endif // COMPILER_WRAPPER_H