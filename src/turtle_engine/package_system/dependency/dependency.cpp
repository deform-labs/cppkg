#include "../../file_system/toml/parser/toml_parser.h"
#include "../../file_system/lock/lock_file.h"
#include "../../misc/UX/color.h"
#include "../github/github.h"
#include "dependency.h"
#include <filesystem>
#include <iostream>
#include <fstream>

using namespace cppkg;

toml_parser toml_;
namespace fs = std::filesystem;

/*
 * parses a dependency name into author and repo components
 * this is very technical it needed this comment :thumbs_up:
 */
std::pair<std::string, std::string> dependency::parse_name(const std::string& full) {
    size_t slash = full.find('/');
    if (slash == std::string::npos) {
        throw std::runtime_error("Invalid package name '" + full + "'. Expected format: author/repo (e.g. fmtlib/fmt)");
    }
    return {full.substr(0, slash), full.substr(slash + 1)};
}

///  dependencies, nice.
dependency_impl::dependency_impl(bool use_https)
    : git_(use_https) {}

/// ADD A DEPENDENCY GODDAMNIT
dependency dependency_impl::add(const std::string& spec, const std::string& project_root) {
    size_t at = spec.find('@');
    if (at == std::string::npos)
        throw std::runtime_error("Invalid format, expected: author/repo@version (e.g. fmtlib/fmt@10.1.0)");

    std::string name = spec.substr(0, at);
    std::string version = spec.substr(at + 1);

    auto [author, repo] = dependency::parse_name(name);

    // does it exist? I dont knowww
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

    dependency dep;
    dep.name = name;
    dep.author = author;
    dep.repo = repo;
    dep.version = version;
    dep.url = git_.build_url(author, repo);

    std::cout << ux::color::green << "Added dependency: " << name << " @ " << version << ux::color::reset << "\n";

    return dep;
}

/// REMOVE A DEPENDENCY GODDAMNIT
void dependency_impl::remove(const std::string& name, const std::string& project_root) {
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

    // kirkenstein if you dont know what this block does youre lowk cooked :skull:
    size_t dep_section = content.find("[dependencies]");
    if (dep_section == std::string::npos)
        throw std::runtime_error("No [dependencies] section found");

    // did you find the key?
    std::string search;

    // ah tinder match
    search = pkg_name + " = ";
    size_t pos = content.find(search, dep_section);

    // swipe left if it doesnt match :skull:
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

    // brutus deletus lines
    size_t line_start = content.rfind('\n', pos);
    if (line_start == std::string::npos) line_start = 0;
    else line_start++; // skip the newline char itself

    size_t line_end = content.find('\n', pos);
    if (line_end != std::string::npos) line_end++;

    content.erase(line_start, line_end - line_start);

    std::ofstream out(toml_path);
    out << content;

    std::cout << ux::color::green << "Removed dependency: " << pkg_name << ux::color::reset << "\n";
}

/// shall we load your dependencies sir?
std::vector<dependency> dependency_impl::load_dependencies(const std::string& project_root) {
    std::vector<dependency> deps;
    std::string toml_path = project_root + "/cppkg.toml";

    if (!fs::exists(toml_path))
        return deps;
    try {
        auto toml = toml_.parse_toml(toml_path);

        if (!toml.has("dependencies", "")) {
            // does dependencies have children?
            auto it = toml.sections.find("dependencies");
            if (it == toml.sections.end())
                return deps;

            for (const auto& [key, val] : it->second.keys) {
                auto [author, repo] = dependency::parse_name(key);
                dependency dep;
                dep.name = key;
                dep.author = author;
                dep.repo = repo;
                dep.version = val.value;
                dep.url = git_.build_url(author, repo);
                deps.push_back(dep);
            }
        }
    } catch (const std::exception& e) {
        std::cout << ux::color::red << "Warning: failed to parse dependencies: " << e.what() << ux::color::reset << "\n";
    }

    return deps;
}

/// shall we fetch all your dependencies sir?
bool dependency_impl::fetch_all(const std::string& project_root) {
    auto deps = load_dependencies(project_root);
    if (deps.empty()) return true;

    bool all_ok = true;
    fs::current_path(project_root);

    LockFile lock(project_root + "/cppkg.lock");  // add this

    for (const auto& dep : deps) {
        std::string dest = GithubClient::dep_path(dep.repo, "target/deps");

        if (fs::exists(dest)) {
            std::cout << ux::color::cyan << "Already fetched: " << dep.name << ux::color::reset << "\n";
            continue;
        }

        if (!git_.clone(dep.author, dep.repo, dep.version)) {
            std::cout << ux::color::red << "Failed to fetch: " << dep.name << ux::color::reset << "\n";
            all_ok = false;
        } else {
            // write to lockfile after successful clone
            std::string hash = git_.get_commit_hash(dest);
            lock.upsert(dep.name, dep.version, hash);
            std::cout << ux::color::cyan << "Locked: " << dep.name << " # " << hash << ux::color::reset << "\n";
        }
    }

    return all_ok;
}

/// shall we clean your dependencies sir?
void dependency_impl::clean(const std::string& project_root) {
    fs::path deps_dir = fs::path(project_root) / "target" / "deps";
    if (fs::exists(deps_dir)) {
        fs::remove_all(deps_dir);
        std::cout << ux::color::green << "Cleaned deps/" << ux::color::reset << "\n";
    }
}
