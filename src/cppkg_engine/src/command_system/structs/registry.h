#ifndef REGISTRY_H
    #define REGISTRY_H
    #include <functional>
    #include <iostream>
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
        // In CommandRegistry class
        void executeCommand(const std::string& name, const std::vector<std::string>& args) {
            for (const auto& cmd : commands) {
                if (cmd.name == name) {
                    // Build argv array
                    std::vector<char*> argv;
                    argv.push_back(const_cast<char*>(name.c_str()));
                    for (const auto& arg : args) {
                        argv.push_back(const_cast<char*>(arg.c_str()));
                    }
                    argv.push_back(nullptr);
                    cmd.execute((int)argv.size() - 1, argv.data());
                    return;
                }
            }
            std::cout << "Command not found: " << name << std::endl;
        }
    };
#endif // REGISTRY_H