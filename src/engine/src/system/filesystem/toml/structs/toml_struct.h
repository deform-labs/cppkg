#ifndef TOML_STRUCT_H
    #define TOML_STRUCT_H

    #include "internal/toml_sections.h"
    #include <unordered_map>
    #include <sstream>
    #include <string>
    #include <vector>

    namespace cppkg {
        struct Toml {
            /// File these tables came from; every error message is prefixed with it.
            std::string source;
            std::unordered_map<std::string, TomlSection> sections;

            /// Scalar value of a key. Throws naming the file when the table or key is missing.
            std::string get(const std::string& section, const std::string& key) const;
            /// Same as get(), but returns the fallback instead of throwing.
            std::string get_or(const std::string& section, const std::string& key, const std::string& fallback) const;
            bool has(const std::string& section, const std::string& key) const;
            bool has_section(const std::string& section) const;
            /// Key names of a table, sorted, for "unknown key" diagnostics.
            std::vector<std::string> keys(const std::string& section) const;
            /// Elements of an array key; empty when the key is absent or holds a scalar.
            std::vector<std::string> array(const std::string& section, const std::string& key) const;
            std::vector<std::string> parse_array(const std::string& val);
        };
    }
#endif
