#ifndef COMMANDS_H
    #define COMMANDS_H

    #include <string>

    class command_system {
        public:
            int run(const std::string& command);

            int run_quiet(const std::string& command);

            std::string run_with_output(const std::string& command);

            bool command_exists(const std::string& command);
    };

    #include "structs/command.h"
    #include "structs/registry.h"
#endif