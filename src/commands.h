#pragma once
#include <functional>
#include <stdexcept>
#include <vector>
#include <string>

/// C O M M A N D
struct Command {
    std::string name;
    std::string description;

    //for beginners this just gives you a way to run a function with arguments
    std::function<void(int argc, char* argv[])> execute;

    Command(std::string name, std::string desc, std::function<void*(int, char*[])> fn)
        : name(std::move(name)), description(std::move(desc)), execute(std::move(fn)) {}
};

/// registry for managing commands (WOOAH I DIDNT KNOW THAT!)
struct CommandRegistry {
    std::vector<Command> commands;

    /// i think you should go to a doctor if you dont understand what this does
    void addCommand(const Command& command) {
        commands.push_back(command);
    }

    // same goes here lol
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
