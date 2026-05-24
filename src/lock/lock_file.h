#pragma once
#include <filesystem>
#include <fstream>
#include <string>
#include <unordered_map>

struct LockEntry {
    std::string version;
    std::string hash;
};

// author/repo -> LockEntry
using LockMap = std::unordered_map<std::string, LockEntry>;

struct LockFile {
    std::string path;

    LockFile(const std::string& path) : path(path) {}

    bool exists() const;
    LockMap read() const;
    void write(const LockMap& entries) const;
    void upsert(const std::string& name, const std::string& version, const std::string& hash) const;
    void remove_entry(const std::string& name) const;
};