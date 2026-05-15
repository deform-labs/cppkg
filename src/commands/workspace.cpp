#include "workspace/workspace.h"

Workspace load_workspace(const std::string &root_path) {
    Workspace ws;
    ws.root = root_path;

    // read root cppkg.toml
    // if it has [workspace] section, parse members[]
    // for each member, check that member/cppkg.toml exists
    // push into ws.members

    return ws;
}
