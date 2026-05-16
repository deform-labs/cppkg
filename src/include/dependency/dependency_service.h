#pragma once
#include <string>
#include <vector>
#include "../git/git_service.h"
#include "../system/system.h"

struct Dependency {
    std::string name;    // e.g. "fmtlib/fmt"
    std::string repo;    // e.g. "fmt"
    std::string author;  // e.g. "fmtlib"
    std::string url;     // full git URL
    std::string version; // tag/branch, e.g. "10.1.0"

    /// Parse "author/repo" into author and repo
    static std::pair<std::string, std::string> parse_name(const std::string& full);
};

class DependencyService {
    public:
        explicit DependencyService(bool use_https = false);

        /// Add a dependency line like "author/repo@version" to cppkg.toml
        /// Returns the parsed Dependency on success.
        Dependency add(const std::string& spec, const std::string& project_root = ".");

        /// Remove a dependency by name (author/repo or just repo name)
        void remove(const std::string& name, const std::string& project_root = ".");

        /// Read all dependencies from cppkg.toml
        std::vector<Dependency> load_dependencies(const std::string& project_root = ".");

        /// Clone all missing dependencies into deps/
        bool fetch_all(const std::string& project_root = ".");

        /// Clean all cloned dependencies (remove deps/ directory)
        void clean(const std::string& project_root = ".");

    private:
        GitService git_;
        SystemService shell_;
};
