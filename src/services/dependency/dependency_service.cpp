#include "../../include/dependency/dependency_service.h"
#include "../../include/toml/toml_parser.h"
#include "../../helpers/color.h"
#include <filesystem>
#include <fstream>
#include <iostream>

namespace fs = std::filesystem;

std::pair<std::string, std::string> Dependency::parse_name(const std::string& full) {
    size_t slash = full.find('/');
    if (slash == std::string::npos) {
        throw std::runtime_error("Invalid package name '" + full + "'. Expected format: author/repo (e.g. fmtlib/fmt)");
    }
    return {full.substr(0, slash), full.substr(slash + 1)};
}

DependencyService::DependencyService(bool use_https)
    : git_(use_https) {}

Dependency DependencyService::add(const std::string& spec, const std::string& project_root) {
    size_t at = spec.find('@');
    if (at == std::string::npos)
        throw std::runtime_error("Invalid format, expected: author/repo@version (e.g. fmtlib/fmt@10.1.0)");

    std::string name = spec.substr(0, at);
    std::string version = spec.substr(at + 1);

    auto [author, repo] = Dependency::parse_name(name);

    // Validate the repo exists before writing to cppkg.toml
    std::cout << "Checking repository: " << name << "...\n";
    if (!git_.validate_repo(author, repo)) {
        throw std::runtime_error("Repository not found or unreachable: " + name +
            "\n  Make sure it exists at github.com/" + author + "/" + repo +
            "\n  Use --https if you need HTTPS instead of SSH");
    }

    std::string toml_path = project_root + "/cppkg.toml";
    if (!fs::exists(toml_path))
        throw std::runtime_error("No cppkg.toml found in " + project_root);

    std::ifstream in(toml_path);
    std::string content((std::istreambuf_iterator<char>(in)),
                         std::istreambuf_iterator<char>());
    in.close();

    size_t pos = content.find("[dependencies]");
    if (pos == std::string::npos)
        throw std::runtime_error("No [dependencies] section found in cppkg.toml");

    size_t insert_pos = content.find('\n', pos) + 1;


    std::string entry = name + " = \"" + version + "\"\n";
    content.insert(insert_pos, entry);

    std::ofstream out(toml_path);
    out << content;

    Dependency dep;
    dep.name = name;
    dep.author = author;
    dep.repo = repo;
    dep.version = version;
    dep.url = git_.build_url(author, repo);

    std::cout << Color::green << "Added dependency: " << name << " @ " << version << Color::reset << "\n";

    return dep;
}

void DependencyService::remove(const std::string& name, const std::string& project_root) {
    std::string pkg_name = name;
    size_t at = name.find('@');
    if (at != std::string::npos)
        pkg_name = name.substr(0, at);

    std::string toml_path = project_root + "/cppkg.toml";
    if (!fs::exists(toml_path))
        throw std::runtime_error("No cppkg.toml found in " + project_root);

    std::ifstream in(toml_path);
    std::string content((std::istreambuf_iterator<char>(in)),
                         std::istreambuf_iterator<char>());
    in.close();

    // Find the [dependencies] section
    size_t dep_section = content.find("[dependencies]");
    if (dep_section == std::string::npos)
        throw std::runtime_error("No [dependencies] section found");

    // Look for the dependency key
    std::string search;

    // Try exact match with author/repo first
    search = pkg_name + " = ";
    size_t pos = content.find(search, dep_section);

    // If not found, try just the repo name part
    if (pos == std::string::npos) {
        auto slash = pkg_name.find('/');
        if (slash != std::string::npos) {
            std::string short_name = pkg_name.substr(slash + 1);
            search = short_name + " = ";
            pos = content.find(search, dep_section);
        }
    }

    if (pos == std::string::npos)
        throw std::runtime_error("Dependency not found: " + pkg_name);

    // Find line boundaries and remove the entire line
    size_t line_start = content.rfind('\n', pos);
    if (line_start == std::string::npos) line_start = 0;
    else line_start++; // skip the newline char itself

    size_t line_end = content.find('\n', pos);
    if (line_end != std::string::npos) line_end++;

    content.erase(line_start, line_end - line_start);

    std::ofstream out(toml_path);
    out << content;

    std::cout << Color::green << "Removed dependency: " << pkg_name << Color::reset << "\n";
}

std::vector<Dependency> DependencyService::load_dependencies(const std::string& project_root) {
    std::vector<Dependency> deps;
    std::string toml_path = project_root + "/cppkg.toml";

    if (!fs::exists(toml_path))
        return deps;

    try {
        auto toml = parse_toml(toml_path);

        if (!toml.has("dependencies", "")) {
            // Check if there are any key-value pairs under [dependencies]
            auto it = toml.sections.find("dependencies");
            if (it == toml.sections.end())
                return deps;

            for (const auto& [key, val] : it->second.keys) {
                auto [author, repo] = Dependency::parse_name(key);
                Dependency dep;
                dep.name = key;
                dep.author = author;
                dep.repo = repo;
                dep.version = val.value;
                dep.url = git_.build_url(author, repo);
                deps.push_back(dep);
            }
        }
    } catch (...) {
        // If parsing fails, return empty list
    }

    return deps;
}

bool DependencyService::fetch_all(const std::string& project_root) {
    auto deps = load_dependencies(project_root);
    if (deps.empty()) {
        return true; // nothing to fetch
    }

    bool all_ok = true;
    fs::current_path(project_root);

    for (const auto& dep : deps) {
        std::string dest = GitService::dep_path(dep.repo);

        if (fs::exists(dest)) {
            std::cout << Color::cyan << "Already fetched: " << dep.name << Color::reset << "\n";
            continue;
        }

        if (!git_.clone(dep.author, dep.repo, dep.version)) {
            std::cout << Color::red << "Failed to fetch: " << dep.name << Color::reset << "\n";
            all_ok = false;
        }
    }

    return all_ok;
}

void DependencyService::clean(const std::string& project_root) {
    fs::path deps_dir = fs::path(project_root) / "deps";
    if (fs::exists(deps_dir)) {
        fs::remove_all(deps_dir);
        std::cout << Color::green << "Cleaned deps/" << Color::reset << "\n";
    }
}
