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


/// simple helper: extract the string value of a JSON key from a substring
static std::string json_string_field(const std::string& blob, const std::string& key, size_t from = 0) {
    std::string needle = "\"" + key + "\"";
    size_t kpos = blob.find(needle, from);
    if (kpos == std::string::npos) return "";
    size_t colon = blob.find(':', kpos + needle.size());
    if (colon == std::string::npos) return "";
    size_t vstart = blob.find_first_not_of(" \t\r\n", colon + 1);
    if (vstart == std::string::npos) return "";
    if (blob[vstart] == 'n') return ""; // null
    if (blob[vstart] != '"') return ""; // not a string
    vstart++;
    std::string result;
    for (size_t i = vstart; i < blob.size(); ++i) {
        if (blob[i] == '\\' && i + 1 < blob.size()) { result += blob[++i]; continue; }
        if (blob[i] == '"') break;
        result += blob[i];
    }
    return result;
}

static int json_int_field(const std::string& blob, const std::string& key, size_t from = 0) {
    std::string needle = "\"" + key + "\"";
    size_t kpos = blob.find(needle, from);
    if (kpos == std::string::npos) return 0;
    size_t colon = blob.find(':', kpos + needle.size());
    if (colon == std::string::npos) return 0;
    size_t vstart = blob.find_first_not_of(" \t\r\n", colon + 1);
    if (vstart == std::string::npos || !isdigit(blob[vstart])) return 0;
    return std::stoi(blob.substr(vstart));
}

/// forked ts because i didnt know how the github api worked
std::vector<search_result> github::search(const std::string& query) {
    std::string cmd = "curl -s \"https://api.github.com/search/repositories"
                  "?q=" + query + "+language:cpp&sort=stars&per_page=5\"";

    std::string output = shell_.run_with_output(cmd);
    if (output.empty()) return {};

    std::vector<search_result> results;
    size_t pos = 0;

    // Each repository object starts after a "full_name" key; walk through them
    while ((pos = output.find("\"full_name\"", pos)) != std::string::npos) {
        // Find the enclosing object: walk back to the nearest '{'
        size_t obj_start = output.rfind('{', pos);
        if (obj_start == std::string::npos) { pos++; continue; }

        // Find the matching '}' (shallow — repos don't nest deeply here)
        int depth = 0;
        size_t obj_end = obj_start;
        for (size_t i = obj_start; i < output.size(); ++i) {
            if (output[i] == '{') depth++;
            else if (output[i] == '}') { if (--depth == 0) { obj_end = i; break; } }
        }
        std::string obj = output.substr(obj_start, obj_end - obj_start + 1);

        std::string full_name   = json_string_field(obj, "full_name");
        std::string description = json_string_field(obj, "description");
        int         stars       = json_int_field(obj, "stargazers_count");

        if (!full_name.empty())
            results.push_back({ full_name, description, stars });

        pos = obj_end + 1;
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
    // JSON body and token go through temp files so neither appears in the process list.
    std::cout << ux::color::cyan << "Creating GitHub release..." << ux::color::reset << "\n";

    std::string body_json = "{\"tag_name\":\"v" + version + "\","
                            "\"name\":\"v" + version + "\","
                            "\"body\":\"" + message + "\"}";

    namespace fs_tmp = std::filesystem;
    std::string tmp_body   = (fs_tmp::temp_directory_path() / "cppkg_release_body.json").string();
    std::string tmp_config = (fs_tmp::temp_directory_path() / "cppkg_curl.cfg").string();

    {
        std::ofstream body_file(tmp_body);
        body_file << body_json;
    }
    {
        // curl config file: keeps the token out of argv / ps output
        std::ofstream cfg_file(tmp_config);
        cfg_file << "header = \"Authorization: token " << token << "\"\n";
        cfg_file << "header = \"Content-Type: application/json\"\n";
    }

    std::string curl_cmd =
        "curl -s -X POST "
        "https://api.github.com/repos/" + owner + "/" + repo + "/releases "
        "--config " + tmp_config + " "
        "--data @" + tmp_body;

    std::string response = shell_.run_with_output(curl_cmd);

    std::remove(tmp_body.c_str());
    std::remove(tmp_config.c_str());

    // check if release was created
    if (response.find("\"id\"") != std::string::npos) {
        std::cout << ux::color::green << "Released v" << version
                  << " at github.com/" << owner << "/" << repo << ux::color::reset << "\n";
        return true;
    }

    std::cout << ux::color::red << "Release creation failed!\n" << response << ux::color::reset << "\n";
    return false;
}