#ifndef CPPKG_BUILD_PROJECT_H
    #define CPPKG_BUILD_PROJECT_H

    #include <string>
    #include <filesystem>
    #include "../package_system/package_system.h"
    #include <vector>

    namespace cppkg {
        class build {
            public:
                static void build_project(const std::string& path);
                static void run_project(const std::string& path);
                static void create_lists(std::filesystem::path project_dir);
        };
    }

#endif // CPPKG_BUILD_PROJECT_H