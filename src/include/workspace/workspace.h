#pragma once
#include <string>
#include <vector>

struct WorkspaceMember {
    std::string name;
    std::string path;
};

struct Workspace {
    std::string root;
    std::vector<WorkspaceMember> members;

    bool is_workspace() const { return !members.empty(); }
};
