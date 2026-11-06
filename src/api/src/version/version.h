#pragma once
#include "../../../engine/src/cppkg_engine.h"
#include <string>


/// Get the current Git commit hash (short version) for the repository.
inline std::string get_git_commit() {
    cppkg::CommandSystem shell_;
    // `git rev-parse --short HEAD` gives a short 7‑character hash.
    std::string out = shell_.RunWithOutput("git rev-parse --short HEAD");
    if (out.empty()) return "unknown";

    // Trim trailing new‑line characters.
    while (!out.empty() && (out.back() == '\n' || out.back() == '\r')) out.pop_back();
    return out;
};
