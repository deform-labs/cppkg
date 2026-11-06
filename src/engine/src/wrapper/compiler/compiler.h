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

                    /// Sources live below this root; objects mirror their path under it, so
                    /// src/net/util.cpp and src/io/util.cpp no longer share one object file.
                    std::string source_root;

                    /// Project root, recorded as the working directory in compile_commands.json.
                    std::string project_dir;

                    /// Threads used to compile. 0 means one per hardware thread.
                    int jobs = 0;

                    Config() = default;
                };

                explicit Compiler(const Config& config);
                // Constructors
                Compiler();  // default constructor

                /// Reduces "c++20", "20", "C++20", "latest" or "" to a bare version: "20".
                /// Both -std=c++NN and CMAKE_CXX_STANDARD need the number without the prefix.
                static std::string StdVersion(const std::string& cpp_std);

                /// Object file for a source, mirroring its path below Config::source_root.
                std::string ObjectFileFor(const std::string& source_file);

                /// The compile command for one source, with absolute paths.
                std::string CompileCommandFor(const std::string& source_file, const std::string& output_file = "");

                /// Writes compile_commands.json for clangd and other tooling. Returns false when
                /// the file could not be written.
                bool WriteCompileCommands(const std::vector<std::string>& source_files, const std::string& path);

                /// Prints one whole line; compile threads share stdout.
                static void LogLine(const std::string& line);

                int Compile(const std::string& source_file, const std::string& output_file = "");
                int CompileAll(const std::vector<std::string>& source_files);

            private:
                Config config_;
                std::vector<std::string> build_command_;

                void BuildBaseCommand();
                void AddFlag(const std::string& flag);

                /// Hash of the compile command line: changing a flag, -std or the compiler
                /// invalidates objects even when they are newer than the sources.
                std::string CommandSignature() const;
                /// An object is reusable only if it was built by this exact command line and is
                /// newer than the source plus every header listed in its dependency file.
                bool IsUpToDate(const std::string& source, const std::string& object);
                void WriteSignature(const std::string& object) const;
                bool DependsOnNewer(const std::string& depfile, std::filesystem::file_time_type object_time);


                /// misc

                bool IsMSVC(const std::string& CompilerPath);
                std::string TranslateFlag(const std::string& flag, bool msvc);
                std::string Quote(const std::string& s);
                std::string HandleMSVCLinker(const Config& config, const std::vector<std::string>& obj_files);
                std::string HandleLinuxMacOSLinker(const Config& config, const std::vector<std::string>& obj_files);
                int CompileSourceDirectory(Compiler& wrapper, const Config& config, const std::vector<std::string>& source_files, std::vector<std::string>& obj_files);
        };
    }
#endif // COMPILER_WRAPPER_H
