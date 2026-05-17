#include "../../include/git/git_service.h"
#include "../../include/system/system.h"
#include "../../helpers/color.h"
#include <filesystem>
#include <iostream>

/// Namespace alias for std::filesystem
namespace fs = std::filesystem;

/// Constructs a GitService instance, optionally using HTTPS for all operations
GitService::GitService(bool use_https)
    : https_mode_(use_https) {}

/// Returns the URL for the given author and repo, using HTTPS if enabled
std::string GitService::build_url(const std::string& author, const std::string& repo, bool https) const {
    if (https || https_mode_) {
        return "https://github.com/" + author + "/" + repo + ".git";
    }
    return "git@github.com:" + author + "/" + repo + ".git";
}

/// Returns the path to the cloned repository
std::string GitService::dep_path(const std::string& repo, const std::string& base) {
    return base + "/" + repo;
}

/// Clones the repository for the given author and repo, using the specified tag if provided
bool GitService::clone(const std::string& author, const std::string& repo, const std::string& tag) {
    std::string dest = dep_path(repo, "target/deps");

    if (fs::exists(dest)) {
        return true;
    }

    fs::create_directories("target/deps");

    std::cout << "Cloning " << author << "/" << repo << "...\n";

    std::string url = build_url(author, repo, false);

    std::string cmd = "git clone --depth 1";
    if (!tag.empty()) {
        cmd += " --branch " + tag;
    }
    cmd += " " + url + " " + dest;

    int rc = shell_.run_quiet(cmd);

    if (rc != 0 && !https_mode_) {
        std::cout << "SSH failed, trying HTTPS...\n";
        if (fs::exists(dest)) {
            fs::remove_all(dest);
        }

        url = build_url(author, repo, true);
        cmd = "git clone --depth 1";
        if (!tag.empty()) {
            cmd += " --branch " + tag;
        }
        cmd += " " + url + " " + dest;

        rc = shell_.run_quiet(cmd);
    }

    if (rc != 0) {
        if (fs::exists(dest)) {
            fs::remove_all(dest);
        }
        return false;
    }

    return true;
}

/// Validates the repository for the given author and repo, using HTTPS if enabled
bool GitService::validate_repo(const std::string& author, const std::string& repo) {
    // Try preferred method first
    std::string url = build_url(author, repo, false);
    std::string cmd = "git ls-remote " + url + " HEAD";
    int rc = shell_.run_quiet(cmd);

    // If SSH failed and we weren't already using HTTPS, try HTTPS fallback
    if (rc != 0 && !https_mode_) {
        url = build_url(author, repo, true);
        cmd = "git ls-remote " + url + " HEAD";
        rc = shell_.run_quiet(cmd);
    }

    return rc == 0;
}

/// Commits the changes with the given message
bool GitService::commit(const std::string& message) {
    if (!shell_.command_exists("git")) {
        std::cout << Color::green << "Git not found. Please install Git: https://git-scm.com/" << Color::reset << std::endl;
        return false;
    }
    std::string cmd = "git commit -m \"" + message + "\"";
    int rc = shell_.run(cmd);
    return rc == 0;
}

/// Pushes the changes to the remote repository
bool GitService::push() {
    if (!shell_.command_exists("git")) {
        return false;
    }
    std::string cmd = "git push";
    int rc = shell_.run(cmd);
    return rc == 0;
}
