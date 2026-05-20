#pragma once
#include <string>

class turtle {
    public:
        int run(const std::string& command);

        int run_quiet(const std::string& command);

        std::string run_with_output(const std::string& command);

        bool command_exists(const std::string& command);

    private:
        int shell_count = 1; // only one. always.
};
