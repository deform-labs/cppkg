#pragma once
#include <string>
#include <vector>
#include "../toml/toml_parser.h"
#include <filesystem>

struct WorkspaceMember {
    std::string name;
    std::string path;
};

struct Workspace {
    std::string root;
    std::vector<WorkspaceMember> members;
};

inline Workspace load_workspace(const std::string& root_path) {
    Workspace ws;
    ws.root = root_path;

    auto toml = parse_toml(root_path + "/cppkg.toml");

    if (!toml.has("workspace", "members"))
        throw std::runtime_error("Not a workspace: no [workspace] members found");

    return ws;
}
