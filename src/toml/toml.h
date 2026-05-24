#pragma once
#include <unordered_map>
#include <stdexcept>
#include <fstream>
#include <sstream>
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

    std::vector<std::string> parse_array(const std::string& val) {
        std::vector<std::string> result;
        size_t start = val.find('[');
        size_t end = val.find(']');

        if (start == std::string::npos || end == std::string::npos) return result;

        std::string inner = val.substr(start + 1, end - start - 1);
        std::stringstream ss(inner);
        std::string item;

        while (std::getline(ss, item, ',')) {
            size_t a = item.find_first_not_of(" \t\"");
            size_t b = item.find_last_not_of(" \t\"");
            if (a != std::string::npos)
                result.push_back(item.substr(a, b - a + 1));
        }

        return result;
    }
};