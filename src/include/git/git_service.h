#pragma once
#include "../system/system.h"
#include <string>

class GitService {
    public:
        explicit GitService(bool use_https = false);

        bool clone(const std::string& author, const std::string& repo, const std::string& tag = "");

        bool validate_repo(const std::string& author, const std::string& repo);

        std::string build_url(const std::string& author, const std::string& repo, bool https = false) const;

        static std::string dep_path(const std::string& repo, const std::string& base = "target");

    private:
        bool https_mode_;
        SystemService shell_;
};
