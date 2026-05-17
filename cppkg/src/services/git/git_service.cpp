#include "../../include/git/git_service.h"
#include "../../include/system/system.h"
#include "../../helpers/color.h"
#include <filesystem>
#include <iostream>

/// aliases here aliases there, are we ever going to stop using aliases? 
namespace fs = std::filesystem;

/// wooah https? 
GitService::GitService(bool use_https)
    : https_mode_(use_https) {}

/// i high-key dunno what most of this codr does i just trust it
std::string GitService::build_url(const std::string& author, const std::string& repo, bool https) const {
    if (https || https_mode_) {
        return "https://github.com/" + author + "/" + repo + ".git";
    }
    return "git@github.com:" + author + "/" + repo + ".git";
}

/// get dat path
std::string GitService::dep_path(const std::string& repo, const std::string& base) {
    return base + "/" + repo;
}

/// clone that ass
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
    // if user prefereth ith cppkgth shall useth ith
    std::string url = build_url(author, repo, false);
    std::string cmd = "git ls-remote " + url + " HEAD";
    int rc = shell_.run_quiet(cmd);

    // if dumb ass ssh fails lowk switch to https
    if (rc != 0 && !https_mode_) {
        url = build_url(author, repo, true);
        cmd = "git ls-remote " + url + " HEAD";
        rc = shell_.run_quiet(cmd);
    }

    return rc == 0;
}

/// for beginners this saves the changes to github
/// genuinely tho if you used git befoe you should know this
bool GitService::commit(const std::string& message) {
    if (!shell_.command_exists("git")) {
        std::cout << Color::green << "Git not found. Please install Git: https://git-scm.com/" << Color::reset << std::endl;
        return false;
    }
    std::string cmd = "git commit -m \"" + message + "\"";
    int rc = shell_.run(cmd);
    return rc == 0;
}

/// cmon girl! Push!
bool GitService::push() {
    if (!shell_.command_exists("git")) {
        return false;
    }
    std::string cmd = "git push";
    int rc = shell_.run(cmd);
    return rc == 0;
}
