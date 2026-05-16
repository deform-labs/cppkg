#pragma once

#include <stdexcept>
#include <fstream>
#include <string>

inline void create_file(const std::string& name, const std::string& path, const std::string& content) {
    std::ofstream file(path + "/" + name);
    if (!file.is_open())
        throw std::runtime_error("Failed to create file: " + path + "/" + name);
    file << content;
}
