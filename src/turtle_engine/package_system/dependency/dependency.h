#pragma once
#include "../github/github.h"
#include <string>
#include <vector>

namespace cppkg {
    struct dependency {
        std::string name;
        std::string repo;
        std::string author;
        std::string url;
        std::string version;

        static std::pair<std::string, std::string> parse_name(const std::string& full);
    };

    class dependency_impl {
        public:
            explicit dependency_impl(bool use_https = false);

            dependency add(const std::string& spec, const std::string& project_root = ".");

            void remove(const std::string& name, const std::string& project_root = ".");

            std::vector<dependency> load_dependencies(const std::string& project_root = ".");

            bool fetch_all(const std::string& project_root = ".");

            void clean(const std::string& project_root = ".");

        private:
            GithubClient git_;
    };
};