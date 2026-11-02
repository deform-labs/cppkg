#ifndef TOML_SECTIONS_H
    #define TOML_SECTIONS_H

    #include <unordered_map>
    #include "toml_value.h"

    namespace cppkg {
        struct TomlSection {
            std::unordered_map<std::string, TomlValue> keys;
        };
    }
#endif