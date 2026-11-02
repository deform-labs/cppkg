#ifndef COMMANDS_H
    #define COMMANDS_H

    #include <string>

    namespace cppkg {
        class CommandSystem {
            public:
            int Run(const std::string& command);

            int RunQuiet(const std::string& command);

            std::string RunWithOutput(const std::string& command);

            bool DoesCommandExist(const std::string& command);
        };
    }

    #include "structs/command.h"
    #include "structs/registry.h"
    #include "caller/alias.h"
#endif