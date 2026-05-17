#include "../../include/git/git_service.h"
#include "../../include/system/system.h"
#include <filesystem>
#include <iostream>

namespace fs = std::filesystem;

GitService::GitService(bool use_https)
    : https_mode_(use_https) {}

std::string GitService::build_url(const std::string& author, const std::string& repo, bool https) const {
    if (https || https_mode_) {
        return "https://github.com/" + author + "/" + repo + ".git";
    }
    return "git@github.com:" + author + "/" + repo + ".git";
}

std::string GitService::dep_path(const std::string& repo, const std::string& base) {
    return base + "/" + repo;
}

bool GitService::clone(const std::string& author, const std::string& repo, const std::string& tag) {
    std::string dest = dep_path(repo, "target/deps");

    if (fs::exists(dest)) {
        return true;
    }

    fs::create_directories("target/deps");

    std::cout << "Cloning " << author << "/" << repo << "...\n";

    // Try preferred method first, fall back to HTTPS
    std::string url = build_url(author, repo, false);

    std::string cmd = "git clone --depth 1";
    if (!tag.empty()) {
        cmd += " --branch " + tag;
    }
    cmd += " " + url + " " + dest;

    int rc = shell_.run(cmd);

    // If SSH failed and we weren't already using HTTPS, try HTTPS fallback
    if (rc != 0 && !https_mode_) {
        std::cout << "SSH failed, trying HTTPS...\n";
        // Clean up partial directory
        if (fs::exists(dest)) {
            fs::remove_all(dest);
        }

        url = build_url(author, repo, true);
        cmd = "git clone --depth 1";
        if (!tag.empty()) {
            cmd += " --branch " + tag;
        }
        cmd += " " + url + " " + dest;

        rc = shell_.run(cmd);
    }

    if (rc != 0) {
        if (fs::exists(dest)) {
            fs::remove_all(dest);
        }
        return false;
    }

    return true;
}

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

bool GitService::commit(const std::string& message) {
    std::string cmd = "git commit -m \"" + message + "\"";
    int rc = shell_.run(cmd);
    return rc == 0;
}

bool GitService::push() {
    std::string cmd = "git push";
    int rc = shell_.run(cmd);
    return rc == 0;
}
