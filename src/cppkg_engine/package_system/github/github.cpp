#include "../../command_system/command_system.h"
#include "../../misc/ux/color.h"
#include <filesystem>
#include <iostream>
#include "github.h"
#include <vector>

/// aliases here aliases there, are we ever going to stop using aliases?
cppkg::command_system shell_;
namespace fs = std::filesystem;
using namespace cppkg;

/// wooah https?
github::github(bool use_https)
    : https_mode_(use_https) {}

/// i high-key dunno what most of this codr does i just trust it
std::string github::build_url(const std::string& author, const std::string& repo, bool https) const {
    if (https || https_mode_) {
        return "https://github.com/" + author + "/" + repo + ".git";
    }
    return "git@github.com:" + author + "/" + repo + ".git";
}

/// get dat path
std::string github::dep_path(const std::string& repo, const std::string& base) {
    return base + "/" + repo;
}

// get that assh
std::string github::get_commit_hash(const std::string& repo_path) {
    std::string cmd = "git -C \"" + repo_path + "\" rev-parse --short HEAD";
    std::string out = shell_.run_with_output(cmd);
    // trim that assh
    while (!out.empty() && (out.back() == '\n' || out.back() == '\r' || out.back() == ' '))
        out.pop_back();
    return out;
}

/// clone that ass
bool github::clone(const std::string& author, const std::string& repo, const std::string& tag) {
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
bool github::validate_repo(const std::string& author, const std::string& repo) {
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
/// genuinely tho if you used git before you should know this
bool github::commit(const std::string& message) {
    if (!shell_.command_exists("git")) {
        std::cout << ux::color::green << "Git not found. Please install Git: https://git-scm.com/" << ux::color::reset << std::endl;
        return false;
    }
    std::string cmd = "git commit -m \"" + message + "\"";
    int rc = shell_.run(cmd);
    return rc == 0;
}

/// cmon girl! Push!
bool github::push() {
    if (!shell_.command_exists("git")) {
        return false;
    }
    std::string cmd = "git push";
    int rc = shell_.run(cmd);
    return rc == 0;
}


/// forked ts because i didnt know how the github api worked
std::vector<search_result> github::search(const std::string& query) {
    std::string cmd = "curl -s \"https://api.github.com/search/repositories"
                  "?q=" + query + "+language:cpp&sort=stars&per_page=5\"";

    std::string output = shell_.run_with_output(cmd);
    if (output.empty()) return {};

    // parse json manually - no deps
    std::vector<search_result> results;
    size_t pos = 0;

    while ((pos = output.find("\"full_name\"", pos)) != std::string::npos) {
        size_t start = output.find('"', pos + 12) + 1;
        size_t end   = output.find('"', start);
        std::string full_name = output.substr(start, end - start);

        size_t desc_pos = output.find("\"description\"", pos);
        std::string description;
        if (desc_pos != std::string::npos) {
            size_t ds = output.find('"', desc_pos + 14);
            if (output[ds + 1] != 'n') { // not null
                ds++;
                size_t de = output.find('"', ds + 1);
                description = output.substr(ds, de - ds);
            }
        }

        size_t stars_pos = output.find("\"stargazers_count\"", pos);
        int stars = 0;
        if (stars_pos != std::string::npos) {
            size_t sv = stars_pos + 19;
            stars = std::stoi(output.substr(sv, output.find(',', sv) - sv));
        }

        results.push_back({ full_name, description, stars });
        pos = end;
    }

    return results;
}

/// forked this too lol
/// gets the owner from git remote
std::string github::get_remote_owner() {
    std::string output = shell_.run_with_output("git remote get-url origin");
    // parses both https://github.com/owner/repo and git@github.com:owner/repo
    size_t pos = output.find("github.com");
    if (pos == std::string::npos) return "";
    pos += 11; // skip "github.com/" or "github.com:"
    size_t end = output.find('/', pos);
    if (end == std::string::npos) return "";
    return output.substr(pos, end - pos);
}

/// forked this too lmao
/// publish a release to github
bool github::publish(const std::string& version, const std::string& message, const std::string& token) {
    // step 1: git add and commit
    std::cout << ux::color::cyan << "Committing changes..." << ux::color::reset << "\n";
    shell_.run("git add .");
    std::string commit_cmd = "git commit -m \"" + message + "\"";
    shell_.run(commit_cmd);

    // step 2: push
    std::cout << ux::color::cyan << "Pushing to remote..." << ux::color::reset << "\n";
    int rc = shell_.run("git push");
    if (rc != 0) {
        std::cout << ux::color::red << "Push failed!" << ux::color::reset << "\n";
        return false;
    }

    // step 3: create and push tag
    std::cout << ux::color::cyan << "Creating tag v" << version << "..." << ux::color::reset << "\n";
    shell_.run("git tag v" + version);
    rc = shell_.run("git push origin v" + version);
    if (rc != 0) {
        std::cout << ux::color::red << "Tag push failed!" << ux::color::reset << "\n";
        return false;
    }

    // step 4: get owner and repo from remote
    std::string owner = get_remote_owner();
    std::string remote = shell_.run_with_output("git remote get-url origin");
    // parse repo name
    size_t slash = remote.rfind('/');
    size_t dot   = remote.rfind('.');
    if (slash == std::string::npos) return false;
    std::string repo = remote.substr(slash + 1, dot - slash - 1);
    // trim whitespace
    while (!repo.empty() && (repo.back() == '\n' || repo.back() == '\r' || repo.back() == ' '))
        repo.pop_back();

    // step 5: create github release via api
    std::cout << ux::color::cyan << "Creating GitHub release..." << ux::color::reset << "\n";
    std::string body = "{\\\"tag_name\\\":\\\"v" + version + "\\\","
                   "\\\"name\\\":\\\"v" + version + "\\\","
                   "\\\"body\\\":\\\"" + message + "\\\"}";

    std::string curl_cmd =
        "curl -s -X POST "
        "https://api.github.com/repos/" + owner + "/" + repo + "/releases "
        "-H \"Authorization: token " + token + "\" "
        "-H \"Content-Type: application/json\" "
        "-d \"" + body + "\"";

    std::string response = shell_.run_with_output(curl_cmd);

    // check if release was created
    if (response.find("\"id\"") != std::string::npos) {
        std::cout << ux::color::green << "Released v" << version
                  << " at github.com/" << owner << "/" << repo << ux::color::reset << "\n";
        return true;
    }

    std::cout << ux::color::red << "Release creation failed!\n" << response << ux::color::reset << "\n";
    return false;
}