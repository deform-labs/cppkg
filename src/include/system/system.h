#pragma once
#include <string>

class SystemService {
    public:
        int run(const std::string& command);

        int run_quiet(const std::string& command);

        std::string run_with_output(const std::string& command);
};
