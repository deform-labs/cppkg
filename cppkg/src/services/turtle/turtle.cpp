#include "../../include/turtle/turtle.h"
#include <cstdlib>
#include <cstdio>
#include <array>

/// JUST DO IT
int turtle::run(const std::string& command) {
    return std::system(command.c_str());
}

/// shh mom will hear us
int turtle::run_quiet(const std::string& command) {
    #ifdef _WIN32
        std::string cmd = command + " > nul 2>&1";
    #else
        std::string cmd = command + " > /dev/null 2>&1";
    #endif

    return std::system(cmd.c_str());
}

/// mom heard us!
std::string turtle::run_with_output(const std::string& command) {
    #ifdef _WIN32
        // Use _popen on Windows
        FILE* pipe = _popen(command.c_str(), "r");
    #else
        FILE* pipe = popen(command.c_str(), "r");
    #endif
    if (!pipe) return "";

    std::string result;
    std::array<char, 4096> buffer;
    while (fgets(buffer.data(), static_cast<int>(buffer.size()), pipe) != nullptr) {
        result += buffer.data();
    }

    #ifdef _WIN32
        int rc = _pclose(pipe);
    #else
        int rc = pclose(pipe);
    #endif

    if (rc != 0) return "";
        return result;
}


/// i cant find it!
bool turtle::command_exists(const std::string& command) {
    #ifdef _WIN32
        return run_quiet("where " + command) == 0;
    #else
        /// roses are red, violets are blue, we don't know what's best for these two.
        if (run_quiet("command -v " + command) == 0) return true;
        return run_quiet("which " + command) == 0;
    #endif
}
