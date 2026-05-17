#pragma once
#include <string>
#include "../system/system.h"

/// Returns the short SHA‑1 of the current Git commit.
/// If the repository cannot be queried (e.g. not a Git repo or Git not installed),
/// the function returns the string "unknown".
inline std::string get_git_commit() {
    SystemService shell;
    // `git rev-parse --short HEAD` gives a short 7‑character hash.
    std::string out = shell.run_with_output("git rev-parse --short HEAD");
    if (out.empty()) return "unknown";

    // Trim trailing new‑line characters.
    while (!out.empty() && (out.back() == '\n' || out.back() == '\r')) out.pop_back();
    return out;
}
