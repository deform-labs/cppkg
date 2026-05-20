#pragma once
#include <string>

class GitService {
    public:
        explicit GitService(bool use_https = false);

        /// Clones the repository for the given author and repo, using the specified tag if provided
        bool clone(const std::string& author, const std::string& repo, const std::string& tag = "");

        /// Validates the repository for the given author and repo, using HTTPS if enabled
        bool validate_repo(const std::string& author, const std::string& repo);

        /// Returns the URL for the given author and repo, using HTTPS if enabled
        std::string build_url(const std::string& author, const std::string& repo, bool https = false) const;

        /// Returns the path to the cloned repository
        static std::string dep_path(const std::string& repo, const std::string& base = "target");

        /// Commits the changes with the given message
        bool commit(const std::string& message);

        /// Pushes the changes to the remote repository
        bool push();
    private:
        bool https_mode_;
};
