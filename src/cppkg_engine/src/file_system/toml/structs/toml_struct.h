#ifndef TOML_STRUCT_H
    #define TOML_STRUCT_H

    #include "internal/toml_sections.h"
    #include <unordered_map>
    #include <sstream>
    #include <string>
    #include <vector>

    namespace cppkg {
        struct Toml {
            std::unordered_map<std::string, TomlSection> sections;

            std::string get(const std::string& section, const std::string& key) const;
            bool has(const std::string& section, const std::string& key) const;
            std::vector<std::string> parse_array(const std::string& val);
        };
    }
#endif