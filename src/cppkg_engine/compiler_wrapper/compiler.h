// compiler_wrapper.h
#ifndef COMPILER_WRAPPER_H
    #define COMPILER_WRAPPER_H
    #include <string>
    #include <vector>
    #include <filesystem>

    namespace cppkg {
        class compiler {
            public:
                struct config {
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
                    config() = default;
                };

                explicit compiler(const config& config);
                // Constructors
                compiler();  // default constructor

                int compile(const std::string& source_file, const std::string& output_file = "");
                int compile_all(const std::vector<std::string>& source_files);

            private:
                config config_;
                std::vector<std::string> build_command_;

                void build_base_command();
                void add_flag(const std::string& flag);
                bool check_cache(const std::string& source, const std::string& output);
                void store_cache(const std::string& source, const std::string& output);
                std::string get_file_hash(const std::string& filename);


                /// misc

                bool is_msvc(const std::string& compiler_path);
                std::string translate_flag(const std::string& flag, bool msvc);
                std::string quote(const std::string& s);
                std::string handle_msvc_linker(const compiler::config& config, const std::vector<std::string>& obj_files);
                std::string handle_linux_macos_linker(const compiler::config& config, const std::vector<std::string>& obj_files);
                int compile_sourcedir(compiler& wrapper, const compiler::config& config, const std::vector<std::string>& source_files, bool msvc, std::vector<std::string>& obj_files);
        };
    }
#endif // COMPILER_WRAPPER_H