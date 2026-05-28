#pragma once
#include "../file_system/toml/toml.h"
#include <filesystem>
#include <string>
#include <vector>

struct WorkspaceMember {
    std::string name;
    std::string path;
};

struct Workspace {
    std::string root;
    std::vector<WorkspaceMember> members;
};

// load that motherfucker
inline Workspace load_workspace(const std::string& root_path) {
    Workspace ws;
    cppkg::toml_parser workspace_parser;
    ws.root = root_path;

    auto toml = workspace_parser.parse_toml(root_path + "/cppkg.toml");

    if (!toml.has("workspace", "members"))
        throw std::runtime_error("Not a workspace: no [workspace] members found");

    return ws;
}
