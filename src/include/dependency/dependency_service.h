#pragma once
#include "../git/git_service.h"
#include <string>
#include <vector>

struct Dependency {
    std::string name;
    std::string repo;
    std::string author;
    std::string url;
    std::string version;

    static std::pair<std::string, std::string> parse_name(const std::string& full);
};

class DependencyService {
    public:
        explicit DependencyService(bool use_https = false);

        Dependency add(const std::string& spec, const std::string& project_root = ".");

        void remove(const std::string& name, const std::string& project_root = ".");

        std::vector<Dependency> load_dependencies(const std::string& project_root = ".");

        bool fetch_all(const std::string& project_root = ".");

        void clean(const std::string& project_root = ".");

    private:
        GitService git_;
};
