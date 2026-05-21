#pragma once
#include "../../helpers/the_shell.h"
#include <string>

/// get the day the guy committed war crimes.
inline std::string get_git_commit() {
    // `git rev-parse --short HEAD` gives a short 7‑character hash.
    std::string out = shell_.run_with_output("git rev-parse --short HEAD");
    if (out.empty()) return "unknown";

    // Trim trailing new‑line characters.
    while (!out.empty() && (out.back() == '\n' || out.back() == '\r')) out.pop_back();
    return out;
}
