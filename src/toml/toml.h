#pragma once
#include <unordered_map>
#include <stdexcept>
#include <fstream>
#include <string>

struct TomlValue {
    std::string value;
};

struct TomlSection {
    std::unordered_map<std::string, TomlValue> keys;
};

struct Toml {
    std::unordered_map<std::string, TomlSection> sections;

    std::string get(const std::string& section, const std::string& key) const {
        auto s = sections.find(section);
        if (s == sections.end())
            throw std::runtime_error("Section not found: " + section);
        auto k = s->second.keys.find(key);
        if (k == s->second.keys.end())
            throw std::runtime_error("Key not found: " + key);
        return k->second.value;
    }

    bool has(const std::string& section, const std::string& key) const {
        auto s = sections.find(section);
        if (s == sections.end()) return false;
        return s->second.keys.count(key) > 0;
    }
};