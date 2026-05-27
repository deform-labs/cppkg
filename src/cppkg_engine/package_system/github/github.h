#ifndef CPPKG_GITHUB_IMPL_H
    #define CPPKG_GITHUB_IMPL_H
    #include <string>
    #include <vector>

    namespace cppkg {
        struct search_result {
            std::string full_name;    // "fmtlib/fmt"
            std::string description;
            int stars;
        };

        class github {
            public:
                explicit github(bool use_https = false);

                /// Clones the repository for the given author and repo, using the specified tag if provided
                bool clone(const std::string& author, const std::string& repo, const std::string& tag = "");

                /// Validates the repository for the given author and repo, using HTTPS if enabled
                bool validate_repo(const std::string& author, const std::string& repo);

                /// Returns the URL for the given author and repo, using HTTPS if enabled
                std::string build_url(const std::string& author, const std::string& repo, bool https = false) const;

                /// Returns the path to the cloned repository
                static std::string dep_path(const std::string& repo, const std::string& base = "target");

                /// Returns the commit hash of the given repository
                std::string get_commit_hash(const std::string& repo_path);

                /// Commits the changes with the given message
                bool commit(const std::string& message);

                /// Pushes the changes to the remote repository
                bool push();

                /// Searches for dependencies on GitHub
                std::vector<search_result> search(const std::string& query);

                /// Publishes a release to GitHub
                bool publish(const std::string& version, const std::string& message, const std::string& token);

                /// Returns the owner of the remote repository
                std::string get_remote_owner();
            private:
                bool https_mode_;
        };
    }
#endif // CPPKG_GITHUB_IMPL_H