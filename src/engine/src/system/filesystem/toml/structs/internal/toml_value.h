#ifndef TOML_VALUE_H
    #define TOML_VALUE_H

    #include <string>
    #include <vector>

    namespace cppkg {
        struct TomlValue {
            /// Scalar text with surrounding quotes removed and escapes resolved.
            std::string value;

            /// Array elements in file order; empty for a scalar.
            std::vector<std::string> items;
            bool is_array = false;
        };
    }
#endif
