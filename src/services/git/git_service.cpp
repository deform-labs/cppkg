#include "../../include/git/git_service.h"
#include "../../include/system/system.h"
#include <filesystem>
#include <iostream>

namespace fs = std::filesystem;

GitService::GitService(bool use_https)
    : https_mode_(use_https) {}

std::string GitService::build_url(const std::string& author, const std::string& repo) const {
    if (https_mode_) {
        return "https://github.com/" + author + "/" + repo + ".git";
    }
    return "git@github.com:" + author + "/" + repo + ".git";
}

std::string GitService::dep_path(const std::string& repo) {
    return "deps/" + repo;
}

bool GitService::clone(const std::string& author, const std::string& repo, const std::string& tag) {
    std::string dest = dep_path(repo);

    if (fs::exists(dest)) {
        return true;
    }

    fs::create_directories("deps");

    std::string url = build_url(author, repo);

    std::cout << "Cloning " << author << "/" << repo << "...\n";

    std::string cmd = "git clone --depth 1";
    if (!tag.empty()) {
        cmd += " --branch " + tag;
    }
    cmd += " " + url + " " + dest;

    int rc = shell_.run(cmd);

    if (rc != 0) {
        if (fs::exists(dest)) {
            fs::remove_all(dest);
        }
        return false;
    }

    return true;
}

bool GitService::validate_repo(const std::string& author, const std::string& repo) {
    std::string url = build_url(author, repo);

    std::string cmd = "git ls-remote " + url + " HEAD";
    int rc = shell_.run_quiet(cmd);
    return rc == 0;
}
