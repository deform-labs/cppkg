#pragma once
#include <stdexcept>
#include <sstream>
#include <fstream>
#include <string>
#include <vector>
#include "../structs/toml_struct.h"

namespace cppkg {
    class toml_parser {
        public:
            Toml parse_toml(const std::string& path);
            std::vector<std::string> parse_array(const std::string& val);
    };
}