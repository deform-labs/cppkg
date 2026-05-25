#include "toml_parser.h"
using namespace cppkg;

Toml toml_parser::parse_toml(const std::string& path) {
    std::ifstream file(path);

    if (!file.is_open())
        throw std::runtime_error("Could not open: " + path);

    Toml result;
    std::string current_section;
    std::string line;

    while (std::getline(file, line)) {
        size_t start = line.find_first_not_of(" \t");
        if (start == std::string::npos) continue;
        line = line.substr(start);

        if (line[0] == '#') continue;

        if (line[0] == '[') {
            size_t end = line.find(']');
            if (end == std::string::npos)
                throw std::runtime_error("Malformed section: " + line);
            current_section = line.substr(1, end - 1);
            continue;
        }

        size_t eq = line.find('=');
        if (eq == std::string::npos) continue;

        std::string key = line.substr(0, eq);
        std::string val = line.substr(eq + 1);

        auto trim = [](std::string s) {
            size_t a = s.find_first_not_of(" \t\"");
            size_t b = s.find_last_not_of(" \t\"");
            if (a == std::string::npos) return std::string();
            return s.substr(a, b - a + 1);
        };

        key = trim(key);
        val = trim(val);

        result.sections[current_section].keys[key] = { val };
    }

    return result;
}

std::vector<std::string> toml_parser::parse_array(const std::string& val) {
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