#ifndef REGISTRY_H
    #define REGISTRY_H
    #include <functional>
    #include <stdexcept>
    #include "command.h"
    #include <string>

    /// registry for managing commands (WOOAH I DIDNT KNOW THAT!)
    struct CommandRegistry {
        std::vector<Command> commands;

        /// i think you should go to a doctor if you dont understand what this does
        void addCommand(const Command& command) {
            commands.push_back(command);
        }

        /// same goes here lol
        void executeCommand(const std::string& name, int argc, char* argv[]) {
            for (const auto& command : commands) {
                if (command.name == name) {
                    command.execute(argc, argv);
                    return;
                }
            }
            throw std::runtime_error("Command not found: " + name);
        }
    };
#endif // REGISTRY_H