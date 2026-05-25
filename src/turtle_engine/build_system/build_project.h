#ifndef CPPKG_BUILD_PROJECT_H
    #define CPPKG_BUILD_PROJECT_H

    #include <string>

    namespace cppkg {
        class Build {
            public:
                static void build_project(const std::string& path);
                static void run_project(const std::string& path);
        };
    }
#endif // CPPKG_BUILD_PROJECT_H